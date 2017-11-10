#   Copyright (c) 2008 Axel Wachtler
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
# @brief basic communication class
#
# === import ==================================================================
import sys, traceback, struct

# === globals =================================================================

# === functions ===============================================================

# === classes =================================================================
##
# @brief programm context storage class
class ProgramContext:
    ## name of the file socket, overwritten by -i option
    SOCKNAME='ieee802154'
    ## name of the capture file, overwritten by -r option
    CAPFILE=None
    ## name of the capture port, overwritten by -p option
    PORT=None
    ## Channel number for sniffing.
    CHANNEL = None
    ## Verbosity Level.
    VERBOSE = 0
    ## Maximum Packets to be captured.
    MAXPACKETS = -1
    ## Start control GUI.
    GUI = False
    # Data Rate
    RATE = None

##
# Generic reporting and exception handler class
class UtilBase:
    TMO = 1
    VERBOSE = 0
    ## reporting an error to stderr.
    def error(self,msg,*args):
        sys.stderr.write("ERROR:"+msg%args+"\n")
        sys.stderr.flush()

    ## regular message
    def message(self,lvl,msg,*args):
        if self.VERBOSE >= lvl:
            prompt = "+"*(lvl)
            msg = msg%args
            msg = msg.replace("\n","\n> ")
            print prompt,msg
            sys.stdout.flush()

    ## configure verbosity level
    def setverbose(self,verbose):
        self.VERBOSE = verbose

    ## custom exception printer
    def exc_handler(self, diag):
        print sys.exc_info()
        self.message(0,diag)
        if self.VERBOSE>0:
            traceback.print_exc()


##
# @brief Base class for @ref PcapFile and @ref PcapPort
class PcapBase:
    ## Cstruct format string for pcap packet header
    PCAP_PACKET="=QLL"
    ## Cstruct format string for pcap file header
    # for details see http://wiki.wireshark.org/Development/LibpcapFileFormat
    #
    # typedef struct pcap_hdr_s {
    #   guint32 magic_number;   /* magic number */
    #   guint16 version_major;  /* major version number */
    #   guint16 version_minor;  /* minor version number */
    #   gint32  thiszone;       /* GMT to local correction */
    #   guint32 sigfigs;        /* accuracy of timestamps */
    #   guint32 snaplen;        /* max length of captured packets, in octets */
    #   guint32 network;        /* data link type */
    # } pcap_hdr_t;
    #
    PCAP_HEADER="=LHHLLLL"
    ##
    # create pcap header file
    def pcap_get_header(self):
        magic_number = 0xa1b2c3d4 #<1> L
        version_major = 2         #<2> H
        version_minor = 4         #<3> H
        thiszone = 0              #<4> L
        sigfigs = 0               #<5> L
        snaplen = 0x80            #<6> L maximum length of 802.15.4 packet incl. PHR (length field)
        network = 0xc3            #<7> L
        ret = struct.pack(self.PCAP_HEADER,
                     magic_number, version_major, version_minor,
                     thiszone, sigfigs,snaplen, network)
        return ret

    ## parse the entire data of a capture file.
    # @retval hdr   the header structure of the file
    # @retval ret   a list containing the packets as raw strings
    def pcap_parse_data(self,data):
        ret = []
        hsz = struct.calcsize(self.PCAP_HEADER)
        psz = struct.calcsize(self.PCAP_PACKET)
        o = hsz
        hdr, data = data[:o], data[o:]
        while(len(data) > psz):
            o = psz
            ph, data = data[:o], data[o:]
            ts,cl,fc = struct.unpack(self.PCAP_PACKET , ph)
            if cl != fc:
                self.message(1,"MISMATCH cl=%x fc=%x", cl, fc)
            o = cl
            frm, data = data[:o], data[o:]
            ret.append(ph+frm)
        return hdr,ret


# === init ====================================================================
