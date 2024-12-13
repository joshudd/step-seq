#ifndef MAIN_H
#define MAIN_H

#include "config.h"
#include "types.h"
#include "interface.h"
#include "ble.h"
#include "midi.h"
#include "usart.h"
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

#define BUF_SIZE 128
#define BPM 111

extern volatile bool ble_connected;

void play_step(NoteState step);
void handle_red_button();
void handle_yellow_button();

#endif	/* MAIN_H */
