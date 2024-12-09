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

#define BPM 60
#define NUM_STEPS 8
typedef struct {
    bool isActive;
    uint8_t note;
    uint8_t velocity;
} Step;

bool initializeClientRole(void);
void readCharacteristic(uint16_t handle);
void writeCharacteristic(uint16_t handle, const char *value);

void playStep(Step step);
void handleRedButton();
void handleYellowButton();

#endif	/* MAIN_H */
