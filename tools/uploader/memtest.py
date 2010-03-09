#!/usr/bin/env python
# -*- coding: utf-8 -*-
import random
import sys
import serial

def die(msg):
    print msg
    sys.exit(-1)

class LM32Serial(object):

    SPEED = 115200
    DEV = "/dev/ttyUSB0"
    BOOT_SIG = "**soc-lm32/bootloader**"
    BOOT_SIG = "xxx"

    def __init__(self):
        self.io = serial.Serial(LM32Serial.DEV, LM32Serial.SPEED)

    def put_uint32(self,i):
        self.io.write( (i >> 24) & 0xff )
        self.io.write( (i >> 16) & 0xff )
        self.io.write( (i >>  8) & 0xff )
        self.io.write( (i >>  0) & 0xff )

    def upload(self, addr, data):
        print "upload 0x%08x (%i)" % (addr,len(data))
        self.io.write('u')
        self.put_uint32(addr)
        self.put_uint32(len(data))
        for v in data:
            self.io.write(v)


    def download(self, addr, size):
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
            count = count + 1
            if count == max_tries:
                die("Bootloader %s not not found" % BOOT_SIG)
            self.io.write('\r')
            line = self.io.readline()
            if line and BOOT_SIG in line:
                break

def main():
    BLOCK_SIZE = 0x100
    TEST_SIZE  = 0x200
    TEST_BASE  = 0x40000000
    print "Looking for soc-lm32 bootloader...",
    lm32 = LM32Serial()
    lm32.find_bootloader()
    print "found."
    data = []
    for x in range(TEST_SIZE):
        data.append(random.randint(0,256))
    print "Uploading 0x%X random bytes to 0x%X..." % (TEST_SIZE, TEST_BASE),
    for i in range((TEST_SIZE / BLOCK_SIZE)):
        print ".",
        offset = i*BLOCK_SIZE
        block = data[ offset : (offset+BLOCK_SIZE) ]
        lm32.upload( TEST_BASE + i*BLOCK_SIZE, block )
        print "Done"

    print "Downloading again...",
    read_data = []
    for i in range((TEST_SIZE / BLOCK_SIZE)):
        print "."
        read_data += lm32.download( TEST_BASE + i*BLOCK_SIZE, BLOCK_SIZE )
    print "done."
    print "Checking for memory errors...",
    for idx,val in enumerate(data):
        if val != read_data[i]:
            print "0x%X 0x%02x 0x%02x" % (TEST_BASE + idx,val,read_data[idx])


if __name__ == '__main__':
    main()


