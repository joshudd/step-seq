#include "interface.h"

void gpio_init()
{
    PORTA.DIRCLR = RED_BUTTON;                                    // input
    PORTA.PIN4CTRL |= (PORT_PULLUPEN_bm | PORT_ISC_BOTHEDGES_gc); // enable pull-up resistor and both edges interrupt

    PORTA.DIRCLR = YELLOW_BUTTON;                                 // input
    PORTA.PIN5CTRL |= (PORT_PULLUPEN_bm | PORT_ISC_BOTHEDGES_gc); // enable pull-up resistor and both edges interrupt
}
