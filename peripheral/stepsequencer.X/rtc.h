#ifndef RTC_H
#define	RTC_H

#include "main.h"

#include <avr/io.h>
#include <avr/interrupt.h>
#include <stdlib.h>
#include <stdbool.h>

#define BASE_FREQUENCY (32768UL / 32)

static inline uint32_t calculateNoteDuration(float noteFraction, uint32_t bpm) {
    return (uint32_t)((BASE_FREQUENCY) * 60 / bpm * noteFraction - 1);
}

#define QUARTER_NOTE_FREQUENCY calculateNoteDuration(1.0, BPM)
#define HALF_NOTE_FREQUENCY calculateNoteDuration(2.0, BPM)
#define EIGHTH_NOTE_FREQUENCY calculateNoteDuration(0.5, BPM)
#define SIXTEENTH_NOTE_FREQUENCY calculateNoteDuration(0.25, BPM)
#define TRIPLET_FREQUENCY calculateNoteDuration(1.0 / 3, BPM)

void rtc_init(void);

#endif	/* RTC_H */