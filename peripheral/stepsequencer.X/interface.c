#include "interface.h"

void setupButtons() {
    PORTA.DIRCLR = RED_BUTTON; // input
    PORTA.PIN2CTRL |= (PORT_PULLUPEN_bm | PORT_ISC_RISING_gc); // enable pull-up resistor and rising edge interrupt

    PORTA.DIRCLR = YELLOW_BUTTON; // input
    PORTA.PIN3CTRL |= (PORT_PULLUPEN_bm | PORT_ISC_RISING_gc); // enable pull-up resistor and rising edge interrupt
}

void handleRedButton() {
    if (red_released) {
        char print_string[32];
        sprintf(print_string, "[me] red button pressed: %d\r\n", red_count);
        serialPrintF(print_string);

        startAdvertising();
        
        _delay_ms(10);
        red_released = 0;
        red_count++;
    }
}

void handleYellowButton() {
    if (yellow_released) {
        char print_string[32];
        sprintf(print_string, "[me] yellow button pressed: %d\r\n", yellow_count);
        serialPrintF(print_string);

        sendMidiMessage(MIDI_NOTE_ON, MIDI_NOTE_NUMBER, MIDI_VELOCITY);

        // getConnectionStatus();
        // listServicesAndCharacteristics();

        _delay_ms(10);
        yellow_released = 0;
        yellow_count++;
    }
}