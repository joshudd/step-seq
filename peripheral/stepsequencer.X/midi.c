#include "midi.h"

const char* NOTE_NAMES[] = {"C ", "C#", "D ", "D#", "E ", "F ", "F#", "G ", "G#", "A ", "A#", "B "};

const char* get_midi_note_name(uint8_t midi_note) {
    static char note_name[8];
    const char* notes[] = {"C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"};
    int note_index = midi_note % 12;
    int octave = (midi_note / 12) - 1;
    
    sprintf(note_name, "%s%d", notes[note_index], octave);
    return note_name;
}

void send_midi_note(uint8_t note_on_off, uint8_t note, uint8_t velocity) {
    uint8_t midi_channel = 0;
    
    uint32_t timestamp = 0x0002 & 0x1FFF; // mask to lower 13 bits
    uint8_t buf[BUF_SIZE];
    size_t packet_length = 0;

    // header byte (0b10xxxxxx)
    uint8_t header_byte = 0x80 | (timestamp >> 6); // set bit 7 to 1, include top 6 bits of timestamp
    buf[packet_length++] = header_byte;

    // timestamp byte (0b1xxxxxxx)
    buf[packet_length++] = 0x80 | (timestamp & 0x3F); // set bit 7 to 1, include lower 7 bits of timestamp

    // status byte for note on message (note on or off)
    buf[packet_length++] = note_on_off | (midi_channel & 0x0F); // set channel

    // note and velocity
    buf[packet_length++] = note;
    buf[packet_length++] = velocity;

    // send the message
    gatt_server_send_characteristic_notification(0x0072, buf, packet_length);
}

