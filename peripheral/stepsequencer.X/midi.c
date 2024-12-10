#include "midi.h"

const char* NOTE_NAMES[] = {"C ", "C#", "D ", "D#", "E ", "F ", "F#", "G ", "G#", "A ", "A#", "B "};

const char* getMidiNoteName(uint8_t midiNote) {
    return NOTE_NAMES[midiNote % 12];
}

// Optional: If you also want to get the octave
int getMidiNoteOctave(uint8_t midiNote) {
    return (midiNote / 12) - 1;  // MIDI note 60 is middle C (C4)
}

void sendMidiMessage(uint8_t status, uint8_t data1, uint8_t data2) {
    // Implement the function to send MIDI messages over BLE
    char buf[BUF_SIZE];
    sprintf(buf, "CHW,0073,%02X %02X %02X", status, data1, data2);
    usartWriteCommand(buf);
}

void constructMidiBlePacket(uint8_t *midiMessage, uint8_t midiMessageLength, uint8_t *blePacket) {
    // Set the timestamp bytes
    blePacket[0] = 0x00;
    blePacket[1] = 0x00;

    // Copy the MIDI message into the BLE packet
    for (uint8_t i = 0; i < midiMessageLength; i++) {
        blePacket[i + 2] = midiMessage[i];
    }
}

void midiPacketToString(uint8_t *midiPacket, char *string) {
    sprintf(string, "%02X%02X%02X%02X%02X", midiPacket[0], midiPacket[1], midiPacket[2], midiPacket[3], midiPacket[4]);
}