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
    sprintf(buf, "CHW,0072,%02X %02X %02X", status, data1, data2);
    usartWriteCommand(buf);
}

void sendMidiMessageNew(uint8_t *midiData, size_t length, uint16_t timestamp) {
    char buf[BUF_SIZE];
    size_t packetLength = 0;

    // Prepare the header byte
    uint8_t headerByte = 0x80 | (timestamp >> 7); // Set bit 7 to 1, and include timestamp high
    buf[packetLength++] = headerByte;

    // Prepare the timestamp bytes
    buf[packetLength++] = (uint8_t)(timestamp & 0x7F); // Timestamp low

    // Encode MIDI message
    for (size_t i = 0; i < length; i++) {
        if (i == 0) {
            buf[packetLength++] = midiData[i] | 0x80; // Status byte
        } else {
            buf[packetLength++] = midiData[i]; // Data byte
        }
    }

    // Send the command
    usartWriteCommand(buf);
    usartReadUntil(buf, BLE_RADIO_PROMPT);
}

void midi_note_on(uint8_t midiChannel, uint8_t note, uint8_t velocity) {
    uint32_t timestamp = 0x0002 & 0x1FFF; // Mask to lower 13 bits
    char buf[BUF_SIZE];
    size_t packetLength = 0;

    // Prepare the header byte (0b10xxxxxx)
    uint8_t headerByte = 0x80 | (timestamp >> 6); // Set bit 7 to 1, include top 6 bits of timestamp
    buf[packetLength++] = headerByte;

    // Prepare the timestamp byte (0b1xxxxxxx)
    buf[packetLength++] = 0x80 | (timestamp & 0x3F); // Set bit 7 to 1, include lower 7 bits of timestamp

    // Status byte for note on message (0x90)
    buf[packetLength++] = 0x90 | (midiChannel & 0x0F); // Set channel

    // Note and velocity
    buf[packetLength++] = note; // Note
    buf[packetLength++] = velocity; // Velocity

    // while (!initializeClientOperation()) {} // wait for client to initialize

    // Send the message using the characteristic notification function
    gatt_server_send_characteristic_notification(0x0072, buf, packetLength);
}

void gatt_server_send_characteristic_notification(uint16_t handle, uint8_t *data, size_t length) {
    char buf[BUF_SIZE];
    strcpy(buf, "SHW,");
    sprintf(buf + strlen(buf), "%04X,", handle);
    // strcat(buf, ",06\r\n");

    for (size_t i = 0; i < length; i++) {
        sprintf(buf + strlen(buf), "%02X", data[i]);
    }

    sprintf(buf + strlen(buf), "\r\n");
    usartWriteCommand(buf); // Send the command to write the characteristic
    usartReadUntil(buf, BLE_RADIO_PROMPT); // Wait for the response

    // char buf[BUF_SIZE];
    // strcpy(buf, "SHW,0072,08\r\n"); // Replace with actual UUID
    // usartWriteCommand(buf); // Send the command with no payload
    // usartReadUntil(buf, BLE_RADIO_PROMPT); // Optionally read the response
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