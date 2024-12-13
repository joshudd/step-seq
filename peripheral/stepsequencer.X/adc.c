#include "adc.h"

void adc_init(void)
{
    PORTD.PIN1CTRL &= ~PORT_ISC_gm;
    PORTD.PIN1CTRL |= PORT_ISC_INPUT_DISABLE_gc;
    PORTD.PIN1CTRL &= ~PORT_PULLUPEN_bm;
    ADC0.CTRLC = ADC_PRESC_DIV32_gc | ADC_REFSEL_VDDREF_gc;
    ADC0.CTRLA = ADC_RESSEL_8BIT_gc;
    ADC0.MUXPOS = ADC_MUXPOS_AIN1_gc;
    ADC0.INTCTRL |= ADC_RESRDY_bm;
}

uint16_t adc_read(void)
{
    ADC0.INTFLAGS = ADC_RESRDY_bm;
    return ADC0.RES;
}

void adc_start(void)
{
    ADC0.CTRLA |= ADC_ENABLE_bm;
    ADC0.CTRLA |= ADC_FREERUN_bm;
    ADC0.COMMAND = ADC_STCONV_bm;
}

void adc_stop(void)
{
    ADC0.CTRLA &= ~ADC_FREERUN_bm;
    ADC0.COMMAND &= ~ADC_STCONV_bm;
    ADC0.CTRLA &= ~ADC_ENABLE_bm;
}