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

#define TEST_SIZE 0x1000
#define TEST_STARTADDRESS 0x40000000
#define TEST_PATTERN 0x41424142

int main(int argc, char **argv)
{
	// Initialize UART
	int i;
    uart_init();

	uart_putstr("Writing testpattern...\r\n");
	uint32_t* p = (uint32_t*)TEST_STARTADDRESS;
	for(i = 0; i < 0x1000/sizeof(uint32_t); i++)
	{
		*p = TEST_PATTERN;
		p++;
	}

	uart_putstr("Dumping testpattern...\r\n");
	uint8_t* q = (uint8_t*)TEST_STARTADDRESS;
	for(i = 0; i < 0x1000; i++)
	{
		uart_putchar( *q );
		q++;
	}
	uart_putstr("\r\ndone!\r\n");
    while(1);
}

