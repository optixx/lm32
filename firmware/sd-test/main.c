/**
 * Bootloader for soc-lm32
 */

#include <stddef.h>
#include "spike_hw.h"
#include "ff.h"

#include "diskio.h"

void* memset (void *str, int c, size_t len)
{
  register char *st = str;

  while (len-- > 0)
    *st++ = c;
  return str;
}

void *memcpy (void *destaddr, void const *srcaddr, size_t len)
{
  char *dest = destaddr;
  char const *src = srcaddr;

  while (len-- > 0)
    *dest++ = *src++;
  return destaddr;
}

int
memcmp(const void* ab1, const void* ab2, size_t n)
{
	register const unsigned char*	b1 = (const unsigned char*)ab1;
	register const unsigned char*	b2 = (const unsigned char*)ab2;
	register const unsigned char*	e = b1 + n;

	while (b1 < e)
		if (*b1++ != *b2++)
			return(*--b1 - *--b2);
	return(0);
}

void memtest()
{
	volatile uint32_t *p;

	uart_putstr("\r\nMEMTEST...");

	for (p=(uint32_t *)RAM_START; p<(uint32_t *)(RAM_START+1024*(512 - 16)); p++) {
		*p = (uint32_t) p;  
	}
	
	uart_putstr("...");

	for (p=(uint32_t *)RAM_START; p<(uint32_t *)(RAM_START+1024*(512 - 16)); p++) {
		if (*p != (uint32_t)p) {
			uart_putstr("\r\nMEMTEST ERROR: ");
			writeint(8,(uint32_t)p);
		}
	}
	uart_putstr("OK\n\r");
}


FATFS fs;				/* File system object */
FIL fil;

uint8_t read_buffer[512 + 128];

int main()
{
	WORD fsize;
    WORD len;
	FRESULT fresult;
	int8_t  *p;
	uint32_t i;
	
	isr_init();
	tic_init();
	
    msleep(500);
        uart_putstr("Bootloader init\n\r"); 
	
    memset(&fs, 0, sizeof(FATFS)); 	/* Clear file system object */
	FatFs = &fs;	                /* Assign it to the FatFs module */	

	uart_putstr("Looking for sys/firmware.bin\n\r"); 
	const char firmware[] = "sys/firmware.bin";
 
	fresult = f_open(&fil, firmware,  FA_READ|FA_OPEN_EXISTING);
	uart_putstr("Got fresult\n\r"); 
		
	if(fresult){
		switch(fresult){
			case FR_NO_FILE:
			case FR_NO_PATH:
				uart_putstr(firmware);
				uart_putstr(" not found.\n\r");
				break;
			case FR_NOT_READY:
				uart_putstr("no card found.\n\r");
				break;
			case FR_NO_FILESYSTEM:
				uart_putstr("no FAT-FS\n\r");
				break;
		}
		goto uartmode;
	}

	uart_putstr("found file size: 0x");
	writeint(8, fil.fsize);
	uart_putstr("\n\r");
    len = fil.fsize; 
    for (i = 0; i < len; i += 512)	{ 
		f_read (&fil, read_buffer , 512, &fsize);
		dump_packet(i,512,read_buffer);
	}
	uart_putstr("Done");
    /* 
    for (i = 0; i < fil.fsize && i < 1024*(512 - 16); i += 64*1024 - 1)	{ 
		f_read (&fil, (uint8_t*) (0x40000000+i), 64*1024 - 1, &fsize);
		uart_putstr("\nread bytes: 0x");
		writeint(8, i+fsize);
	}
	jump(0x40000000);
    */
uartmode: 
	uart_putstr("\r\n** SPIKE BOOTLOADER **\n");
	for(;;) {
		uint32_t start, size, checksum, help;
		uart_putchar('>');
		uint32_t c = uart_getchar();

		switch (c) {
    		case 'r': // reset
    			jump(0x00000000);
    			break;

    		case 's': // start 
    			jump(0x4007C000);
    			break;
                
			case 'm': // memtest
				memtest();
				break;

    		case 'u': // Upload programm
      			checksum = 0;
      			// read size 
    			size  = readint(2, (uint8_t *) &checksum);
    			size -= 5;
      			// read start
    			start = readint(8, (uint8_t *) &checksum);
    			for (p = (int8_t *) start; p < (int8_t *) (start+size); p++) {
    				*p = readint(2, (uint8_t *) &checksum);
    			}
    			writeint(2, ~checksum);
    			break;
    			
    		case 'g': // go
    			start = readint(8, (uint8_t *) &checksum);
    			jump(start);
    			break;   
    		}
	}

	//while (1);
}

