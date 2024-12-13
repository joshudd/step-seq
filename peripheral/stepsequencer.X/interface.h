
#ifndef INTERFACE_H
#define	INTERFACE_H

#include "main.h"
#include "midi.h"
#include "ble.h"
#include "config.h"

extern volatile int red_released;
extern volatile int yellow_released;
extern int red_count;
extern int yellow_count;

#define RED_BUTTON PIN4_bm
#define RED_INTERRUPT (PORTA.INTFLAGS & RED_BUTTON)
#define RED_INTERRUPT_CLEAR (PORTA.INTFLAGS = RED_BUTTON)

#define YELLOW_BUTTON PIN5_bm
#define YELLOW_INTERRUPT (PORTA.INTFLAGS & YELLOW_BUTTON)
#define YELLOW_INTERRUPT_CLEAR (PORTA.INTFLAGS = YELLOW_BUTTON)

void gpio_init();

#endif	/* INTERFACE_H */

