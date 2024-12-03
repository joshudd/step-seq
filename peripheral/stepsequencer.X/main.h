#ifndef MAIN_H
#define MAIN_H

#define F_CPU 3333333
#include <avr/io.h>
#include <util/delay.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>

#define MIDI_SERVICE_UUID "03b80e5aede84b33a7516ce34ec4c700"
#define MIDI_CHARACTERISTIC_UUID "7772e5db38684112a1a9f2669d106bf3"

#define BUF_SIZE 128
#define BLE_RADIO_PROMPT "CMD> "

#define SAMPLES_PER_BIT 16
#define USART_BAUD_VALUE(BAUD_RATE) (uint16_t) ((F_CPU << 6) / (((float) SAMPLES_PER_BIT) * (BAUD_RATE)) + 0.5)

#define MIDI_NOTE_ON 0x90 // Note On message
#define MIDI_NOTE_NUMBER 60 // Middle C
#define MIDI_VELOCITY 127   // Max velocity

#endif	/* MAIN_H */