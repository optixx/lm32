
OBJ = spike_hw.o main.o mmc.o ff.o 

LM32_CC=lm32-elf-gcc
LM32_LD=lm32-elf-ld
LM32_OBJCOPY=lm32-elf-objcopy
LM32_OBJDUMP=lm32-elf-objdump

SREC2VRAM ?= ../../tools/srec2vram/srec2vram

VRAMFILE=image.ram

CPU_FLAGS=-mbarrel-shift-enabled -mmultiply-enabled -msign-extend-enabled \
          -mdivide-enabled

CFLAGSL=-MMD -Os -Wall -g -s -fomit-frame-pointer $(CPU_FLAGS)
CFLAGSZ=-MMD -Os -Wall -g -s -fomit-frame-pointer $(CPU_FLAGS)

LLDFLAGS=-nostdlib -nodefaultlibs -Tlinker.ld 
SEGMENTS = -j .text -j .rodata -j .data -j .bss

all: $(VRAMFILE) image.bin

crt0ram.o: crt0ram.S
	$(LM32_CC) $(CFLAGS) -c crt0ram.S

main.o: main.c
	$(LM32_CC) $(CFLAGSZ) -c $<
	
spike_hw.o: spike_hw.c 
	$(LM32_CC) $(CFLAGSZ) -c $<
	
mmc.o: mmc.c
	$(LM32_CC) $(CFLAGSZ) -c $<
	
ff.o: ff.c
	$(LM32_CC) $(CFLAGSZ) -c $<

image.elf: crt0ram.o $(OBJ)
	$(LM32_LD) $(LLDFLAGS) -Map image.map -N -o image.elf crt0ram.o $(OBJ)

image.lst: image.elf
	$(LM32_OBJDUMP) -h -S $< > $@

image.bin: image.elf
	$(LM32_OBJCOPY) $(SEGMENTS) -O binary image.elf image.bin
	wc -c image.bin

image.srec: image.elf image.lst
	$(LM32_OBJCOPY) $(SEGMENTS) -O srec image.elf image.srec

$(VRAMFILE): image.srec
	$(SREC2VRAM) image.srec 0x00000000 0x2000 > $(VRAMFILE)

clean:
	rm -f compress convert image image.lst image.bin image.srec image.map image.ram image.hex *.o *.d

DEPS := $(wildcard *.d)
ifneq ($(DEPS),)
include $(DEPS)
endif

