#!/usr/bin/env python
# -*- coding: utf-8 -*-
import random
import sys
import serial
import cgitb

def die(msg):
    print msg
    sys.exit(-1)

def progress():
    sys.stdout.write(".")
    sys.stdout.flush()

class LM32Serial(object):

    SPEED = 115200
    DEV = "/dev/ttyUSB0"
    BOOT_SIG = "**soc-lm32/bootloader**"

    def __init__(self):
        self.io = serial.Serial(LM32Serial.DEV, LM32Serial.SPEED)
        self.debug = False

    def put_uint32(self,i):
        self.io.write( chr((i >> 24) & 0xff ))
        self.io.write( chr((i >> 16) & 0xff ))
        self.io.write( chr((i >>  8) & 0xff ))
        self.io.write( chr((i >>  0) & 0xff ))

    def upload(self, addr, data):
        if self.debug:
            print "upload 0x%08x (%i)" % (addr,len(data))
        self.io.write('u')
        self.put_uint32(addr)
        self.put_uint32(len(data))
        for v in data:
            self.io.write(v)


    def download(self, addr, size):
        if self.debug:
            print "download 0x%08x (%i)" % (addr,size)
        r = []
        self.io.write('d')
        self.put_uint32(addr)
        self.put_uint32(size)
        for i in range(size):
            r.append(self.io.read(1))
        return r

    def find_bootloader(self, max_tries = 32):
        count = 0;
        while True:
            progress()
            sys.stdout.flush()
            count = count + 1
            if count == max_tries:
                die("Bootloader %s not not found" % BOOT_SIG)
            self.io.write('\r')
            line = self.io.readline()
            if line and LM32Serial.BOOT_SIG in line:
                break

def main():
    BLOCK_SIZE = 0x800
    TEST_SIZE  = 0x8000
    TEST_BASE  = 0x40000000
    print "Looking for soc-lm32 bootloader",
    sys.stdout.flush()
    lm32 = LM32Serial()
    lm32.find_bootloader()
    print "found."
    data = []
    for x in range(TEST_SIZE):
        data.append(chr(random.randint(0,255)))
    print "Uploading 0x%X (%i kb) random bytes to 0x%X..." % (TEST_SIZE, TEST_SIZE/1024,TEST_BASE),
    for i in range((TEST_SIZE / BLOCK_SIZE)):
        progress()
        offset = i*BLOCK_SIZE
        block = data[ offset : (offset+BLOCK_SIZE) ]
        lm32.upload( TEST_BASE + i*BLOCK_SIZE, block )
    print "Done."

    print "Downloading again...",
    read_data = []
    for i in range((TEST_SIZE / BLOCK_SIZE)):
        progress()
        read_data += lm32.download( TEST_BASE + i*BLOCK_SIZE, BLOCK_SIZE )
    print "Done."
    print "Checking for memory errors...",
    for idx,val in enumerate(data):
        if idx%BLOCK_SIZE==0:
            progress()
        if val != read_data[idx]:
            print "0x%X 0x%02x 0x%02x" % (TEST_BASE + idx,ord(val),ord(read_data[idx]))
    print "Done."

if __name__ == '__main__':
    main()


