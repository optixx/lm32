#include "spike_hw.h"

uart_t   *uart0  = (uart_t *)   0xf0000000;
timer_t  *timer0 = (timer_t *)  0xf0010000;
gpio_t   *gpio0  = (gpio_t *)   0xF0020000;
spi_t    *spi0   = (spi_t *)    0xF0030000;

isr_ptr_t isr_table[32];


void tic_isr();
/***************************************************************************
 * IRQ handling
***************************************************************************/

void isr_null()
{
}

void irq_handler(uint32_t pending)
{
	int i;

	for(i=0; i<32; i++) {
		if (pending & 0x01) (*isr_table[i])();
		pending >>= 1;
	}
}

void isr_init()
{
	int i;
	for(i=0; i<32; i++)
		isr_table[i] = &isr_null;
}

void isr_register(int irq, isr_ptr_t isr)
{
	isr_table[irq] = isr;
}

void isr_unregister(int irq)
{
	isr_table[irq] = &isr_null;
}


/***************************************************************************
 * TIMER Functions
***************************************************************************/

void msleep(uint32_t msec)
{
	uint32_t tcr;

	// Use timer0.1
	timer0->compare1 = (FCPU/1000)*msec;
	timer0->counter1 = 0;
	timer0->tcr1 = TIMER_EN;

	do {
		//halt();
 		tcr = timer0->tcr1;
 	} while ( ! (tcr & TIMER_TRIG) );
}

void nsleep(uint32_t nsec)
{
	uint32_t tcr;

	// Use timer0.1
	timer0->compare1 = (FCPU/1000000)*nsec;
	timer0->counter1 = 0;
	timer0->tcr1 = TIMER_EN;

	do {
		//halt();
 		tcr = timer0->tcr1;
 	} while ( ! (tcr & TIMER_TRIG) );
}


uint32_t tic_msec;

void tic_isr()
{
	tic_msec++;
	timer0->tcr0     = TIMER_EN | TIMER_AR | TIMER_IRQEN;
}

void tic_init()
{
	tic_msec = 0;

	// Setup timer0.0
	timer0->compare0 = (FCPU/10000);
	timer0->counter0 = 0;
	timer0->tcr0     = TIMER_EN | TIMER_AR | TIMER_IRQEN;

	isr_register(1, &tic_isr);
}


/***************************************************************************
 * UART Functions
 */
void uart_init()
{
	//uart0->ier = 0x00;  // Interrupt Enable Register
	//uart0->lcr = 0x03;  // Line Control Register:    8N1
	//uart0->mcr = 0x00;  // Modem Control Register

	// Setup Divisor register (Fclk / Baud)
	//uart0->div = (FCPU/(57600*16));
}

char uart_getchar()
{   
	while (! (uart0->ucr & UART_DR)) ;
	return uart0->rxtx;
}

void uart_putchar(char c)
{
	while (uart0->ucr & UART_BUSY) ;
	uart0->rxtx = c;
}

void uart_putstr(char *str)
{
	char *c = str;
	while(*c) {
		uart_putchar(*c);
		c++;
	}
}

uint32_t readint(uint8_t nibbles, uint8_t* checksum) {
	uint32_t val = 0, i;
    uint8_t c;
    for (i = 0; i < nibbles; i++) {
        val <<= 4;
        c = uart_getchar();
        if (c <= '9')
    	   val |= (c - '0') & 0xf;
        else
    	   val |= (c - 'A' + 0xa) & 0xf; 
    	if (i & 1)
    	   *checksum += val;      
    }
    return val;
}

void writeint(uint8_t nibbles, uint32_t val)
{
	uint32_t i, digit;

	for (i=0; i<8; i++) {
	    if (i >= 8-nibbles) {
    		digit = (val & 0xf0000000) >> 28;
    		if (digit >= 0xA) 
      			uart_putchar('A'+digit-10);
      		else
    			uart_putchar('0'+digit);
	    }
		val <<= 4;
	}
}

void uart_putint8(unsigned char c)
{
	uart_putchar(c + ((c < 10) ? '0' : 'A' - 10));
}

void uart_puthex8(char c)
{
    uart_putint8(c >> 4);
    uart_putint8(c & 15);
}

void uart_puthex32(int i)
{
    uart_puthex8((i >> 24) & 0xff);
    uart_puthex8((i >> 16) & 0xff);
    uart_puthex8((i >>  8) & 0xff);
    uart_puthex8((i >>  0) & 0xff);
}



