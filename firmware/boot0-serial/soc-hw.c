#include "soc-hw.h"

uart_t   *uart0  = (uart_t *)   0xF0000000;
timer_t  *timer0 = (timer_t *)  0xF0010000;
gpio_t   *gpio0  = (gpio_t *)   0xF0020000;
// uint32_t *sram0  = (uint32_t *) 0x40000000;

uint32_t msec = 0;

/***************************************************************************
 * General utility functions
 */
void sleep(int msec)
{
	uint32_t tcr;

	// Use timer0.1
	timer0->compare1 = (FCPU/1000)*msec;
	timer0->counter1 = 0;
	timer0->tcr1 = TIMER_EN | TIMER_IRQEN;

	do {
		//halt();
 		tcr = timer0->tcr1;
 	} while ( ! (tcr & TIMER_TRIG) );
}

void tic_init()
{
	// Setup timer0.0
	timer0->compare0 = (FCPU/1000);
	timer0->counter0 = 0;
	timer0->tcr0     = TIMER_EN | TIMER_AR | TIMER_IRQEN;
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




