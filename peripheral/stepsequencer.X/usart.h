#ifndef USART_H
#define USART_H

#include "main.h"

#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>

#define SAMPLES_PER_BIT 16
#define USART_BAUD_VALUE(BAUD_RATE) (uint16_t)((F_CPU << 6) / (((float)SAMPLES_PER_BIT) * (BAUD_RATE)) + 0.5)

void usart_init();
void usart_write_char(char c);
void usart_write_command(const char *cmd);
char usart_read_char();
void usart_read_until(char *dest, const char *end_str);

// DEBUGGING USE
void serial_init(void);
void serial_print_f(char *str);

#endif /* USART_H */
