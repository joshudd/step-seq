#include "midi.h"

void sendMidiMessage(uint8_t status, uint8_t data1, uint8_t data2) {
    // Implement the function to send MIDI messages over BLE
    char buf[BUF_SIZE];
    sprintf(buf, "%02X%02X%02X", status, data1, data2);
    usartWriteCommand(buf);
}