#include "rtc.h"

void rtc_init(void) {
    RTC.CLKSEL = RTC_CLKSEL_INT32K_gc; // 32.768kHz internal oscillator
    while (RTC.STATUS > 0); // wait for all register to be synchronized

    // PER = (32768 Hz / 32 prescaler) * (60 secs / BPM) - 1
    // RTC.PER = HALF_NOTE_FREQUENCY;
    // RTC.PER = QUARTER_NOTE_FREQUENCY;
    RTC.PER = EIGHTH_NOTE_FREQUENCY;
    // RTC.PER = SIXTEENTH_NOTE_FREQUENCY;

    RTC.CTRLA = RTC_PRESCALER_DIV32_gc
    | RTC_RTCEN_bm
    | RTC_RUNSTDBY_bm;
    
    RTC.INTCTRL = RTC_OVF_bm; // enable overflow and compare interrupt
}
