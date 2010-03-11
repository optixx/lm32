/**
 * 
 */

#include "soc-hw.h"



void test() {
    uart_putchar('a');
    uart_putchar('c');
} 

char glob[] = "Global";

volatile uint32_t *p;
volatile uint8_t *p2;

extern uint32_t tic_msec;

int main()
{
    while(1)
        test();
}
