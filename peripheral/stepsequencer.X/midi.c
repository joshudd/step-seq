#include "midi.h"

const char* NOTE_NAMES[] = {"C ", "C#", "D ", "D#", "E ", "F ", "F#", "G ", "G#", "A ", "A#", "B "};

const char* getMidiNoteName(uint8_t midiNote) {
    return NOTE_NAMES[midiNote % 12];
}

// Optional: If you also want to get the octave
int getMidiNoteOctave(uint8_t midiNote) {
    return (midiNote / 12) - 1;  // MIDI note 60 is middle C (C4)
}

void send_midi_note(uint8_t note_on_off, uint8_t note, uint8_t velocity) {
    uint8_t midiChannel = 0;

    uint32_t timestamp = 0x0002 & 0x1FFF; // Mask to lower 13 bits
    char buf[BUF_SIZE];
    size_t packetLength = 0;

    // header byte (0b10xxxxxx)
    uint8_t headerByte = 0x80 | (timestamp >> 6); // set bit 7 to 1, include top 6 bits of timestamp
    buf[packetLength++] = headerByte;

    // timestamp byte (0b1xxxxxxx)
    buf[packetLength++] = 0x80 | (timestamp & 0x3F); // Set bit 7 to 1, include lower 7 bits of timestamp

    // status byte for note on message (note on or off)
    buf[packetLength++] = note_on_off | (midiChannel & 0x0F); // Set channel

    // note and velocity
    buf[packetLength++] = note;
    buf[packetLength++] = velocity;

    // Send the message using the characteristic notification function
    gatt_server_send_characteristic_notification(0x0072, buf, packetLength);
}

void gatt_server_send_characteristic_notification(uint16_t handle, uint8_t *data, size_t length) {
    char buf[BUF_SIZE];
    strcpy(buf, "SHW,"); // start with the command
    sprintf(buf + strlen(buf), "%04X,", handle); // append the handle

    // append the data
    for (size_t i = 0; i < length; i++) {
        sprintf(buf + strlen(buf), "%02X", data[i]);
    }

    sprintf(buf + strlen(buf), "\r\n");
    usartWriteCommand(buf); // write the characteristic
    usartReadUntil(buf, BLE_RADIO_PROMPT); // wait for the response
}
