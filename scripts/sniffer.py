#!/usr/bin/env python
#   Copyright (c) 2011 - 2014 Axel Wachtler
#   All rights reserved.
#
#   Redistribution and use in source and binary forms, with or without
#   modification, are permitted provided that the following conditions
#   are met:
#
#   * Redistributions of source code must retain the above copyright
#     notice, this list of conditions and the following disclaimer.
#   * Redistributions in binary form must reproduce the above copyright
#     notice, this list of conditions and the following disclaimer in the
#     documentation and/or other materials provided with the distribution.
#   * Neither the name of the authors nor the names of its contributors
#     may be used to endorse or promote products derived from this software
#     without specific prior written permission.
#
#   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
#   AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
#   IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
#   ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
#   LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
#   CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
#   SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
#   INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
#   CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
#   ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
#   POSSIBILITY OF SUCH DAMAGE.

# $Id$
"""\
uracoli Sniffer Proxy - version %s

The script dumps a PCAP file with the received frames to stdout.

Usage:  python sniffer.py [OPTIONS]

Options:
    -p PORT:
        initial data rate.
    -c CHANNEL:
        initial channel to be used.
    -r RATE:
        initial data rate to be used.
    -s SFD:
        initial SFD value to be used.
    -h:
        Show help and exit.
    -V:
        show version and exit.
    -X:
        start the script headless without GUI,
        that means, you need to provide parameters, like channel, rate, special
        SFD via command line options.

Interactive Examples:

    Open the sniffer proxy and dump frames to wireshark:
    $ python sniffer.py -p COM1 -c 17 | wireshark -ki -

    Open the sniffer proxy and simultaneously send the frames to wireshark and
    to a log file:
    $ python sniffer.py -p COM1 -c 17 | tee capture.pcap | wireshark -ki -

Shell Examples:

    Dump data to pcap file, that later can be inspected with wireshark/tshark.
    $ python sniffer.py -p COM1 -c 17 -X > capture.pcap

    Open the sniffer proxy together with tshark in a text terminal headless,
    which is good for scripting applications:
    $ python sniffer.py -p COM1 -c 17 -X | tshark -i -

"""

# === import ===================================================================
import sys, os, ctypes, time
import traceback, getopt, threading, atexit

import ieee802154_base as base
from sniffer_io import PortIn
from Tkinter import *

# === globals ==================================================================
VERSION = "0.42"
CTX = dict(rate = None, channel = None, port = None, sfd = None)
DEVOUT = None
DEVIN = None
USEGUI = True

# === output class =============================================================
class StdOut(base.PcapBase):
    VERBOSE = 0
    activity_callback = None
    ## conctructor
    def open(self, sockname, devin):
        try:
            import msvcrt
            msvcrt.setmode(sys.stdout.fileno(), os.O_BINARY)
            sys.stderr.write("unbuffered sdtout in Windows\n")
        except:
            unbuffered = os.fdopen(sys.stdout.fileno(), "wb", 0)
            sys.stdout = unbuffered
            sys.stderr.write("unbuffered sdtout in Linux\n")
        self.InputDev = devin
        self.TxThread = threading.Thread(target = self.__tx__)
        self.TxThread.setDaemon(1)
        self.TxThread.start()
        self.TxThread.setName("TX")

    ## closing the socket
    def close(self):
        sys.stdout.flush()
        pass

    ## regular messagporte
    def message(self,lvl,msg,*args):
        if self.VERBOSE >= lvl:
            prompt = "+"*(lvl)
            msg = msg%args
            msg = msg.replace("\n","\n> ")
            sys.stderr(prompt+msg)
            sys.stderr.flush()

    ## periodically send data retrived from the input device
    # (a instance of @ref PcapFile or @ref PcapFile).
    def __tx__(self):
        cnt = 0
        sys.stdout.write(self.InputDev.pcap_get_header())
        sys.stdout.flush()
        while 1:
            try:
                d = self.InputDev.read_packet()
                if d:
                    sys.stdout.write(d)
                    sys.stdout.flush()
                    self.message(1,"cnt=%d pack=%d len=%d", cnt, self.InputDev.FCNT, len(d))
                    if self.VERBOSE > 1:
                        self.message(self.VERBOSE,":%08d:P:% 3d:%s",
                                     self.InputDev.FCNT,len(d),str(self.InputDev))
                        self.message(self.VERBOSE,":".join(map(hex,map(ord,d))))
                    cnt += 1
                    if self.activity_callback:
                        self.activity_callback()
            except IOError, e:
                print >>sys.stderr, "IOError"
                import thread
                thread.interrupt_main()
            except:
                traceback.print_exc()
                break

# === Graphical User Interface class ===========================================
class CtrlGui(threading.Thread):
    def __init__(self, devin, devout):
        self.devin = devin
        self.devout = devout
        self.doSniff = False
        self.root=Tk()

        master = self.root
        master.title("uracoli sniffer interface V%s" % VERSION)
        devcfg = self.devin.info()

        menuframe = Frame(master)

        # === File ===
        fmenu = Menubutton(menuframe, text = "File", underline = 0)
        fmenu.pack(side = LEFT)

        filemenu = Menu(fmenu)
        filemenu.add_command(label = "Quit",
                command = master.quit)
        fmenu.configure(menu = filemenu)
        # === Activity ===
        self.ActivityCnt = 1
        self.al_activity = Label(menuframe, text= "#", width = 1)
        self.al_activity.pack(side=RIGHT)

        # === Help ===
        bhelp = Menubutton(menuframe, text = "Help", underline = 0)
        bhelp.pack(side = RIGHT)

        helpmenu = Menu(bhelp)
        helpmenu.add_command(label = "Help",
                command = lambda : self.message("\n === Help ===\n" +\
                                                __doc__ % VERSION + "\n"))
        helpmenu.add_command(label = "About",
                command = lambda : self.message("\n === About ===\n"\
                                                "uracoli sniffer proxy %s\n"\
                                                "http://www.uracoli.de\n\n" % VERSION))

        bhelp.configure(menu = helpmenu)
        menuframe.pack(side = TOP, expand = N, fill = X)

        width=80
        hframe = Frame(master)

        Label(hframe,
              text= "Board: %(platform)s\nPort: %(port)s\n" % devcfg,
              justify = LEFT).pack(side=LEFT)

        hframe.pack()

        # select channel
        self.VarChannel = StringVar(master)
        self.VarChannel.set(devcfg['chan'])
        of = Frame(master, width=width)
        Label(of,text="Channel:").pack(side=LEFT)
        self.OmChannel = OptionMenu( of, self.VarChannel,
                            *tuple(devcfg['clist']),
                            command=self.update_channel)
        self.OmChannel.pack(side=LEFT)
        of.pack()

        # select data rate
        rates = devcfg['rates'].split()
        self.VarRate = StringVar(master)
        self.VarRate.set(str(devcfg['crate']))
        Label(of, text="Rate:").pack(side=LEFT)
        self.OmRate = OptionMenu( of, self.VarRate,
                            *tuple(rates),
                            command=self.update_rate)
        self.OmRate.pack(side=LEFT)
        sfdl = Label(of, text="SFD:")
        sfdl.pack(side=LEFT)
        self.VarSfd = StringVar(master)
        e = Entry(of, textvariable = self.VarSfd, width=4)
        if devcfg['sfd'] != None:
            self.VarSfd.set("0x%02x" % devcfg['sfd'])
            e.bind('<Return>', self.update_sfd)
        else:
            self.VarSfd.set("0xa7")
            e.configure(state = DISABLED)
        e.pack(side=LEFT)

        # start / stop
        self.al_startstop = Button(of, text = "SNIFF")
        self.al_startstop.bind("<Button-1>", self.startstop)
        self.al_startstop.pack()
        of.pack()


        # message window
        tf = Text(master,width=width,height=10)
        tf.pack(expand=YES, fill = BOTH)
        self.TxtLog = tf
        threading.Thread.__init__(self)
        self.setDaemon(1)

        self.devout.activity_callback = self.update_activity

    def message(self,txt):
        self.TxtLog.configure(state=NORMAL)
        self.TxtLog.insert(END, txt.strip()+"\n")
        self.TxtLog.yview(END)
        self.TxtLog.configure(state=DISABLED)

    def update_channel(self, channel):
        self.message("set channel %s\n" % channel)
        self.devin.set_channel(channel)

    def update_rate(self, rate):
        self.message("set rate %s\n" % rate)
        self.devin.set_rate(rate)

    def update_sfd(self, *args):
        vsfd = eval(str(self.VarSfd.get()))
        self.message("set sfd 0x%02x\n" % vsfd)
        self.devin.set_sfd(vsfd)

    def update_activity(self, *args):
        self.ActivityCnt += 1
        self.ActivityCnt &= 3
        txt = "_oOo"[self.ActivityCnt]
        self.al_activity.configure(text=txt )

    def startstop(self, *args):
        if self.doSniff:
            self.doSniff = False
            self.al_startstop['text'] = "SNIFF"
            self.devin.sniff(0)
            self.message("sniffer halted")
        else:
            self.doSniff = True
            self.al_startstop['text'] = "STOP"
            self.devin.sniff(1)
            self.message("sniffer run")

# === helper functions =========================================================
def interactive_inspect_mode():
    flagPtr = ctypes.cast(ctypes.pythonapi.Py_InteractiveFlag,
                         ctypes.POINTER(ctypes.c_int))
    return flagPtr.contents.value > 0 or \
           bool(os.environ.get("PYTHONINSPECT",False))

def exit_function():
    global DEVIN, DEVOUT
    if DEVIN:
        DEVIN.sniff(0)
        DEVIN.close()
    if DEVOUT:
        DEVOUT.close()

# === main =====================================================================
if __name__ == "__main__":
    try:
        import rlcompleter
        import readline
        readline.parse_and_bind("tab:complete")
    except:
        pass

    atexit.register(exit_function)

    opts,args = getopt.getopt(sys.argv[1:],"hvp:c:r:s:X")
    do_exit = False
    for o,v in opts:
        if o == "-p":
            CTX["port"] = v
        elif o == "-r":
            CTX["rate"] = v
        elif o == "-c":
            CTX["channel"] = eval(v)
        elif o == "-s":
            CTX["sfd"] = eval(v)
        elif o == "-X":
            USEGUI = False
        elif o == "-h":
            print __doc__ % VERSION
            do_exit = True
        elif o == "-v":
            print VERSION
            do_exit = True

    if do_exit:
        sys.exit()

    DEVIN = PortIn()
    DEVIN.open(CTX["port"])
    DEVIN.sniff(0)

    DEVOUT = StdOut()
    DEVOUT.open("xxx",DEVIN)
    DEVOUT.verbose = 2

    if CTX["channel"]:
        DEVIN.set_channel(CTX["channel"])
    if CTX["rate"]:
        DEVIN.set_rate(CTX["rate"])

    if USEGUI:
        CGUI = CtrlGui(DEVIN, DEVOUT)
        try:
            if not interactive_inspect_mode():
                CGUI.root.mainloop()
        except KeyboardInterrupt:
            print "Quitting %s" % sys.argv[0]
    else:
        print >>sys.stderr, "running headless in 2s ..."
        try:
            time.sleep(2)
            DEVIN.sniff(1)
            while 1:
                pass
        except IOError:
            print >>sys.stderr, "wireshark has terminated, pipe broken ..."


