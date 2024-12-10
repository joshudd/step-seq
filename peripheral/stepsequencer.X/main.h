#ifndef MAIN_H
#define MAIN_H

#include "interface.h"
#include "connection.h"
#include "midi.h"
#include "rtc.h"
#include "twi.h"
#include "display.h"
#include "adc.h"

#ifndef F_CPU
#define F_CPU 3333333
#endif

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

#define BPM 111
#define NUM_STEPS 8
typedef struct {
    bool isActive;
    uint8_t note;
    uint8_t velocity;
} Step;

typedef struct {
    int note;       // note value (0-127)
    int octave;     // octave (0-8)
    int velocity;   // velocity (0-127)
    int duration;   // duration (1-100)
    bool active;    // is active
} NoteState;

typedef struct {
    const char* descriptor;  // Text description of the setting ("NOTE", "VEL", etc)
    int value;              // Current value
    int min_value;          // Minimum allowed value
    int max_value;          // Maximum allowed value
    const char* (*format_value)(int value);  // Optional function to format value (e.g., note to text)
} SettingInfo;

extern volatile bool ble_connected;

void playStep(NoteState step);
void handleRedButton();
void handleYellowButton();

#define YELLOW_BUTTON_PRESSED !(PORTA.IN & PIN5_bm)  // Assuming yellow button is on PA3

#endif	/* MAIN_H */
