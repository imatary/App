#   Copyright (c) 2015 Axel Wachtler, Daniel Thiele
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
"""
Usage:

    python %(scriptname)s [OPTIONS] CFG_FILE (list of commands)

Options:

    -C CFGFILE, --config CFGFILE
        setup config file, default: hbm251.cfg
    -B, --board
        handle a dedicated board, must exists in config file
    -h, --help
        show help and exit
    -v, --version
        show version and exit
    -n, --dry-run
        dry-run, just show commands, do not execute anything

Commands:

    build
        Build all boards described in setup config
    flash APP
        Flash the given application to all boards of setup config
        APP is evaluated as follows:
          [1] is it an existing file
          [2] is it an uracoli type application, like wuart, rdiag, rsensor
    test
        Exec py.test Regression

Example:

    python %(scriptname)s -C myenv.cfg build flash wuart

Config File Example:

    [hbm251.py]
    bindir = <direct>
    boardcfg = <board-config>
    buildcmd = <firmware-compile-command>

    [target1]
    board = <boardname>
    reset = <board-reset-command>
    flwrt = <board-flash-command>
    flvfy = <board-verify-command>
    fltmo = <time-to-wait-after-flash>

    [target2]
    ...
"""
# === import ==================================================================
from __future__ import print_function
import sys
import os
from ConfigParser import RawConfigParser
import subprocess
import time
import getopt
import pprint

#=== globals ===================================================================
VERSION = "1.0"
SETUPCFG = 'hbm251.cfg'
BOARDCFG = 'Config/board.cfg'
BINDIR = 'install/bin'

#=== classes ===================================================================
def execute(cmd, verbose = 0, verbose_onerror = 1):
    proc = subprocess.Popen( cmd,
                             shell = True,
                             stdin = subprocess.PIPE,
                             stderr = subprocess.PIPE,
                             stdout = subprocess.PIPE)
    stdout, stderr = proc.communicate()
    if verbose > 0 or (verbose_onerror and proc.returncode != 0):
        print("# %s" % cmd)
    if verbose > 1 or (verbose_onerror and proc.returncode != 0):
        if len(stdout):
            print(": stdout :\n: ", stdout.replace("\n", "\n: "))
        if len(stderr):
            print(": stderr :\n: ", stderr.replace("\n", "\n: "))
    return proc.returncode, stdout, stderr

class Board(dict):
    """ Dictionary as result of enviroment.cfg and board.cfg """
    def __repr__(self):
        return "%(name)s:%(board)s@%(port)s:%(baudrate)s"%self

    def reset(self, dryrun = 0, verbose = 0):
        cmd_args = {}
        cmd_args.update(self)
        rst_cmd = self["reset"] % cmd_args
        fl_tmo = float(self.get("fltmo", "0"))
        if dryrun:
            print(rst_cmd)
            ret = 0
        else:
            if fl_tmo:
                time.sleep(fl_tmo)
            ret, so, se = execute(rst_cmd, verbose)
        return ret

    def find_app(self, app):
        if not os.path.exists(app) or not os.path.isfile(app):
            fname = "%s_%s.hex" % tuple(map(str,[app, self.__getitem__('board')]))
            fw = os.path.join(self.__getitem__('bindir'), fname)
            do_exist = os.path.exists(fw) and os.path.isfile(fw)
            if not do_exist:
                return None
        else:
            fw = app
        return fw

    def flash(self, hexfile, dryrun = 0, verbose = 0):
        cmd_args = {"fw": hexfile, "afw": os.path.abspath(hexfile)}
        cmd_args.update(self)
        vfy_cmd = self.get("flvfy", "echo undefined FLVFY for %(device)s:%(board)s %(fw)s") % cmd_args
        wrt_cmd = self.get("flwrt", "echo undefined FLWRT for %(device)s:%(board)s %(fw)s") % cmd_args
        fl_tmo = float(self.get("fltmo", "0"))
        if dryrun:
            print(vfy_cmd)
            print(wrt_cmd)
            wrt_fail = 0
        else:
            wrt_fail = 1
            vfy_fail = 1
            if len(vfy_cmd):
                if fl_tmo:
                    time.sleep(fl_tmo)
                vfy_fail, so, se = execute(vfy_cmd, verbose, 0)
                if vfy_fail == 0:
                    wrt_fail = 0
            if vfy_fail and len(wrt_cmd):
                # wait before accessing avrdude a second time
                if fl_tmo:
                    time.sleep(fl_tmo)
                wrt_fail, so, se = execute(wrt_cmd, verbose, 1)
        return wrt_fail
##
# This class is a collection of boards, read from the setup config file.
#
class HBM251(object):
    def __init__(self, scfg, bcfg, bindir, boardfilter=None):
        assert os.path.exists(scfg)
        # do not check bcfg, it might be optional

        # retrieve setup data from setup.cfg
        cfg_setup = RawConfigParser()
        cfg_setup.read(scfg)
        setup = {}
        for sec in cfg_setup.sections():
            d = dict(cfg_setup.items(sec))
            # skip all sections that have not a "board" key.
            if 'board' in d:
                d['name'] = sec
                setup[sec] = d

        self.buildcmds = ["scons %(targets)s"]

        # if board config is invalid till here, try to get it from setupcfg
        if not os.path.exists(str(bcfg)):
            bcfg = None

        # read config section of the tool
        if cfg_setup.has_section("hbm251.py"):
            for k, v in cfg_setup.items("hbm251.py"):
                if k == "bindir":
                    # overwrite function parameter to be used later
                    bindir = v
                elif k == "boardcfg" and bcfg == None:
                    bcfg = v
                elif k == "buildcmd":
                    self.buildcmds = v.split("\n")

        # get a flat board dictionary
        cfg_board = RawConfigParser()
        if os.path.exists(bcfg):
            cfg_board.read(bcfg)
        boards = {}
        for b,v in [(s, dict(cfg_board.items(s))) for s in cfg_board.sections()]:
            boards[b] = v
            for a in v.get("aliases","").split():
                boards[a.strip()] = v

        # update setup with needed data from board.cfg
        self.boards=[]
        for b,v in setup.items():
            # update the board from board.cfg with the section
            # from config-file, this ensures priorization of
            # config-file parameters overwriting board parameters
            brd = boards[v['board']]
            brd.update(v)
            brd.update({'bindir':bindir})
            if boardfilter==None or v['board'] in boardfilter or b in boardfilter:
                self.boards.append(Board(brd))

    def build(self, dryrun = False, verbose = 0):
        targets = ' '.join(set([b['board'] for b in self.boards]))
        for c in self.buildcmds:
            cmd = c % {"targets" : targets}
            if dryrun:
                print(cmd)
            else:
                ret, so, se = execute(cmd, verbose)

    def reset(self, dryrun = False, verbose = 0):
        for b in self.boards:
            assert b.reset(dryrun, verbose) == 0

    def flash(self, app, dryrun = False, verbose = 0):
        boards = [b for b in self.boards]
        for b in boards:
            fwname = b.find_app(app)
            assert os.path.exists(fwname), "firmware file %s does not exist" % fwname
            b.flash(fwname, dryrun, verbose)

    def __repr__(self):
        return '\n'.join([b.__repr__() for b in self.boards])

# === main =====================================================================
if __name__ == '__main__':
    doexit = False

    cmdargs = {'dryrun':False, 'verbose':0}
    boardfilter = []
    opts, cmds = getopt.getopt(sys.argv[1:], "C:B:nhvV",
                ["config=", "board=", "help", "verbose", "version", "dry-run"])
    for o,v in opts:
        if o in ('-C', '--config'):
            SETUPCFG = v
        elif o in ('-B', '--board'):
            boardfilter.append(v)
        elif o in ('-n', '--dry-run'):
            cmdargs['dryrun'] = True
        elif o in ('-v', '--verbose'):
            cmdargs['verbose'] += 1
        elif o in ('-h', '--help'):
            print(__doc__ % {"scriptname" : os.path.basename(sys.argv[0])})
            doexit = True
        elif o in ('-V', '--version'):
            print(sys.argv[0], VERSION)
            doexit = True

    if doexit:
        sys.exit(0)

    print("Reading", os.path.abspath(SETUPCFG))
    if len(boardfilter) == 0: boardfilter=None
    hbm = HBM251(SETUPCFG, BOARDCFG, BINDIR, boardfilter=boardfilter)
    print(hbm)

    idx = 0
    while idx < len(cmds):
        cmd = cmds[idx]
        if cmd.startswith("build"):
            hbm.build(**cmdargs)
        elif cmd.startswith("flash"):
            try:
                idx += 1
                appname = cmds[idx]
            except:
                appname = None
            assert hbm.flash(appname, **cmdargs) != 0
        elif cmd.startswith('reset'):
            assert hbm.reset(**cmdargs) != 0
        elif cmd.startswith('test'):
            print("test", cmd, idx)
            print("Not supported yet")
        else:
            print("Unknown command given, skipping", cmd)
        idx += 1

# EOF
