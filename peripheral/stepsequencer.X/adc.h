#ifndef ADC_H
#define	ADC_H

#include "main.h"
#include "config.h"

#include <stdint.h>

void adc_init(void);
uint16_t adc_read(void);
void adc_start(void);
void adc_stop(void);

#endif	/* ADC_H */

