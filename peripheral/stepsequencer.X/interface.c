#include "interface.h"

void setupButtons() {
    PORTA.DIRCLR = RED_BUTTON; // input
    PORTA.PIN4CTRL |= (PORT_PULLUPEN_bm | PORT_ISC_RISING_gc); // enable pull-up resistor and rising edge interrupt

    PORTA.DIRCLR = YELLOW_BUTTON; // input
    PORTA.PIN5CTRL |= (PORT_PULLUPEN_bm | PORT_ISC_RISING_gc); // enable pull-up resistor and rising edge interrupt
}
