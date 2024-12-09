#ifndef ADC_H
#define	ADC_H

#include "main.h"
#include <stdint.h>

void ADC0_init(void);
uint16_t ADC0_read(void);
void ADC0_start(void);

#endif	/* ADC_H */

