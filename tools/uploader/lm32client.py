#!/usr/bin/env python
# -*- coding: utf-8 -*-
import random
import sys
import serial
import cgitb
import os
import time
import threading
import termios

EXITCHARCTER = '\x1d'   # GS/CTRL+]
MENUCHARACTER = '\x14'  # Menu: CTRL+T


def key_description(character):
    """generate a readable description for a key"""
    ascii_code = ord(character)
    if ascii_code < 32:
        return 'Ctrl+%c' % (ord('@') + ascii_code)
    else:
        return repr(character)

class Console:
    def __init__(self):
        self.fd = sys.stdin.fileno()

    def setup(self):
        self.old = termios.tcgetattr(self.fd)
        new = termios.tcgetattr(self.fd)
        new[3] = new[3] & ~termios.ICANON & ~termios.ECHO & ~termios.ISIG
        new[6][termios.VMIN] = 1
        new[6][termios.VTIME] = 0
        termios.tcsetattr(self.fd, termios.TCSANOW, new)

    def getkey(self):
        c = os.read(self.fd, 1)
        return c

    def cleanup(self):
        termios.tcsetattr(self.fd, termios.TCSAFLUSH, self.old)

console = Console()

def cleanup_console():
    console.cleanup()

console.setup()
sys.exitfunc = cleanup_console      #terminal modes have to be restored on exit...


CONVERT_CRLF = 2
CONVERT_CR   = 1
CONVERT_LF   = 0
NEWLINE_CONVERISON_MAP = ('\n', '\r', '\r\n')
LF_MODES = ('LF', 'CR', 'CR/LF')

REPR_MODES = ('raw', 'some control', 'all control', 'hex')

class Miniterm:
    def __init__(self, port, baudrate, parity, rtscts, xonxoff, echo=False, convert_outgoing=CONVERT_CRLF, repr_mode=0):
        try:
            self.serial = serial.serial_for_url(port, baudrate, parity=parity, rtscts=rtscts, xonxoff=xonxoff, timeout=1)
        except AttributeError:
            # happens when the installed pyserial is older than 2.5. use the
            # Serial class directly then.
            self.serial = serial.Serial(port, baudrate, parity=parity, rtscts=rtscts, xonxoff=xonxoff, timeout=1)
        self.echo = echo
        self.repr_mode = repr_mode
        self.convert_outgoing = convert_outgoing
        self.newline = NEWLINE_CONVERISON_MAP[self.convert_outgoing]
        self.dtr_state = True
        self.rts_state = True
        self.break_state = False

    def start(self):
        self.alive = True
        # start serial->console thread
        self.receiver_thread = threading.Thread(target=self.reader)
        self.receiver_thread.setDaemon(1)
        self.receiver_thread.start()
        # enter console->serial loop
        self.transmitter_thread = threading.Thread(target=self.writer)
        self.transmitter_thread.setDaemon(1)
        self.transmitter_thread.start()

    def stop(self):
        self.alive = False

    def join(self, transmit_only=False):
        self.transmitter_thread.join()
        if not transmit_only:
            self.receiver_thread.join()

    def dump_port_settings(self):
        sys.stderr.write("\n--- Settings: %s  %s,%s,%s,%s\n" % (
            self.serial.portstr,
            self.serial.baudrate,
            self.serial.bytesize,
            self.serial.parity,
            self.serial.stopbits,
        ))
        sys.stderr.write('--- RTS %s\n' % (self.rts_state and 'active' or 'inactive'))
        sys.stderr.write('--- DTR %s\n' % (self.dtr_state and 'active' or 'inactive'))
        sys.stderr.write('--- BREAK %s\n' % (self.break_state and 'active' or 'inactive'))
        sys.stderr.write('--- software flow control %s\n' % (self.serial.xonxoff and 'active' or 'inactive'))
        sys.stderr.write('--- hardware flow control %s\n' % (self.serial.rtscts and 'active' or 'inactive'))
        sys.stderr.write('--- data escaping: %s\n' % (REPR_MODES[self.repr_mode],))
        sys.stderr.write('--- linefeed: %s\n' % (LF_MODES[self.convert_outgoing],))
        try:
            sys.stderr.write('--- CTS: %s  DSR: %s  RI: %s  CD: %s\n' % (
                (self.serial.getCTS() and 'active' or 'inactive'),
                (self.serial.getDSR() and 'active' or 'inactive'),
                (self.serial.getRI() and 'active' or 'inactive'),
                (self.serial.getCD() and 'active' or 'inactive'),
                ))
        except serial.SerialException:
            # on RFC 2217 ports it can happen to no modem state notification was
            # yet received. ignore this error.
            pass

    def reader(self):
        """loop and copy serial->console"""
        try:
            while self.alive:
                data = self.serial.read(1)

                if self.repr_mode == 0:
                    # direct output, just have to care about newline setting
                    if data == '\r' and self.convert_outgoing == CONVERT_CR:
                        sys.stdout.write('\n')
                    else:
                        sys.stdout.write(data)
                elif self.repr_mode == 1:
                    # escape non-printable, let pass newlines
                    if self.convert_outgoing == CONVERT_CRLF and data in '\r\n':
                        if data == '\n':
                            sys.stdout.write('\n')
                        elif data == '\r':
                            pass
                    elif data == '\n' and self.convert_outgoing == CONVERT_LF:
                        sys.stdout.write('\n')
                    elif data == '\r' and self.convert_outgoing == CONVERT_CR:
                        sys.stdout.write('\n')
                    else:
                        sys.stdout.write(repr(data)[1:-1])
                elif self.repr_mode == 2:
                    # escape all non-printable, including newline
                    sys.stdout.write(repr(data)[1:-1])
                elif self.repr_mode == 3:
                    # escape everything (hexdump)
                    for character in data:
                        sys.stdout.write("%s " % character.encode('hex'))
                sys.stdout.flush()
        except serial.SerialException, e:
            self.alive = False
            # would be nice if the console reader could be interruptted at this
            # point...
            raise


    def writer(self):
        """loop and copy console->serial until EXITCHARCTER character is
           found. when MENUCHARACTER is found, interpret the next key
           locally.
        """
        menu_active = False
        try:
            while self.alive:
                try:
                    c = console.getkey()
                except KeyboardInterrupt:
                    c = '\x03'
                if menu_active:
                    if c == MENUCHARACTER or c == EXITCHARCTER: # Menu character again/exit char -> send itself
                        self.serial.write(c)                    # send character
                        if self.echo:
                            sys.stdout.write(c)
                    elif c == '\x15':                       # CTRL+U -> upload file
                        sys.stderr.write('\n--- File to upload: ')
                        sys.stderr.flush()
                        console.cleanup()
                        filename = sys.stdin.readline().rstrip('\r\n')
                        if filename:
                            try:
                                file = open(filename, 'r')
                                sys.stderr.write('--- Sending file %s ---\n' % filename)
                                while True:
                                    line = file.readline().rstrip('\r\n')
                                    if not line:
                                        break
                                    self.serial.write(line)
                                    self.serial.write('\r\n')
                                    # Wait for output buffer to drain.
                                    self.serial.flush()
                                    sys.stderr.write('.')   # Progress indicator.
                                sys.stderr.write('\n--- File %s sent ---\n' % filename)
                            except IOError, e:
                                sys.stderr.write('--- ERROR opening file %s: %s ---\n' % (filename, e))
                        console.setup()
                    elif c in '\x08hH?':                    # CTRL+H, h, H, ? -> Show help
                        sys.stderr.write(get_help_text())
                    elif c == '\x12':                       # CTRL+R -> Toggle RTS
                        self.rts_state = not self.rts_state
                        self.serial.setRTS(self.rts_state)
                        sys.stderr.write('--- RTS %s ---\n' % (self.rts_state and 'active' or 'inactive'))
                    elif c == '\x04':                       # CTRL+D -> Toggle DTR
                        self.dtr_state = not self.dtr_state
                        self.serial.setDTR(self.dtr_state)
                        sys.stderr.write('--- DTR %s ---\n' % (self.dtr_state and 'active' or 'inactive'))
                    elif c == '\x02':                       # CTRL+B -> toggle BREAK condition
                        self.break_state = not self.break_state
                        self.serial.setBreak(self.break_state)
                        sys.stderr.write('--- BREAK %s ---\n' % (self.break_state and 'active' or 'inactive'))
                    elif c == '\x05':                       # CTRL+E -> toggle local echo
                        self.echo = not self.echo
                        sys.stderr.write('--- local echo %s ---\n' % (self.echo and 'active' or 'inactive'))
                    elif c == '\x09':                       # CTRL+I -> info
                        self.dump_port_settings()
                    elif c == '\x01':                       # CTRL+A -> cycle escape mode
                        self.repr_mode += 1
                        if self.repr_mode > 3:
                            self.repr_mode = 0
                        sys.stderr.write('--- escape data: %s ---\n' % (
                            REPR_MODES[self.repr_mode],
                        ))
                    elif c == '\x0c':                       # CTRL+L -> cycle linefeed mode
                        self.convert_outgoing += 1
                        if self.convert_outgoing > 2:
                            self.convert_outgoing = 0
                        self.newline = NEWLINE_CONVERISON_MAP[self.convert_outgoing]
                        sys.stderr.write('--- line feed %s ---\n' % (
                            LF_MODES[self.convert_outgoing],
                        ))
                    #~ elif c in 'pP':                         # P -> change port XXX reader thread would exit
                    elif c in 'bB':                         # B -> change baudrate
                        sys.stderr.write('\n--- Baudrate: ')
                        sys.stderr.flush()
                        console.cleanup()
                        backup = self.serial.baudrate
                        try:
                            self.serial.baudrate = int(sys.stdin.readline().strip())
                        except ValueError, e:
                            sys.stderr.write('--- ERROR setting baudrate: %s ---\n' % (e,))
                            self.serial.baudrate = backup
                        else:
                            self.dump_port_settings()
                        console.setup()
                    elif c == '8':                          # 8 -> change to 8 bits
                        self.serial.bytesize = serial.EIGHTBITS
                        self.dump_port_settings()
                    elif c == '7':                          # 7 -> change to 8 bits
                        self.serial.bytesize = serial.SEVENBITS
                        self.dump_port_settings()
                    elif c in 'eE':                         # E -> change to even parity
                        self.serial.parity = serial.PARITY_EVEN
                        self.dump_port_settings()
                    elif c in 'oO':                         # O -> change to odd parity
                        self.serial.parity = serial.PARITY_ODD
                        self.dump_port_settings()
                    elif c in 'mM':                         # M -> change to mark parity
                        self.serial.parity = serial.PARITY_MARK
                        self.dump_port_settings()
                    elif c in 'sS':                         # S -> change to space parity
                        self.serial.parity = serial.PARITY_SPACE
                        self.dump_port_settings()
                    elif c in 'nN':                         # N -> change to no parity
                        self.serial.parity = serial.PARITY_NONE
                        self.dump_port_settings()
                    elif c == '1':                          # 1 -> change to 1 stop bits
                        self.serial.stopbits = serial.STOPBITS_ONE
                        self.dump_port_settings()
                    elif c == '2':                          # 2 -> change to 2 stop bits
                        self.serial.stopbits = serial.STOPBITS_TWO
                        self.dump_port_settings()
                    elif c == '3':                          # 3 -> change to 1.5 stop bits
                        self.serial.stopbits = serial.STOPBITS_ONE_POINT_FIVE
                        self.dump_port_settings()
                    elif c in 'xX':                         # X -> change software flow control
                        self.serial.xonxoff = (c == 'X')
                        self.dump_port_settings()
                    elif c in 'rR':                         # R -> change hardware flow control
                        self.serial.rtscts = (c == 'R')
                        self.dump_port_settings()
                    else:
                        sys.stderr.write('--- unknown menu character %s --\n' % key_description(c))
                    menu_active = False
                elif c == MENUCHARACTER: # next char will be for menu
                    menu_active = True
                elif c == EXITCHARCTER: 
                    self.stop()
                    break                                   # exit app
                elif c == '\n':
                    self.serial.write(self.newline)         # send newline character(s)
                    if self.echo:
                        sys.stdout.write(c)                 # local echo is a real newline in any case
                        sys.stdout.flush()
                else:
                    self.serial.write(c)                    # send character
                    if self.echo:
                        sys.stdout.write(c)
                        sys.stdout.flush()
        except:
            self.alive = False
            raise

def die(msg):
    print msg
    sys.exit(-1)

def binary(v,bits=8):
    r = "b"
    for i in range(bits-1,-1,-1):
        s = 1<<i
        if  v & s:
            r +="1"
        else:
            r +="0"
    return r

class LM32Serial(object):

    BOOT_SIG = "**soc-lm32/bootloader**"

    def __init__(self, dev, speed):
        self.io = serial.Serial(dev, speed)
        self.debug = False

    def close(self):
        self.io.close()

    def progress(self):
        sys.stdout.write(".")
        sys.stdout.flush()

    def info(self,msg):
        sys.stdout.write(msg)
        sys.stdout.flush()
    
    def put_uint32(self,i):
        self.io.write( chr((i >> 24) & 0xff ))
        self.io.write( chr((i >> 16) & 0xff ))
        self.io.write( chr((i >>  8) & 0xff ))
        self.io.write( chr((i >>  0) & 0xff ))

    def put_uint8(self,i):
        self.io.write( chr((i) & 0xff ))
   
    def get_uint8(self):
        return ord(self.io.read(1))


    def upload(self, addr, data):
        if self.debug:
            self.info("upload 0x%08x (%i)\n" % (addr,len(data)))
        self.io.write('u')
        self.put_uint32(addr)
        self.put_uint32(len(data))
        for v in data:
            self.io.write(v)

    def upload_chunked(self, data, addr, size, block_size):
        self.info("Uploading 0x%X (%i kb) to 0x%X..." % (size, size/1024, addr))
        for i in range((size / block_size)):
            self.progress()
            offset = i * block_size
            block = data[ offset : (offset + block_size) ]
            self.upload( addr + i * block_size, block )
        self.info("Done.\n")

    def download(self, addr, size):
        if self.debug:
            self.info("download 0x%08x (%i)\n" % (addr,size))
        r = []
        self.io.write('d')
        self.put_uint32(addr)
        self.put_uint32(size)
        for i in range(size):
            r.append(self.io.read(1))
        return r
    
    def download_chunked(self, addr, size, block_size):
        self.info("Download 0x%X (%i kb) from 0x%X..." % (size, size/1024, addr))
        data = []
        for i in range((size / block_size)):
            self.progress()
            data += self.download( addr + i*block_size, block_size )
        self.info("Done.\n")
        return data

    def jump(self,addr):
        self.info("Jump to 0x%X...\n" % (addr))
        self.io.write('g')
        self.put_uint32(addr)
        
    def find_bootloader(self, max_tries = 32):
        self.info("Looking for soc-lm32 bootloader")
        count = 0;
        while True:
            self.progress()
            count = count + 1
            if count == max_tries:
                die("Bootloader %s not not found" % LM32Serial.BOOT_SIG)
            self.io.write('\r')
            line = self.io.readline()
            if line and LM32Serial.BOOT_SIG in line:
                self.info("found.\n")
                break

def memcheck(options):
    
    block_size = int(options.block_size,16)
    test_size  = int(options.size,16)
    test_base  = int(options.start_addr,16)
    
    print "Memcheck Addr=0x%X size=0x%X " % (test_base,test_size)
    
    try:
        lm32 = LM32Serial(options.port, options.baudrate)
    except:
        die("Can't open serial port")

    lm32.find_bootloader()
    
    data = []
    for x in range(test_size):
        data.append(chr(random.randint(0,255)))
    
    lm32.upload_chunked(data, test_base, test_size, block_size)
    read_data = lm32.download_chunked( test_base, test_size, block_size )
    
    print "Checking for memory errors...",
    for idx,val in enumerate(data):
        if val != read_data[idx]:
            print "0x%X 0x%02x != 0x%02x" % (test_base + idx,ord(val),ord(read_data[idx]))
    print "Done."

def upload(options):
    
    block_size = 0x800
    try:
        lm32 = LM32Serial(options.port, options.baudrate)
    except:
        die("Can't open serial port")
    lm32.find_bootloader()
    addr_jump = none
    data = open(options.filename_srec).read().splitlines()
    for line in data:
        line = line.strip()
        if line.startswith("s7"):
            addr_jump = int(line[4:12],16)
        if line.startswith("s3"):
            count = int(line[2:4],16)
            addr  = int(line[4:12],16)
            dat   = line[12:-2]
            cksum = int(line[-2:],16)
            count = (count - 5) * 2
            data = []
            for i in range(0,count,2):
                data.append(chr(int(dat[i:i+2],16)))
            lm32.upload(addr,data)    
    lm32.jump(addr_jump)
    lm32.close()

def jump(options):
    lm32 = LM32Serial(options.port, options.baudrate)
    addr = int(options.start_addr,16)
    lm32.jump(addr)
    lm32.close()


def lac(options):
    

    try:
        select = int(options.lac_select,16)
        trigger = int(options.lac_trigger,16)
        triggermask = int(options.lac_triggermask,16)
        timescale = options.lac_timescale
    except:
        die("Need values for SELECT TRIGGER TRIGGERMASK")
    
    try:
        fd = open(options.filename_vcd,"w")
    except:
        die("Can't open output file %s" % options.filename_vcd)

    try:
        lm32 = lm32serial(options.port, options.baudrate)
    except:
        die("Can't open serial port")
    
    # Write VCD header
    fd.write("$date") 
    fd.write("\t%i" % int(time.time()))
    fd.write("$end")
    fd.write("$version") 
    fd.write("\tLogicAnalyzerComponent (http://www.das-labor.org/)")
    fd.write("$end")
    fd.write("$timescale")
    fd.write("\t%s" % timescale)
    fd.write("$end")

    # Declare wires
    fd.write("$scope module lac $end")
    fd.write("$var wire 8 P probe[7:0] $end")
    fd.write("$enddefinitions $end")
    
    print "Using 0x%02x 0x02% 0x%02x" % ( select, trigger, triggermask)
    # CMD_DISARM
    for i in range(0,6):
        lm32.put_uint8(0x00)
    
    # CMD_ARM
    lm32.put_uint8(0x01)
    lm32.put_uint8(select)
    lm32.put_uint8(trigger)
    lm32.put_uint8(triggermask)
    lm32.put_uint8(0x00)

    print "LAC armed; waiting for trigger condition..."

    size = lm32.get_uint8()
    size = 1 << size;

    print "TRIGGERED -- Reading 0x%x bytes..." % size
    for i in range(0,size):
        c = lm32.get_uint8()
        fd.write("\#%i" % (i +1))
        fd.write("%s P\n" % binary(c))

    lm32.close()
    fd.close()
    fsize = os.stat(options.filename_vcd).st_size
    print "Done %s %s Kb" % (options.filename_vcd, fsize / 1024)

def mterm(options):

    try:
        miniterm = Miniterm(
            options.port,
            options.baudrate,
            "N",
            False,
            False
        )
    except serial.SerialException, e:
        sys.stderr.write("could not open port %r: %s\n" % (port, e))
        sys.exit(1)

    sys.stderr.write('--- Miniterm on %s: %d,%s,%s,%s ---\n' % (
        miniterm.serial.portstr,
        miniterm.serial.baudrate,
        miniterm.serial.bytesize,
        miniterm.serial.parity,
        miniterm.serial.stopbits,
    ))
    sys.stderr.write('--- Quit: %s  |  Menu: %s | Help: %s followed by %s ---\n' % (
        key_description(EXITCHARCTER),
        key_description(MENUCHARACTER),
        key_description(MENUCHARACTER),
        key_description('\x08'),
    ))

    miniterm.start()
    miniterm.join(True)

def debugger(options):
    fd = open("remote.gdb","w")
    fd.write("target remote /dev/ttyUSB0\n")
    fd.close()
    cmd = "cgdb -d lm32-elf-gdb  -x remote.gdb %s" % options.filename_elf
    print "Execute: %s" % cmd
    os.system(cmd)
    if os.path.isfile("remote.gdb"):
        os.unlink("remote.gdb")
        
def main():
    import optparse

    parser = optparse.OptionParser(
        usage = "%prog [options]",
        description = "lm32clieant - A simple client to upload images"
    )

    parser.add_option("-d", "--device",
        dest = "port",
        help = "Set device (Default: %default)",
        default = "/dev/ttyUSB0"
    )

    parser.add_option("-b", "--baud",
        dest = "baudrate",
        action = "store",
        type = 'int',
        help = "set baud rate, default %default",
        default = 115200
    )

    parser.add_option("-f","--filename",
        dest = "filename_srec",
        action = "store",
        help = "Set srec image filename for serial upload",
        default = ''
    )

    parser.add_option("-a", "--action",
        dest = "action",
        action = "store",
        help = "Select mode [memcheck,lac,upload,jump])",
        default = None
    )

    parser.add_option("-s", "--start",
        dest = "start_addr",
        action = "store",
        help = "Set start addr for jumps (Default: %default)",
        default = "0x40000000"
    )
    
    parser.add_option("-S", "--size",
        dest = "size",
        action = "store",
        help = "Set size for memchecks (Default: %default)",
        default = "0x8000"
    )

    parser.add_option("-B", "--blocksize",
        dest = "block_size",
        action = "store",
        help = "Set block size for memchecks (Default: %default)",
        default = "0x800"
    )

    parser.add_option("-m", "--miniterm",
        dest = "miniterm",
        action = "store_true",
        help = "Start miniterm after action",
        default = False
    )

    parser.add_option("-D", "--debugger",
        dest = "debugger",
        action = "store_true",
        help = "Start debugger cgdb and open a remote serial session",
        default = False
    )
    
    parser.add_option("-e", "--elf",
        dest = "filename_elf",
        action = "store",
        help = "Set elf filename for debugger",
        default = False
    )

    parser.add_option("-v", "--vcd",
        dest = "filename_vcd",
        action = "store",
        help = "Set vcd filename for the la (Default: %default)",
        default = "trace.vcd"
    )

    parser.add_option("-t", "--timescale",
        dest = "lac_timescale",
        action = "store",
        help = "Timescale announced in .vcd file (Default: %default)",
        default = "10ns"
    )

    parser.add_option("", "--select",
        dest = "lac_select",
        action = "store",
        help = "Select probe value",
        default = None
    )

    parser.add_option("", "--trigger",
        dest = "lac_trigger",
        action = "store",
        help = "Select trigger value",
        default = None
    )

    parser.add_option("", "--triggermask",
        dest = "lac_triggermask",
        action = "store",
        help = "Select trigger mask value",
        default = None
    )

    (options, args) = parser.parse_args()
    if options.action =='jump':
        jump(options)
    elif options.action =='memcheck':
        memcheck(options)
    elif options.action =='lac':
        if options.filename_vcd is None:
            parser.error("Need to specify a .vcd filename")
        lac(options)
    elif options.action =='upload':
        if options.filename_srec is None:
            parser.error("Need to specify a .srec filename")
        if not os.path.isfile(options.filename_srec):
            parser.error("Can't access  srec file %s" % options.filename_srec)
        upload(options)
    if options.miniterm:
        mterm(options)
    
    if options.debugger:
        if options.filename_elf is None:
            parser.error("Need to specify elf filename")
        if not os.path.isfile(options.filename_elf):
            parser.error("Can't access elf file %s" % options.filename_elf)
        debugger(options)

if __name__ == '__main__':
    main()


