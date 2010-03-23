/**
 * Primitive first stage bootloader 
 *
 *
 */
#include "soc-hw.h"

/* prototypes */

uint32_t read_uint32()
{
	uint32_t val = 0, i;

    for (i = 0; i < 4; i++) {
        val <<= 8;
        val += (uint8_t)uart_getchar();
    }

    return val;
}

#define     TEST_PATTERN        0x41424142

void display_addr(int addr){
    gpio0->out = addr;
}

void display_int(int val){
    gpio0->out = val;
}

int main(int argc, char **argv)
{
	int8_t  *p;
	uint8_t  c;
	// Initialize UART
	uart_init();

    gpio0->oe = 0x000000ff;
    gpio0->out = 0x0000;
	
    c = '*'; // print msg on first iteration
	for(;;) {
		uint32_t start, size, i;
    	uint32_t *mem_start, *mem_end, *mem_p; 
		switch (c) {
    		case 'u': // upload 
    			start = read_uint32();
    			size  = read_uint32();
                display_addr(start);
    			for (p = (int8_t *) start; p < (int8_t *) (start+size); p++) {
    				*p = uart_getchar();
    			}
    			break;
			case 'd': // download
    			start = read_uint32();
    			size  = read_uint32();
    			for (p = (int8_t *) start; p < (int8_t *) (start+size); p++) {
					uart_putchar( *p );
    			}
    			break;
    		case 'g': // goto
    			start = read_uint32();
    			display_int(0xdead);
                jump(start);

    			break;   
        	case '1': // test
            	uart_putstr( "Test Pattern 32bit access" );
            	for (i=0; i < 16777216 / sizeof(uint32_t); i+=1024){ 
                    mem_start = (uint32_t *)0x40000000 + i;
                    mem_end   = (uint32_t *)0x4000000a + i;
                    *mem_start = TEST_PATTERN;
                    mem_p = mem_start;
                    *(uint32_t*)++mem_p = 0x40000000 + i;
                    *(uint32_t*)++mem_p = i;
                    for (mem_p=mem_start; mem_p<mem_end; mem_p++) {
                        if (((uint32_t)mem_p & 15) == 0) {
                            uart_putstr("[");
                            uart_puthex32((uint32_t) mem_p);
                            uart_putchar(']');    
                        }
                        uart_putchar(' ');    
                        uart_puthex32(*mem_p);
                    }
                    uart_putstr("\r\n");
                }
        		break;   
            case '2': // test
            	uart_putstr( "Memory dump 8bit");
            	mem_start = (uint32_t *)0x40000000;
            	mem_end   = (uint32_t *)0x40000080;
                *mem_start = 'A' ;
            	for (p=(uint8_t*)mem_start;  p<(uint8_t*)mem_end; p++) {
            		if (((uint32_t)p & 15 ) == 0) {
            			uart_putstr("\r\n[");
            			uart_puthex32((uint32_t) p);
            			uart_putchar(']');    
            		}
    		        uart_putchar(' ');    
            		uart_puthex8(*p);
            	}
    			uart_putstr("\r\n");
        		break;   
            case '3': 
            	uart_putstr("Writing testpattern...\r\n");
                uint32_t* p = (uint32_t*)0x40000000;
                for(i = 0; i < 0x1000/sizeof(uint32_t); i++)
                {
                    *p = TEST_PATTERN;
                    p++;
            	}
            	uart_putstr("Dumping testpattern...\r\n");
            	uint8_t* q = (uint8_t*)0x40000000;
                for(i = 0; i < 0x1000; i++)
                {
                    uart_putchar( *q );
                    q++;
                }
                uart_putstr("\r\ndone!\r\n");
            	break;   
            case '4': 
                uart_putstr("\r\nCleare memory from 0x40000000 - 0x41000000 \r\n");
            	mem_start = (uint32_t *)0x40000000;
            	mem_end   = (uint32_t *)0x41000000;
        		for (mem_p=mem_start; mem_p<mem_end; mem_p++){
                    *mem_p = 0x00;
            		if ((uint32_t)mem_p % (1024 * 1024) == 0) {
                        uart_putstr("\r0x");
            			uart_puthex32((uint32_t)mem_p);
            		}
                }
                uart_putstr("\r\nDone\r\n");
            	break;   
            case '5': 
            	uart_putstr( "GPIO test Kight Rider\r\n" );
            	gpio0->oe = 0x000000ff;
        		for(i=0; i<8; i++) {
        			uint32_t out1, out2;
        			out1 = 0x01 << i;
        			out2 = 0x80 >> i;
        			gpio0->out = out1 | out2;
                    uart_puthex32(gpio0->out);
                	uart_putstr( "\r\n" );
        			sleep(100);
        		}
                break;
            case '6': 
            	uart_putstr( "GPIO test seven segment counter\r\n" );
            	gpio0->oe = 0x000000ff;
        		for(i=0; i<0xff; i++) {
                	gpio0->out = i;
        			sleep(20);
        		}
                break;
        
			default:
				uart_putstr("**soc-lm32/bootloader** > \r\n");
				break;
		};

		c = uart_getchar();

	}
}

