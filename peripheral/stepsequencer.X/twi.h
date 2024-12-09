
#ifndef TWI_H
#define	TWI_H

#include <avr/io.h>
#include <avr/interrupt.h>
// #include <util/delay.h> // messes up serial data transfer?
#include <stdlib.h>
#include <stdbool.h>

void twi_init(void);
void twi_init_pins(void);
bool twi_write_bytes_to_display(uint8_t* data, uint8_t len);
void twi_start_write(uint8_t addr);
void twi_stop(void);
bool twi_send_bytes(uint8_t addr, uint8_t* data, uint8_t len);

#endif	/* TWI_H */

