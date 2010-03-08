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

int main(int argc, char **argv)
{
	int8_t  *p;
	uint8_t  c;

	// Initialize UART
	uart_init();

	c = '*'; // print msg on first iteration
	for(;;) {
		uint32_t start, size, i;
    	uint32_t *mem_start, *mem_end, *mem_p; 
		switch (c) {
    		case 'u': // upload 
    			start = read_uint32();
    			size  = read_uint32();
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
    			jump(start);
    			break;   
        	case '1': // test
            	uart_putstr( "Memory Dump32: " );
            	mem_start = (uint32_t *)0x40000000;
            	mem_end   = (uint32_t *)0x40000080;
                *mem_start = 'A' ;
            	for (mem_p=mem_start; mem_p<mem_end; mem_p++) {
            		if (((uint32_t)mem_p & 12) == 0) {
            			uart_putstr("\r\n[");
            			uart_puthex32((uint32_t) mem_p);
            			uart_putchar(']');    
            		}
    		        uart_putchar(' ');    
            		uart_puthex32(*mem_p);
            	}
    			uart_putstr("\r\n");
        		break;   
            case '2': // test
            	uart_putstr( "Memory Dump8: " );
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
        		for (mem_p=mem_start; mem_p<mem_end; mem_p++)
                    *mem_p = 0x00000000;
                uart_putstr("\r\nCleared Test Mem\r\n");
            	break;   
            case '5': 
            	uart_putstr( "GPIO Test 1...\r\n" );
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
            	uart_putstr( "GPIO Test 2...\r\n" );
            	gpio0->oe = 0x000000ff;
        		for(i=0; i<0xffff; i++) {
                	gpio0->out = i;
                    uart_puthex32(gpio0->out);
                	uart_putstr( "\r\n" );
        			sleep(50);
        		}
                break;
        
			default:
				uart_putstr("**soc-lm32/bootloader** > \r\n");
				break;
		};

		c = uart_getchar();

	}
}

