#include "adc.h"

void ADC0_init(void) {
    /* Disable digital input buffer */
    PORTD.PIN1CTRL &= ~PORT_ISC_gm;
    PORTD.PIN1CTRL |= PORT_ISC_INPUT_DISABLE_gc;
    
    /* Disable pull-up resistor */
    PORTD.PIN1CTRL &= ~PORT_PULLUPEN_bm;
    
    ADC0.CTRLC = ADC_PRESC_DIV32_gc      /* Much slower sampling */
                | ADC_REFSEL_VDDREF_gc;   /* Internal reference */
    
    ADC0.CTRLA = ADC_RESSEL_8BIT_gc;     /* 8-bit mode */
    
    /* Select ADC channel */
    ADC0.MUXPOS = ADC_MUXPOS_AIN1_gc;
    
    /* enable interrupts */
    ADC0.INTCTRL |= ADC_RESRDY_bm;

    serialPrintF("[adc] initialized\r\n");
}

uint16_t ADC0_read(void) 
{
    /* Clear the interrupt flag by writing 1: */
    ADC0.INTFLAGS = ADC_RESRDY_bm;
    return ADC0.RES;
}

void ADC0_start(void){
    ADC0.CTRLA |= ADC_ENABLE_bm;
    ADC0.CTRLA |= ADC_FREERUN_bm;
    ADC0.COMMAND = ADC_STCONV_bm;
}

void ADC0_stop(void) {
    ADC0.CTRLA &= ~ADC_FREERUN_bm;
    ADC0.COMMAND &= ~ADC_STCONV_bm;
    ADC0.CTRLA &= ~ADC_ENABLE_bm;
}