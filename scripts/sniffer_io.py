#   Copyright (c) 2011, 2012 Axel Wachtler
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
# @ingroup grpContribCapture
# @brief File/Serial port io related classes.
#

# === import ==================================================================
import sys, serial, time, threading, Queue, struct, StringIO, traceback
from ieee802154_base import PcapBase, UtilBase

# === globals =================================================================

# === functions ===============================================================

# === classes =================================================================

# special handling for windows
RxErrors = [Exception]
try:
    import pywintypes
    RxErrors.append(pywintypes.error)
except:
    pass
RxErrors = tuple(RxErrors)


##
# @brief Serial Interface Reader.
#
class PortIn(PcapBase, UtilBase):
    TMO = 10
    FCNT = 0
    UNSYNC = 0
    SYNCED = 1
    IDXERR = 0
    BAUDRATE = 38400
    def __init__(self):
        self.RxThread = None
        self.RxQueue = Queue.Queue()
        self.PlainText = StringIO.StringIO()
        self.PlainTextLock = threading.RLock()
        self.channel = None
        self.crate = None
        self.clist = []
        self.state = self.UNSYNC
        self.sfd = None
        self.maxpackets = -1

    ## @brief Open the serial port.
    def open(self, fname):
        try:
            port,baud = fname.split(":")
        except:
            port = fname
        else:
            self.BAUDRATE = eval(baud)
        self.fname = port
        self.sport = serial.Serial(self.fname,
                                   self.BAUDRATE,
                                   timeout = self.TMO,
                                   xonxoff = False)
        self.sport.close()
        self.sport.open()
        self.sport.write("\nidle\n")
        self.RxThread=threading.Thread(target = self.__rx__)
        self.RxThread.setDaemon(1)
        self.RxThread.setName("PortInRxThread")
        self.RxThread.start()
        time.sleep(.5)
        self.init()

    def reset(self):
        self.state = self.UNSYNC
        self.sport.write("\nidle\n")
        while 1:
            l = self.get_text_queue()
            if l == None:
                break
            elif l.find("IDLE") >= 0:
                break
            else:
                time.sleep(.1)

    def init(self):
        self.update_hw_status()
        t = int(time.time())
        self.sport.write("timeset %s\n" % t)
        self.timebase = t

    def update_hw_status(self):
        self.reset()
        self.sport.write("\nparms\n")
        time.sleep(.5)
        while 1:
            l = self.get_text_queue()
            if l == None:
                break
            else:
                sys.stderr.write("*"+l+"\n")
            if l.find("PLATFORM")>=0:
                self.platform = l.split(":")[1].strip()
            if l.find("SUPP_CMSK")>=0:
                self.clist = self.get_channels(eval(l.split(":")[1]))
            if l.find("CURR_CHAN")>=0:
                self.channel = eval(l.split(":")[1])
            if l.find("TIMER_SCALE")>=0:
                self.tscale = eval(l.split(":")[1])
            if l.find("TICK_NUMBER")>=0:
                self.ticknb = eval(l.split(":")[1])
            if l.find("CURR_RATE")>=0:
                self.crate = l.split(":")[1].strip()
            if l.find("SUPP_RATES")>=0:
                self.rates = l.split(":")[1].strip()
            if l.find("SFD")>=0:
                self.sfd = eval(l.split(":")[1])

    def get_text_queue(self):
        ret = None
        with self.PlainTextLock:
            try:
                ret = self.PlainText.next()
                ret = ret.replace('\x0b',"")
            except StopIteration:
                self.PlainText.truncate()
        return ret

    def get_channels(self,cmask):
        ret = []
        cidx = 0
        while cmask != 0:
            if cmask & 1:
                ret.append(cidx)
            cidx += 1
            cmask /=2
        return ret

    def close(self):
        if self.RxThread != None:
            #self.RxThread.join()
            self.RxThread = None
        self.sport.close()

    def info(self):
        self.update_hw_status()
        ret = {'type'   : 'port',
               'chan'   : self.channel,
               'port'   : self.sport.port,
               'clist'  : self.clist,
               'tscale' : self.tscale,
               'ticknb' : self.ticknb,
               'crate'  : self.crate,
               'rates'  : self.rates,
               'platform' : self.platform,
               'sfd' : self.sfd
                }
        #self.sniff()
        return ret

    def sniff(self, status=1):
        if status:
            self.sport.write("\nsniff\n")
        else:
            self.sport.write("\nidle\n")

    def __rx__(self):
        frm = ""
        while 1:
            try:
                sdata = self.sport.read(1)
                if len(sdata) != 1:
                    continue
                n = self.sport.inWaiting()
                n = min(n, 512)
                if n:
                    sdata += self.sport.read(n)
            except RxErrors:
                break
            except:
                sys.stderr.write(str(sys.exc_info())+"\n")
                break
            frm += sdata
            if self.state == self.UNSYNC:
                with self.PlainTextLock:
                    self.PlainText.buf += sdata
                p = self.sync_search(frm)
                if p != None:
                    frm = frm[p:]
                    self.state = self.SYNCED
            if self.state == self.SYNCED:
                self.state,frm = self.packetizer(frm)
                self.message(2,"state sync after packetizer(), state=%d, len_frm=%d",self.state, len(frm))

    def sync_search(self,frm):
        ret = None
        p = 0
        nbstartpos = frm.count('\x01')
        dlen = len(frm)
        if nbstartpos:
            self.message(2,"syncsearch : dlen=%d, nbstarts=%d" ,dlen,nbstartpos)
            for idx in range(nbstartpos):
                try:
                    p += frm[p:].index('\x01')
                    plen = ord(frm[p+1])
                    pe = p + plen + 2
                    self.message(2,"syncing : idx=%d, packet=%d:%d, plen=%d dlen=%d", idx,p,pe,plen,dlen)

                    if pe <= dlen:
                        self.message(2,"packet : %s " , str(map(lambda i,f=frm: hex(ord(f[i])), (p,p+1,pe-1,pe) )))
                        if(frm[pe] == '\x04'):
                            ret = p
                            self.message(1,"synced : idx=%d, packet=%d:%d, plen=%d dlen=%d", idx,p,pe,plen,dlen)
                            raise Exception, "Synced"
                    p += 1
                except IndexError:
                    # this catches the blind access in line "l = ord(frm[p+1])"
                    break
                except Exception:
                    break
                except:
                    self.exc_handler("sync_search")
                    break
        return ret

    def packetizer(self,frm):
        state = self.SYNCED
        while 1:
            frmlen = len(frm)
            if len(frm) < 3:
                # incomplete data
                break
            if frm[0] != '\x01':
                state = self.UNSYNC
                break
            pktlen = ord(frm[1])
            if (pktlen+3) > frmlen:
                # incomplete data
                break
            if frm[pktlen+2] != '\x04':
                state = self.UNSYNC
                break
            packet,frm = frm[:pktlen+3],frm[pktlen+3:]

            ## XXX refactor this hack
            # convert frame from serial line to pcap format
            # u8   length
            # u64  ts
            # u8[] frm
            #
            # packet data is prefixed with byte STX (1) + length (1)
            # and suffixed with EOT char (4)
            fl = pktlen - 8
            ticks,pkt = packet[2:10], packet[10:-1]
            ticks = struct.unpack('=LL',ticks)
            tstamp = ((ticks[0]*self.ticknb) + ticks[1]) * self.tscale
            t_sec = int(tstamp)
            t_usec = int((tstamp-t_sec)*1.0e6)
            ts = struct.pack('=LL',(t_sec+self.timebase),t_usec)
            lenfield = struct.pack("=LL",fl,fl)
            packet = ts + lenfield + pkt
            if self.FCNT < self.maxpackets or self.maxpackets < 0:
                self.RxQueue.put(packet)
                self.FCNT += 1
                self.message(1,"Found Packet l=%d qsize: %d:\npkt: %s",
                        pktlen, self.RxQueue.qsize(),
                        " ".join(map(lambda s: '%02x' %s,map(ord,packet))))
            else:
                self.message(1, "Discard Packet l=%d qsize: %d",
                                 pktlen, self.RxQueue.qsize())
        return state,frm

    def set_channel(self, channel):
        if channel in self.clist:
            self.channel = channel
            time.sleep(0.1) # this sleep is somehow needed for rzusb stick.
            self.sport.write("\nchan %d\n" % channel)
        else:
            sys.stderr.write("Unsupported channel %d not in %s" % (channel,self.clist))

    def set_rate(self, rate):
        if rate in self.rates:
            self.rate = rate
            self.sport.write("\ndrate %s\n" % self.rate)
        else:
            sys.stderr.write("Unsupported data rate %s not in %s" % (rate, self.rates))

    def set_sfd(self, sfd = None):
        if sfd != None:
            self.sfd = sfd
            self.sport.write("\nsfd %s\n" % self.sfd)
        else:
            sys.stderr.write("SFD change is not supported")

    def read_packet(self):
        if self.RxQueue.empty():
            ret = None
            # preventing high CPU load:
            #  if queue is empty, make break and give other processes a chance.
            time.sleep(.1)
        else:
            ret = self.RxQueue.get()
        return ret

# === init ====================================================================
