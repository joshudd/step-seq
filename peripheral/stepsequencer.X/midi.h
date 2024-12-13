#ifndef MIDI_H
#define MIDI_H

#include "main.h"
#include "ble.h"
#include "config.h"

#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>

#define MIDI_NOTE_ON 0x90
#define MIDI_NOTE_OFF 0x80

#define TIMESTAMP_UPPER 0x80
#define TIMESTAMP_LOWER 0x81

extern const char *NOTE_NAMES[];

const char *get_midi_note_name(uint8_t midi_note);
void send_midi_note(uint8_t note_on_off, uint8_t note, uint8_t velocity);
void gatt_server_send_characteristic_notification(uint16_t handle, uint8_t *data, size_t length);

#endif /* MIDI_H */