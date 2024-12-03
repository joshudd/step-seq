#include "main.h"
#include "interface.h"
#include "connection.h"
#include "midi.h"

volatile int red_released = 0;
volatile int yellow_released = 0;
int red_count = 0;
int yellow_count = 0;

/**
 * Interrupt Service Routine for the red and yellow buttons
 */
ISR(PORTA_PORT_vect) {
   if (RED_INTERRUPT) {
       red_released = 1;
       RED_INTERRUPT_CLEAR;
   } else if (YELLOW_INTERRUPT) {
       yellow_released = 1;
       YELLOW_INTERRUPT_CLEAR;
   }
}

void setup() {
    serialInit(); // DEBUGGING USE
    
    usartInit();
    bleInit();
    setupButtons();

    sei();
}

int main() {
    setup();

    while (1) {
        handleRedButton();
        handleYellowButton();

        readBleData();

        _delay_ms(10);
    }
}