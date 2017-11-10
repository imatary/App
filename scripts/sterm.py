#!/usr/bin/env python
#   Copyright (c) 2007 Axel Wachtler
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
##
# @file
# @brief serial terminal application
#
#
"""
sterm.py - uracoli serial terminal application

Usage:
 python sterm.py [OPTIONS]

Options:
 -h, --help
    show this help message
 -V, --version
    show version number
 -v, --verbose
    increase verbose level, each -v increases the level by +1
 -C, --config <cfgfile>
    load config file, default value is "sterm.cfg"
    if <cfgfile> is not existing, an annotated version
    is generated and sterm.py exits.
 -d, --devices <devive[,device]>
    select devices in terminal via command line
    this overwrites the "devices" key in the config
    file. default: [], devices will be selected according
    to config file.
--eol <CRLF>
    select line end characters
    LF = "\n", CR = "\r", CRLF = "\r\n", LFCR = "\n\r"
    default: LF
"""

# === Imports ==================================================================
from Tkinter import *
from ScrolledText import ScrolledText
from FileDialog import FileDialog
from ConfigParser import RawConfigParser
import sys
import threading
import time
import Queue
import pprint
import serial
import re
import getopt
import os
import traceback
import atexit


# === Globals ==================================================================
CFG_TEMPLATE = """
[sterm.py]
devices = dev1
# EOL configuration: LF = "\n", CR = "\r", CRLF = "\r\n", LFCR = "\n\r"
eol = LF
#plugins = myfile1.py /some/other/place/file.py

# === Example Serial Devices ===================================================
[dev1]
type = serial
port = /dev/ttyUSB0
baudrate = 38400
macros = qbf
connect = 1

# === Example Fifos ============================================================
# [fifo1]
# type = fifo
# macros = qbf peli
#
# [fifo2]
# type = fifo
#
# === Example Macro ============================================================
[qbf]
text = The quick brown fox jumps over the lazy dog.

[peli]
text = A wonderful bird is the pelican,
        His bill will hold more than his belican,
        He can take in his beak
        Enough food for a week
        But I'm damned if I see how the helican!

[foo]
function = usr_func

"""
FRAMEBORDER = {"relief":"groove", 'border':1}
FRAMEBORDER1 = {"relief":"ridge", 'border':3}
VERSION = "0.1"
CFGFILE = "sterm.cfg"
CFGSECTION = "sterm.py"
VERBOSE = 0
CRLF_DECODE = {"CR" : "\r",
               "LF" : "\n",
               "CRLF" : "\r\n",
               "LFCR" : "\n\r"}
EOL = "LF"
# === debug helpers ============================================================
pp = pprint.pprint
ppd = lambda x: pp(dir(x))
try:
    import readline, rlcompleter
    readline.parse_and_bind("tab:complete")
except:
    pass

# === Classes ==================================================================

##
# @brief Terminal IO helper class using File Fifos
# This class is used to simulate serial IO connections between
# terminal windows.
#
class FifoIo:
    def __init__(self, name="dev"):
        self.inQueue = Queue.Queue()
        self.name = name
        self.outQueue = None
        self.peer = None
        self.isConnected = False
        self.logmode = "a"
        self.logname = "sterm_%s.log" % name
        self.log = 0
        self.connect = 0

    def configure(self, **cfg):
        if cfg.has_key('peer'):
            self.peer = eval(cfg["peer"])
        self.logname = cfg.get('logname', self.logname)
        self.logmode = cfg.get('logmode', self.logmode)
        self.log = eval(cfg.get('log', "0"))
        self.auto_connect = eval(cfg.get('connect', "0"))

    def disconnect(self):
        self.isConnected = False
        if self.peer:
            self.outQueue = None

    def connect(self):
        self.outQueue = self.peer.inQueue
        self.isConnected = True

    def write(self, data):
        try:
            self.outQueue.put(data)
        except:
            pass

    def read(self):
        return self.inQueue.get()

##
# Serial IO class
class SerialIo:
    def __init__(self, name="dev"):
        self.sport = serial.Serial(timeout = 2)
        self.name = name
        self.isConnected = False
        self.logmode = "a"
        self.logname = "sterm_%s.log" % name
        self.log = 0
        self.auto_connect = 0
        self.TxQueue = None
        self.RxQueue = None

    def configure(self, **cfg):
        for k,v in cfg.items():
            _print(2, self.name, k, v)
            if k =='port':
                self.sport.port = cfg['port']
                self.sport.baudrate = cfg['baudrate']
            elif k == 'logname':
                self.logname = v
            elif k == 'logmode':
                self.logmode = v
            elif k == 'log':
                self.log = bool(str(v))
            elif k == 'connect':
                self.auto_connect = bool(str(v))
            elif k == "RxQueue":
                self.RxQueue = v
            elif k == "TxQueue":
                self.TxQueue = v

    def disconnect(self):
        self.isConnected = False
        self.sport.close()

    def connect(self):
        self.sport.close() # new version of pyserial open port at instantiation
        self.isConnected = True
        self.sport.open()

    def write(self, data):
        if self.sport.isOpen():
            self.sport.write(data)
        else:
            _print(0, "%s: error write to closed port" % self)


    def read(self):
        if self.sport.isOpen():
            x = self.sport.read(1)
            n = self.sport.inWaiting()
            x += self.sport.read(n)
        else:
            x = ""
        return x

    def __str__(self):
        return "%s@%s:%s" % (self.name, self.sport.port, self.sport.baudrate)

    ##
    # worker function for threaded reading from the input of
    # the assigned device
    #
    def run(self):
        self.rxdata = ""
        self.txdata = ""
        while 1:
            try:
                x = self.read()
                if len(x):
                    self.rxdata += x
                if self.RxQueue and len(self.rxdata):
                    self.RxQueue.put((self.rxdata, "T"))
                    self.rxdata = ""
            except Exception, e:
                print e
                time.sleep(1)

            if (self.TxQueue != None) and not self.TxQueue.empty():
                self.txdata = self.TxQueue.get_nowait()
                _print(3, self.name, self.txdata)
                if self.txdata:
                    self.write(self.txdata)

            # this infinitesimal sleep keeps the GUI update alive
            time.sleep(.01)

##
# terminal windows class
#
# @ingroup stermGUI
#
class TermWindow(Frame):
    EOL = None
    ##
    # constructor method
    def __init__(self, master = None, **cnf):
        apply(Frame.__init__, (self, master), cnf)
        self.__create_widgets__()
        self.rxWaitPattern = ""
        self.localEcho = False
        self.logFile = None
        self.inQueue = Queue.Queue()
        self.outQueue = Queue.Queue()
        self.update_thread_safe()

    def update_thread_safe(self):
        while not self.inQueue.empty():
            element = self.inQueue.get_nowait()
            if element is not None:
                msg,tag = element
                self.__display__(msg,tag)
            self.st_trm.update_idletasks()
        self.st_trm.after(100, self.update_thread_safe)

    ##
    #
    def __create_widgets__(self):
        dfl_font = 'Courier 10'

        # the title frame of the terminal
        self.f_title = Frame(self)
        self.f_title.pack(fill=BOTH)
        self.f_title.configure(FRAMEBORDER)
        self.shVar = StringVar()
        # show/hide button
        self.b_sh = Button(self.f_title, textvariable=self.shVar, font=dfl_font)
        self.b_sh.pack(side=RIGHT)
        self.b_sh['command'] = self.show_hide_cont
        # clear screen button
        self.b_cls = Button(self.f_title, text="CLS", font=dfl_font, underline=0)
        self.b_cls.pack(side=RIGHT)
        self.b_cls['command'] = self.clear_screen
        # echo on/off button
        self.al_echo = Label(self.f_title, text = "ECHO", relief = RAISED,
                             font = dfl_font, padx='3m', pady='1m', underline=0)
        self.al_echo.pack(side=RIGHT, padx=1, pady=1, fill=BOTH)
        self.al_echo.bind("<Button-1>", self.__echo_handler__)
        # log on/off button
        self.al_log = Label(self.f_title, text = "LOG", relief = RAISED,
                            font = dfl_font, padx='3m', pady='1m', underline=0)
        self.al_log.pack(side=RIGHT, padx=1, pady=1, fill=BOTH)
        self.al_log.bind("<Button-1>", self.__log_handler__)
        # device connect button
        self.al_connect = Label(self.f_title, text = "CONNECT", relief = RAISED,
                                font = dfl_font, padx='3m', pady='1m', underline=1)
        self.al_connect.pack(side=RIGHT, padx=1, pady=1, fill=BOTH)
        self.al_connect.bind("<Button-1>", self.__connect_handler__)

        self.mb_macros = Menubutton(self.f_title, text = "Macros", relief=RAISED)
        self.mb_macros.pack(side=RIGHT, padx=1, pady=1, fill=BOTH)
        self.mb_macros.menu = Menu(self.mb_macros, tearoff = 0)
        self.mb_macros["menu"] = self.mb_macros.menu


        # title of terminal window
        self.tVar = StringVar()
        self.l_title = Label(self.f_title, text="foo", font=dfl_font)
        self.l_title['textvariable'] = self.tVar
        self.l_title.pack(side=LEFT, expand=1, fill=X)
        self.l_title['width'] = 42
        self.update_title("------ XXX ------")
        # frame for scrolled text window
        # (this frame handle needs to be kept fo show_hide_cont())
        self.f_cont = Frame(self)
        # IO data scrolled text window
        self.st_trm = ScrolledText(self.f_cont, height=10, state=DISABLED, wrap=NONE)
        self.st_trm.pack(expand=1,fill=BOTH)
        self.st_trm['font'] = dfl_font
        self.st_trm.tag_config('E', foreground="blue")
        self.st_trm.tag_config('M', foreground="magenta")


        tframe = Frame(self.f_cont)
        tframe.pack(expand = 0, fill = X)
        self.cmdVar = StringVar()
        self.ent_trm = Entry(tframe, textvariable=self.cmdVar, font=dfl_font)
        self.ent_trm.pack(side=LEFT, expand =1, fill = X)
        self.ent_trm.bind("<Control-l>", self.__log_handler__)
        self.ent_trm.bind("<Control-e>", self.__echo_handler__)
        self.ent_trm.bind("<Control-o>", self.__connect_handler__)
        self.ent_trm.bind("<Control-c>", self.clear_screen)
        self.ent_trm.bind("<Control-x>", self.show_hide_cont)
        self.ent_trm.bind("<Control-m>", lambda *args: self.do_macro("M"))
        self.ent_trm.bind("<KeyPress>", self.__input_handler__)

        self.gui_elements = [ self.b_sh,
                              self.b_cls,
                              self.al_echo,
                              self.al_log,
                              self.al_connect,
                              self.mb_macros,
                              self.l_title,
                              self.st_trm,
                              self.ent_trm]
        self.show_cont()

    def add_macro(self, id, title, text = None, function = None, params = None):
        if text:
            cmd = lambda *args: self.do_macro(text)
        if function:
            user_func = eval(function)
            if params:
                params = eval(str(params))
            else:
                params = {}
            cmd = lambda *args: user_func(self, DEVICES, **params)
        mb = self.mb_macros.menu.add_command(label = title, command = cmd)

    def _configure_(self,**args):
        for e in self.gui_elements:
            e.configure(args)

    def __input_handler__(self, *args):
        for i in args:
            self.terminal_device_write(i.char)

    def terminal_device_write(self, *args):
        for i in args:
            if self.localEcho:
                self.display(i, "E")
            if len(i):
                if i == "\r" or i == "\n":
                    self.device.write(self.EOL)
                    self.display(self.EOL, "E")
                    self.cmdVar.set("")
                else:
                    self.device.write(i)
        self.st_trm.update_idletasks()

    def __echo_handler__(self, *args):
        if self.localEcho:
            self.localEcho = False
            self.al_echo['relief'] = RAISED
            self.message("Local Echo OFF")
        else:
            self.localEcho = True
            self.al_echo['relief'] = SUNKEN
            self.message("Local Echo ON")

    def __log_handler__(self, *args):
        try:
            do_open = self.logFile.closed
            logname = self.logFile.name
        except:
            do_open = True
            logname = ""
        if do_open:
            if self.device.logname:
                logname = self.device.logname
                self.message("logfile from config file %s " % logname)
            else:
                fd = FileDialog(self)
                logname = fd.go(logname)
            try:
                if self.device.logmode not in "wa":
                    self.device.logmode = "a"
                    _print(3, "force _a_ppend")
                self.logFile = open(logname, self.device.logmode)
                self.al_log['relief'] = SUNKEN
                self.message("Logging ON: %s" % self.logFile.name)
            except:
                self.message("Error open logfile: %s" % logname)

        else:
            self.message("Logging OFF: %s" % self.logFile.name)
            self.logFile.flush()
            self.logFile.close()
            self.al_log['relief'] = RAISED

    def __connect_handler__(self, *args):
        if self.device.isConnected:
            self.device.disconnect()
            self.al_connect['relief'] = RAISED
            self.message("Disconnected")
        else:
            try:
                self.device.connect()
                self.al_connect['relief'] = SUNKEN
                self.message("Connected")
                self.al_connect['fg'] = "black"
            except:
                self.device.isConnected = False
                self.message( str(sys.exc_info()[1]) )
                self.al_connect['fg'] = "red"

    def clear_screen(self, *args):
        self.st_trm['state'] = NORMAL
        self.st_trm.delete("0.0",END)
        self.st_trm['state'] = DISABLED

    def set_device(self, device):
        self.device = device
        self.device.configure(TxQueue = self.outQueue, RxQueue = self.inQueue )
        self.update_title(self.device)

        if self.device.log:
            self.__log_handler__()
        if self.device.auto_connect:
            self.__connect_handler__()

    def update_title(self, title):
        self.tVar.set(title)

    def show_cont(self):
        self.shVar.set("X")
        self.f_cont.pack(expand=1,fill=BOTH)

    def hide_cont(self):
        self.shVar.set("+")
        self.f_cont.pack_forget()

    def show_hide_cont(self, *args):
        if self.shVar.get() == "X":
            self.hide_cont()
        else:
            self.show_cont()

    def do_macro(self, *args):
        if self.localEcho:
            self.display(args[0] + "\n", "E")
        self.device.write(args[0]+ "\n")

    def write(self, data):
        self.outQueue.put((data, None))

    def message(self, text, tag='M'):
        msg = "[%s:%s:%s]\n" % (time.asctime(),self.device.name, text)
        if self.st_trm.index(AtInsert()).find(".0")  < 1:
            msg = "\n" + msg
        self.inQueue.put((msg, tag))

    def display(self, text, tag = None):
        self.inQueue.put((text, tag))

    def __display__(self, msg, tag = None):
        self.st_trm['state'] = NORMAL
        here =  self.st_trm.index(AtInsert())
        for d in re.split("([\r\v\t\n])", msg):
            if len(d):
                if d != '\r':
                    self.st_trm.insert(END, d)
        if tag:
            self.st_trm.tag_add(tag, here, AtInsert())
        self.st_trm.see(END)
        self.st_trm['state'] = DISABLED
        try:
            self.logFile.write(msg)
            self.logFile.flush()
        except:
            pass

##
# Terminal
class TermMain(Frame):
    def __init__(self,master = None, **cnf):
        apply(Frame.__init__, (self, master), cnf)

# === Functions ========================================================

def _print(level, msg, *args):
    if level <= VERBOSE:
        msg = " ".join(map(str, [msg] + list(args)))
        print "%d>%s" % (level, msg)

def sterm_init(cfg):
    # parse arguments
    global root, mw, DEVICES
    root = Tk()
    root.title("sterm V%s" % VERSION)
    mw = []

    # Menu List
    mainmenu = Menu(root)
    filemenu = Menu(root)
    filemenu.add_command(label = "Exit", command = root.quit)
    mainmenu.add_cascade(label = "File", menu = filemenu)
    mainmenu.add_cascade(label = "Help")

    root.config(menu = mainmenu)
    _print(1, DEVICES)
    for dname, d in sorted(DEVICES.items()):
        if not d:
            _print(3, "Ignore %s" % d, dname)
            continue
        m = TermWindow(root)
        m.configure(FRAMEBORDER1)
        m.pack(expand=1, fill=BOTH)
        mw.append(m)
        m.set_device(d)
        d.termwindow = m
        try:
            f = cfg.get(CFGSECTION, "font")
        except:
            f = 'Helvetica 10'
        m._configure_(font = f)
        m.EOL = CRLF_DECODE[cfg.get(CFGSECTION, "eol")]
        try:
            f = cfg.get(CFGSECTION, "term_font")
        except:
            f = 'Courier 10'
        m.st_trm.configure(font = f)
        macros = []
        if cfg.has_option(CFGSECTION, "macros"):
            macros += cfg.get(CFGSECTION, "macros").split()
        if cfg.has_option(dname, "macros"):
            macros += cfg.get(dname, "macros").split()
        for macro in macros:
            if not cfg.has_section(macro):
                continue
            if cfg.has_option(macro, "text"):
                m.add_macro(macro, macro, text = cfg.get(macro, "text"))
            elif cfg.has_option(macro, "function"):
                if cfg.has_option(macro, "params"):
                    params = cfg.get(macro, "params")
                else:
                    params = {}
                m.add_macro(macro, macro,
                            function = cfg.get(macro, "function"),
                            params = params)

def configure_devices(cfg, devlist = None):
    ret = {}
    if devlist == None:
        devlist = cfg.get(CFGSECTION,"devices").split()
    for i in devlist:
        if not cfg.has_section(i):
            _print(3, "Ignore %s, no such device defined" % i)
            continue
        devcfg = dict(cfg.items(i))
        dev_varname =  "dev_%s" % i
        dev_type = devcfg.get('type','serial')
        if dev_type == 'fifo':
            dev = FifoIo(devcfg.get("name",i))
        elif dev_type == 'serial':
            dev = SerialIo(devcfg.get("name",i))
        else:
            _print(2, "Unknown Type", devcfg['type'])
            dev = None
        if dev != None:
            dev.configure(**devcfg)
            ret[i] = dev
            exec("global %s; %s = dev" % (dev_varname,dev_varname))
    return ret

# example user function that can be assigned with a macro
# the tuple args contains the device dictionary
def usr_func(win, devices, *args):
    _print(1, "usr_func", win, devices, args)
    win.terminal_device_write("Arguments: %s %s %s" % (win, devices, args))


def sterm_wrapper(fu):
    def new_fu(*args, **kwargs):
        import StringIO
        fh = StringIO.StringIO()
        sys.stdout = fh
        try:
            args[0].message("sterm_wrapper:" + str(fu.__name__), tag="M")
        except:
            pass
        rv = fu(*args[1:], **kwargs)
        args[0].display(str(sys.stdout.getvalue()) + "\n", tag="E")
        sys.stdout = sys.__stdout__
        fh.close()
        return rv
    return new_fu

##
# This function closes the com ports of the devices at exit.
# It is needed to get the entire application stopped in Linux.
# Otherwise the application may hang in an endles loop
def exit_function(devices):
    for d in devices:
        print "closing:", d
        d.sport.close()

if __name__ == "__main__":
    opts, args = getopt.getopt(sys.argv[1:],"hVC:d:v",
                               ["config=", "devices=", "verbose",
                                "help", "version", "eol="])
    do_exit = False

    device_list = []
    for o,v in opts:
        if o in ('-h','--help'):
            print __doc__
            do_exit = True
        elif o in ('-d','--devices'):
            device_list += [d.strip() for d in v.split(",")]
        elif o in ('-V','--version'):
            print "sterm V%s" % VERSION
            do_exit = True
        elif o in ('-v','--verbose'):
            VERBOSE += 1
        elif o in ('-C','--config'):
            CFGFILE = v
        elif o in ('--eol',):
            EOL = v

    if not os.path.exists(CFGFILE):
        _print(2, "create file %s" % os.path.abspath(CFGFILE))
        _print(2, "edit this file to adapt it to your environment")
        f = open(CFGFILE,"w")
        f.write(CFG_TEMPLATE)
        f.close()
        do_exit = True
    elif not os.path.isfile(CFGFILE):
        _print(2, "%s is not a file" % os.path.abspath(CFGFILE))
        do_exit = True

    if do_exit:
        sys.exit(1)

    # start with setup the environment
    root = None
    _print(2, "reading:", os.path.abspath(CFGFILE))
    CFGDATA = RawConfigParser({'eol': EOL})
    CFGDATA.read(CFGFILE)

    if CFGDATA.has_option(CFGSECTION, "plugins"):
        for plugin in CFGDATA.get(CFGSECTION, "plugins").split():
            _print(2, plugin)
            if os.path.isfile(plugin):
                _print(3, "exec plugin: %s" % plugin)
                execfile(plugin)
    _print(1, "device_list=", device_list)
    if len(device_list):
        DEVICES = configure_devices(CFGDATA, device_list)
    else:
        DEVICES = configure_devices(CFGDATA)

    for dev in DEVICES.values():
        _print(2, dev)
        thread = threading.Thread(target = dev.run)
        thread.setDaemon(1)
        thread.start()
        thread.setName("IO_%s" % dev.name)
        _print(3, thread)

    _print(3, str(threading.enumerate()))
    atexit.register(exit_function, DEVICES.values())
    sterm_init(CFGDATA)

    root.mainloop()
