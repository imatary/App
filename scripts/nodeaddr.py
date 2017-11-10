# $Id$
# This file is generated automatically from wibo_gen_py.tpl
#
"""
Dumps a intel hex file for a node config structure.

    python nodeaddr.py [OPTIONS]

    Options:
     -a <shortaddr>
        Fixed node short source address (16 bit value).
     -A <longaddr>
        Fixed node short source address (64 bit value).
     -p <panid>
        Default pan id (16 bit value).
     -c <channel>
        A Channel hint for applications.

     -C <cfgfile>
        The configuration file that describes the network settings.
        Using such a config file reduces the average command line length by
        several meters :-) Use -G option to generate a commented template file.
        Note that command line parameters overwrite the settings in the config
        file (default: nodeaddr.cfg).
     -G
        Generate a initial config file with comments (see option -C).

     -B <boardname>
        create the record at flash-end for the p['mmcu']
        of the given p['board']NAME (see also -O option)
     -M <mmcu>
        give the name of the p['mmcu'] (instead of -B)
     -O <offset>
        per default the address of the config record is flashend.
        With -O an explicit offset can be specified.

     -f <hexfile>
        Name of IHEX file to be used as firmware, e.g. wuart_<board>.hex
        where <board> is replaced by the current value.
     -b <bootloader>
        Name of IHEX file to be used as bootloader, e.g. wibo_<board>.hex
        where <board> is replaced by the current value.
     -o <newhexfile>
        Name of the outputfile, if '-' stdout is used.
        e.g. "foo_<board>_<saddr>.hex", where <board> and <saddr> are replaced
        by the current values.

     -h
        Display help and exit.
     -v
        Show version and exit.
     -l
        List board names and exit
     -L
        List p['mmcu'] names and exit

Example:
  - Create the config file nodeaddr.cfg:
     python nodeaddr.py -G

  - Create the config file my_nodeaddr.cfg:
     python nodeaddr.py -C my_nodeaddr.cfg -G

  - After editing the file nodeaddr.cfg use:
     python nodeaddr.py -a1
    in order to create a file for node 1.

  - Add an address record to a hexfile and flash it to the target.
     python nodeaddr.py -b rdk230 -f wibo_rdk230.hex -a 42 -p 1 > a42.hex
     avrdude -P usb -p m1281 -c jtag2 -U a42.hex

  - Pipe the generated hexfile directly into avrdude.
     python nodeaddr.py -b rdk230 -f xmpl_leds_rdk230.hex -a 42 -p 1 |\\
            avrdude -P usb -p m1281 -c jtag2 -U fl:w:-:i
  - Flash the record w/o erasing
    python nodeaddr.py -a 1 | avrdude -P usb -p m1281 -c jtag2 -V -D -U fl:w:-:i


   Writes source address 1 into the device via a pipe to avrdude.

"""

# === import ==================================================================
import struct, getopt, sys, ConfigParser, os
try:
    import Tkinter
except:
    pass

# === globals =================================================================
VERSION = "20161012"

# default parameters
DEFAULT_PARAMS = \
    dict(infile = None,
         bootloader = None,
         outfile = None,
         board = None,
         offset = None,
         panid = 0xb5c2,
         saddr = 0x8000,
         laddr = 0x0,
         channel = -1)


# contents of the config records
CFGFILE = "nodeaddr.cfg"

# Tables
MMCU_TABLE = {'any2400': 'atmega1281',
 'any2400st': 'atmega1281',
 'any900': 'atmega1281',
 'any900st': 'atmega1281',
 'atrcb256rfr2xpro': 'atmega256rfr2',
 'atzb256rfr2xpro': 'atmega256rfr2',
 'atzbx212bxpro': 'atxmega256a3',
 'atzbx233usb': 'atxmega256a3u',
 'atzbx233xpro': 'atxmega256a3',
 'bat': 'atmega128rfa1',
 'bitbean': 'atmega1281',
 'cbb212': 'atxmega256a3',
 'cbb230': 'atxmega256a3',
 'cbb230b': 'atxmega256a3',
 'cbb231': 'atxmega256a3',
 'cbb232': 'atxmega256a3',
 'cbb233': 'atxmega256a3',
 'derfa1': 'atmega128rfa1',
 'derfn128': 'atmega128rfa1',
 'derfn128u0': 'atmega128rfa1',
 'derfn256u0': 'atmega256rfr2',
 'derfn256u0pa': 'atmega256rfr2',
 'derfn256u1': 'atmega256rfr2',
 'derfn256u1pa': 'atmega256rfr2',
 'derftorcbrfa1': 'atmega128rfa1',
 'dracula': 'atmega128rfa1',
 'ibdt212': 'atmega644p',
 'ibdt231': 'atmega644',
 'ibdt232': 'atmega644p',
 'icm230_11': 'atmega1281',
 'icm230_12a': 'atmega1281',
 'icm230_12b': 'atmega1281',
 'icm230_12c': 'atmega128',
 'ics230_11': 'atmega1281',
 'ics230_12': 'atmega128',
 'ict230': 'atmega1281',
 'im240a': 'atmega328',
 'im240a_eval': 'atmega328',
 'l3y': 'atmega2564rfr2',
 'lgee231': 'atmega88',
 'lgee231_v2': 'atmega88',
 'm256rfr2xpro': 'atmega256rfr2',
 'mnb900': 'atmega1281',
 'muse231': 'atmega88pa',
 'museII232': 'atmega328p',
 'museIIrfa': 'atmega128rfa1',
 'pinoccio': 'atmega256rfr2',
 'psk212': 'atmega1281',
 'psk230': 'atmega1281',
 'psk230b': 'atmega1281',
 'psk231': 'atmega1281',
 'psk232': 'atmega1281',
 'psk233': 'atmega1281',
 'radiofaro': 'atmega128rfa1',
 'radiofaro2': 'atmega256rfr2',
 'radiofaro_v1': 'atmega128rfa1',
 'raspbee': 'atmega256rfr2',
 'ravrf230a': 'atmega1284p',
 'ravrf230b': 'atmega1284p',
 'rbb128rfa1': 'atmega128rfa1',
 'rbb212': 'atmega1281',
 'rbb230': 'atmega1281',
 'rbb230b': 'atmega1281',
 'rbb231': 'atmega1281',
 'rbb232': 'atmega1281',
 'rbb233': 'atmega1281',
 'rdk212': 'atmega1281',
 'rdk230': 'atmega1281',
 'rdk230b': 'atmega1281',
 'rdk231': 'atmega1281',
 'rdk232': 'atmega1281',
 'rdk233': 'atmega1281',
 'rose231': 'atmega328p',
 'rzusb': 'at90usb1287',
 'sparcrfa1': 'atmega128rfa1',
 'stb128rfa1': 'atmega128rfa1',
 'stb212': 'atmega1281',
 'stb230': 'atmega1281',
 'stb230b': 'atmega1281',
 'stb231': 'atmega1281',
 'stb232': 'atmega1281',
 'stb233': 'atmega1281',
 'stb256rfr2': 'atmega256rfr2',
 'stkm16': 'atmega16',
 'stkm8': 'atmega8',
 'tiny230': 'attiny84',
 'tiny231': 'attiny84',
 'wdba1281': 'atmega1281',
 'wprog': 'atmega128rfa1',
 'xma1u233xpro': 'atxmega128a1u',
 'xme5rz212': 'atxmega32e5',
 'xme5rz230': 'atxmega32e5',
 'xme5rz231': 'atxmega32e5',
 'xxo': 'atmega128rfa1',
 'zgbh212': 'atmega1281',
 'zgbh230': 'atmega1281',
 'zgbh231': 'atmega1281',
 'zgbl212': 'atmega1281',
 'zgbl230': 'atmega1281',
 'zgbl231': 'atmega1281',
 'zgbt1281a2nouart': 'atmega1281',
 'zgbt1281a2uart0': 'atmega1281',
 'zgbt1281a2uart1': 'atmega1281',
 'zigduino': 'atmega128rfa1'}

# data structure for config file handling
CFGP = ConfigParser.ConfigParser()

# This a workaround for obtaining the flashends for the current used MCU's.
# avr-gcc is used to extract the address.
# for i in $(awk -F'=' '/^cpu/{print $2}' Src/Lib/Inc/boards/board.cfg|sort|uniq);do x=$(echo "#include <avr/io.h>"|avr-gcc -mmcu=$i -E -dM -|awk '/FLASHEND/{print  $NF}');echo "\"$i\" : $x,"; done
#
FLASHEND = {
    "at90usb1287" : 0x1FFFF,
    "atmega128" : 0x1FFFF,
    "atmega1281" : 0x1FFFF,
    "atmega1284p" : 0x1FFFF,
    "atmega128rfa1" : 0x1ffff,
    "atmega256rfr2" : 0x3FFFF,
    "atmega16" : 0x3FFF,
    "atmega328" : 0x7FFF,
    "atmega328p" : 0x7FFF,
    "atmega644" : 0xFFFF,
    "atmega8" : 0x1FFF,
    "atmega88" : 0x1FFF,
    "atmega88pa" : (0x1FFF),
    "attiny84" : 0x1FFF,
    "atxmega256a3" : 0x41FFF,
}

CFGTEMPLATE = """
# In the "groups" section several nodes can be configured to be in one group,
# e.g. 1,3,4,5 are ravenboards.
[group]
#ravengang=1,3:5

# In the "board" section the hardware targets are assigned to the short
# addresses.
[board]
default = rdk230
#ravengang = ravrf230a
#0 = stb230

# In the "channel" section the default radio channel for the board is assigned.
[channel]
default=17

[pan_id]
default=0xdeaf

[ieee_addr]
#1=12:34:56:78:9a:bc:de:f0

[firmware]
default = install/bin/wuart_<board>.hex
#0 = install/bin/wibohost_<board>.hex
outfile = /tmp/node_<saddr>.hex

[bootloader]
default = install/bin/wibo_<board>.hex

#[offset]
#ravengang=0x00
"""

# payload of record w/o crc
NODE_CONFIG_FMT ="<HHQB2x"


# === functions ===============================================================
##
# Format an intel hex record
# For a description of the intel hex format see:
#  http://en.wikipedia.org/wiki/Intel_HEX
#
# @param rtype
#        record type
#          00     Data Record
#          01     End of File Record
#          02     Extended Segment Address Record
#          03     Start Segment Address Record
#          04     Extended Linear Address Record
#          05     Start Linear Address Record
# @param addr
#           16 bit address value
# @param data
#           list with 8 bit values.
# @return string of the formated record.
#
def ihex_record(rtype, addr,data = []):

    dlen = len(data) & 0xff
    darr  = [ dlen,
             (addr >> 8) & 0xff,
             (addr & 0xff) ,
              rtype & 0xff]
    darr.extend(data)
    crc = 0
    for d in darr:
        crc += d
    crc = ((crc &0xff)^0xff) + 1
    darr.append(crc & 0xff)
    return ":"+"".join(["%02X" %d  for d in darr])

##
# Dallas ibutton crc8.
#
# This implementation is based on avr-libc
# _crc_ibutton_update(), see http://www.nongnu.org/avr-libc/
#
# @param data
#           array of numbers or raw binary string.
# @return The computed crc8 value.
#
# The Dallas iButton test vector must return 0
# ibutton_crc( [ 0x02, 0x1c, 0xb8, 0x01, 0, 0, 0, 0xa2 ] )
#
def ibutton_crc(data):
    if isinstance(data,str):
        idata = map(ord, data)
    else:
        idata = data
    crc =  0
    for d in idata:
        crc = crc ^ d
        for i in range(8):
            if crc & 0x01:
                crc = (crc >> 1) ^ 0x8C
            else:
                crc >>= 1
    return crc

def resolve_value(cfg, sect, addr_key, group_key = None):
    # cascade for retriving values
    curr_sect = cfg.get(sect, {})
    rv = curr_sect.get(addr_key,
            curr_sect.get(group_key,
                curr_sect.get("default",
                    DEFAULT_PARAMS.get(sect))))
    try:
        rv = eval(str(rv))
    except:
        pass
    return rv

def resolve_groups(cfg):
    rv = {}
    for k, v in cfg.get('group', {}).items():
        for a in v.split(","):
            tmp = map(eval, a.split(":"))
            if len(tmp) == 2:
                tmp2 = range(tmp[0],tmp[1]+1)
                tmp = tmp2
            for xx in tmp:
                rv[str(xx)] = k
    return rv

def resolve_parameters(**p):
    cfg = dict([(s,dict(CFGP.items(s))) for s in CFGP.sections()])
    group_keys =  resolve_groups(cfg)

    addr_key = "%d" % p['saddr']
    group_key = group_keys.get(addr_key)
    if p.get('panid') == None:
        p['panid'] = resolve_value(cfg, "panid", addr_key, group_key)
    if p.get('laddr') == None:
        p['laddr'] = resolve_value(cfg, "laddr", addr_key, group_key)
    if p.get('saddr') == None:
        p['saddr'] = resolve_value(cfg, "saddr", addr_key, group_key)
    if p.get('channel') == None:
        p['channel'] = resolve_value(cfg, "channel", addr_key, group_key)
    if p.get('board') == None:
        p['board'] = resolve_value(cfg, "board", addr_key, group_key)
    if p.get('offset') == None:
        p['offset'] = resolve_value(cfg, "offset", addr_key, group_key)
    if p.get('offset') == None:
        mmcu = MMCU_TABLE.get(p['board'],None)
        if mmcu:
            p['offset'] = get_flashend_offset_for_node_config(mmcu)
        else:
            print "Failure: can not determine offset for"\
                  " mmcu=%s, board = %s" % (mmcu, p['board'])
            sys.exit(2)

    if p.get('infile') == None:
        p['infile'] = resolve_value(cfg, "firmware", addr_key, group_key)
    if p.get('infile') == None:
        p['infile'] = resolve_value(cfg, "infile", addr_key, group_key)
    if p.get('infile') != None:
        p['infile'] = p['infile'].replace("<board>", p['board'])
        if not os.path.exists(p['infile']):
            print "Failure: firmware file '%s' not found" % p['infile']
            sys.exit(3)
    if p.get('bootloader') == None:
        p['bootloader'] = resolve_value(cfg, "bootloader", addr_key, group_key)
    if p.get('bootloader') != None:
        p['bootloader'] = p['bootloader'].replace("<board>", p['board'])
        if not os.path.exists(p['bootloader']):
            print "Failure: bootloader file '%s' not found" % p['bootloader']
            sys.exit(3)

    if p.get('outfile') == None:
        p['outfile'] = resolve_value(cfg, "firmware", "outfile")
    if p.get('outfile') == None or p['outfile'] == "-":
        p['outfile'] = sys.stdout
    else:
        ofn = p['outfile'].replace("<saddr>", "0x%04X" % p['saddr'])
        ofn = ofn.replace("<board>", str(p['board']))
        p['outfile'] = open(ofn ,"w")
    p['addr_key'] = addr_key
    p['group_key'] = group_key

    return p

def get_flashend_offset_for_node_config(mmcu):
    return (FLASHEND[mmcu] - struct.calcsize(NODE_CONFIG_FMT + "B") + 1)

##
# Generating the node config structure.
#
# The format of the structure node_config_t is defined in board.h.
# @param memaddr
#           address, where to locate the record
#           Per default memaddr=None and the location FLASHEND is
#           used.
def generate_nodecfg_record(memaddr, saddr, panid, laddr, channel):
    ret = []
    # payload of record w/o crc
    extaddr = (memaddr >> 16)
    if (extaddr > 0):
        # is extended addr record needed ?
        #  a  b    c  d    e
        # :02 0000 02 1000 EC
        data = map(ord, struct.pack("<H",extaddr<<4))
        ret.append(ihex_record(2, 0, data))
    data = map(ord,struct.pack(NODE_CONFIG_FMT, saddr, panid, laddr, channel))
    crc8 = ibutton_crc( data )
    data.append(crc8)
    ret.append(ihex_record(0, memaddr, data))
    return ret

##
# add a node cofg structure at the end of the flash.
#
def patch_hexfile(appfiles, outfile, offset, saddr, panid, laddr, channel):
    END_RECORD = ":00000001FF"
    fo = outfile
    for ifile in appfiles:
        if ifile != None:
            fi = open(ifile,"r")
            for l in fi.xreadlines():
                if l.find(END_RECORD) == 0:
                    # end record found
                    break
                # write regular record
                fo.write(l)
            fi.close()
    nodecfg = generate_nodecfg_record(offset, saddr, panid, laddr, channel)
    fo.write("\n".join(nodecfg)+"\n")
    fo.write(END_RECORD)
    fo.write("\n")
    if fo != sys.stdout:
        fo.close()

def list_boards():
    bl = MMCU_TABLE.keys()
    bl.sort()
    print "BOARD            MMCU                 FLASH-END"
    for b in bl:
        mmcu = MMCU_TABLE[b]
        print "%-16s %-20s 0x%x" % (b, mmcu, FLASHEND.get(mmcu,0))

def list_mmcus():
    mmcus = FLASHEND.keys()
    mmcus.sort()
    print "BOARD               FLASH-END"
    for mmcu in mmcus:
        print "%-20s 0x%x" % (mmcu, FLASHEND.get(mmcu,0))

##
# This function generates a patched hexfile.
#
# @param board  (mandatory) name of the board
# @param bootloader (optional) name of the bootloader hexfile, default: Nonde
# @param infile (optional) name of the application hexfile, default: Nonde
# @param channel (mandatory) default radio channel
# @param saddr (mandatory) 16 bit short address
# @param laddr (optional) long addr, default: 0
# @param panid (optional) 16 bit PAN ID, default: 0xb5c2
# @param outfile (optional) name of the generated hex file, default: stdout
#
# Example:
#
# @code
#  import nodeaddr
#  nodeaddr.nodeaddr(board = "wdba1281",
#                    bootloader = "install/bin/wibo_wdba1281.hex",
#                    channel = 17,
#                    saddr = 1,
#                    outfile = "node.hex")
# @endcode
def nodeaddr(**p):
    p = resolve_parameters(**p)
    patch_hexfile([p.get('infile'), p.get('bootloader')],
                  p.get('outfile'),
                  p['offset'],
                  p['saddr'], p['panid'], p['laddr'], p['channel'])
    return p
try:
    class EntryField(Tkinter.Frame):
        def __init__(self, master, text, textvariable=None):
            Tkinter.Frame.__init__(self, master=master)
            Tkinter.Label(master=self, text=text).pack(side=Tkinter.LEFT)
            Tkinter.Entry(master=self, textvariable=textvariable).pack(side=Tkinter.RIGHT)

    class Scrollbox(Tkinter.Frame):
        def __init__(self, master, *args, **kwargs):
            Tkinter.Frame.__init__(self, master, *args, **kwargs)
            self.bar=Tkinter.Scrollbar(master=self)
            self.bar.pack(side=Tkinter.RIGHT, fill=Tkinter.Y)
            self.box=Tkinter.Listbox(master=self, yscrollcommand=self.bar.set)
            self.box.pack(side=Tkinter.LEFT)
            self.bar.config(command=self.box.yview)


    class GUI(object):
        def __init__(self, parent):
            self.frame = Tkinter.Frame(parent)
            self.frame.pack()
            Tkinter.Label(master=self.frame, text=p['board']).pack()
            self.shortaddr = Tkinter.StringVar(value=str(hex(p['saddr']))) # start with default
            self.shortaddr.trace("w", self.cb_changed)
            self.longaddr = Tkinter.StringVar(value=str(hex(p['laddr'])))
            self.longaddr.trace("w", self.cb_changed)
            self.panid = Tkinter.StringVar(value=str(hex(p['panid'])))
            self.panid.trace("w", self.cb_changed)
            self.channel = Tkinter.StringVar(value=str(hex(p['channel'])))
            self.channel.trace("w", self.cb_changed)
            sb=Scrollbox(master=self.frame)
            sb.pack(side=Tkinter.LEFT)
            [sb.box.insert(Tkinter.END, i) for i in MMCU_TABLE.keys()]
            EntryField(master=self.frame, text="Short Addr", textvariable=self.shortaddr).pack()
            EntryField(master=self.frame, text="Long Addr", textvariable=self.longaddr).pack()
            EntryField(master=self.frame, text="PAN Id", textvariable=self.panid).pack()
            EntryField(master=self.frame, text="Channel", textvariable=self.channel).pack()
            Tkinter.Button(master=self.frame, text="Done", command=self.frame.quit).pack()

        def cb_changed(self, *args):
            p = {}
            p['saddr'] = int(self.shortaddr.get(), 16)
            p['laddr'] = int(self.longaddr.get(), 16)
            p['panid'] = int(self.panid.get(), 16)
            p['channel'] = int(self.channel.get(), 16)

    def call_gui():
        """ Pop up Tkinter GUI to enter parameters """
        root = Tkinter.Tk()
        app=GUI(root)
        root.mainloop()
except:
    def call_gui():
        msg = "=" * 80 + "\n"\
              "Sorry, there seems to be no Tkinter within your Python "\
              "installation.\n"\
              "Please install python-tk package for your OS or simply use the "\
              "command line.\n"\
              "Use python %s --help to see the available options.\n" +\
              "=" * 80 + "\n"
        print msg % sys.argv[0]
        sys.exit(1)


# === classes =================================================================

if __name__ == "__main__":
    try:
        opts,args = getopt.getopt(sys.argv[1:],"a:p:A:f:b:hvo:c:O:B:lLM:C:G")
    except:
        msg = "=" * 80 +\
              "\nInvalid arguments. Please try python %s -h.\n" +\
              "=" * 80
        print msg % sys.argv[0]
        opts = [('-v',''),]
        args = []

    if opts == []:
        call_gui()

    #test vectors
    #x = ":1001600060E070E000E010E020E030E00E94D705A1"
    #addr = 0x0160
    #data = [ 0x60, 0xE0, 0x70, 0xE0, 0x00, 0xE0, 0x10, 0xE0,
    #         0x20, 0xE0, 0x30, 0xE0, 0x0E, 0x94, 0xD7, 0x05 ]
    #y = ihex_record(0,addr, data)
    p = {}
    p.update(DEFAULT_PARAMS)

    for o,v in opts:
        if o == "-a":
            p['saddr'] = eval(v)
        elif o == "-A":
            p['laddr'] = eval(v)
        elif o == "-p":
            p['panid'] = eval(v)
        elif o == "-c":
            p['channel'] = eval(v)
        elif o == "-f":
            p['infile'] = v
        elif o == "-b":
            p['bootloader'] = v
        elif o == "-B":
            if MMCU_TABLE.has_key(v):
                p['board'] = v
                p['mmcu'] = MMCU_TABLE.get(p['board'],"?")
        elif o == "-M":
            if FLASHEND.has_key(v):
                p['board'] = "??"
                p['mmcu'] = v
        elif o == "-C":
            CFGFILE = v
        elif o == "-G":
            if not os.path.isfile(CFGFILE):
                f = open(CFGFILE, "w")
                f.write(CFGTEMPLATE)
                f.close()
                print "generated file %s" % f.name
            else:
                print "file %s does already exist" % CFGFILE
            doexit = True
        elif o == "-o":
            p['outfile'] = v
        elif o == "-O":
            p['offset'] = eval(v)
        elif o == "-h":
            print __doc__
            p = None
        elif o == "-v":
            print "Version",VERSION
            p = None
        elif o == "-l":
            list_boards()
            p = None
        elif o == "-L":
            list_mmcus()
            p = None

    if p == None:
        sys.exit(0)

    if os.path.isfile(CFGFILE):
        CFGP.read(CFGFILE)

    p = nodeaddr(**p)
    p['ofn'] = p['outfile'].name
    sys.stderr.write(\
        "Use Parameters:\n"\
        " board:      %(board)s\n"\
        " addr_key:   %(addr_key)s\n"\
        " group_key:  %(group_key)s\n"\
        " short_addr: 0x%(saddr)04x\n"\
        " pan_id:     0x%(panid)04x\n"\
        " ieee_addr:  0x%(laddr)016x\n"\
        " channel:    %(channel)d\n"\
        " offset:     0x%(offset)08x\n"\
        " infile:     %(infile)s\n"\
        " bootloader: %(bootloader)s\n"\
        " outfile:    %(ofn)s\n\n"\
        % p)
    try:
        import readline, rlcompleter
        readline.parse_and_bind("tab:complete")
    except:
        pass
