#ifndef MIDI_H
#define	MIDI_H

#include "main.h"

#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>

#define MIDI_NOTE_ON 0x90
#define MIDI_NOTE_OFF 0x80

#define TIMESTAMP_UPPER 0x80 // Example timestamp upper byte
#define TIMESTAMP_LOWER 0x81 // Example timestamp lower byte

void sendMidiMessage(uint8_t status, uint8_t data1, uint8_t data2);
void constructMidiBlePacket(uint8_t *midiMessage, uint8_t midiMessageLength, uint8_t *blePacket);
void midiPacketToString(uint8_t *midiPacket, char *string);

#endif	/* MIDI_H */