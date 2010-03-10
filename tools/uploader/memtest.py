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
        
    def find_bootloader(self, max_tries = 32):
        self.info("Looking for soc-lm32 bootloader")
        count = 0;
        while True:
            self.progress()
            count = count + 1
            if count == max_tries:
                die("Bootloader %s not not found" % BOOT_SIG)
            self.io.write('\r')
            line = self.io.readline()
            if line and LM32Serial.BOOT_SIG in line:
                self.info("found.\n")
                break

def main():
    BLOCK_SIZE = 0x800
    TEST_SIZE  = 0x8000
    TEST_BASE  = 0x40000000
    
    lm32 = LM32Serial()
    lm32.find_bootloader()
    
    data = []
    for x in range(TEST_SIZE):
        data.append(chr(random.randint(0,255)))
    
    lm32.upload_chunked(data, TEST_BASE, TEST_SIZE, BLOCK_SIZE)
    read_data = lm32.download_chunked( TEST_BASE, TEST_SIZE, BLOCK_SIZE )
    
    print "Checking for memory errors...",
    for idx,val in enumerate(data):
        if idx%BLOCK_SIZE==0:
            progress()
        if val != read_data[idx]:
            print "0x%X 0x%02x 0x%02x" % (TEST_BASE + idx,ord(val),ord(read_data[idx]))
    print "Done."

if __name__ == '__main__':
    main()


