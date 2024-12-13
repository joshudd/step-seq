#include "usart.h"

void usart_init()
{
    PORTA.DIR &= ~PIN1_bm;
    PORTA.DIR |= PIN0_bm;

    USART0.BAUD = (uint16_t)USART_BAUD_VALUE(115200);

    USART0.CTRLB |= USART_TXEN_bm;
    USART0.CTRLB |= USART_RXEN_bm;
    USART0.CTRLB |= USART_RXMODE_NORMAL_gc;

    USART0.CTRLC |= USART_CMODE_ASYNCHRONOUS_gc;
    USART0.CTRLC |= USART_CHSIZE_8BIT_gc;
}

void usart_write_char(char c)
{
    while (!(USART0.STATUS & USART_DREIF_bm))
    {
        ;
    }
    USART0.TXDATAL = c;
}

void usart_write_command(const char *cmd)
{
    for (uint8_t i = 0; cmd[i] != '\0'; i++)
    {
        usart_write_char(cmd[i]);
    }
}

char usart_read_char()
{
    while (!(USART0.STATUS & USART_RXCIF_bm))
    {
        ;
    }
    return USART0.RXDATAL;
}

void usart_read_until(char *dest, const char *end_str)
{
    memset(dest, 0, BUF_SIZE);
    uint8_t end_len = strlen(end_str);
    uint8_t bytes_read = 0;
    uint32_t max_attempts = 100000; // Set a maximum number of attempts

    while (bytes_read < BUF_SIZE - 1 && max_attempts > 0)
    {
        if (USART0.STATUS & USART_RXCIF_bm)
        {
            dest[bytes_read] = usart_read_char();
            bytes_read++;

            if (bytes_read >= end_len && strcmp(dest + bytes_read - end_len, end_str) == 0)
            {
                break;
            }
        }
        else
        {
            _delay_us(1);
        }
        max_attempts--;
    }
    dest[bytes_read] = '\0';
}

// DEBUGGING USE
void serial_init(void)
{
    PORTF.DIRSET = PIN0_bm;
    PORTF.DIRCLR = PIN1_bm;
    USART2.BAUD = (uint16_t)USART_BAUD_VALUE(9600);
    USART2.CTRLB |= USART_TXEN_bm;
}

// DEBUGGING USE
void serial_print_f(char *str)
{
    for (size_t i = 0; i < strlen(str); i++)
    {
        while (!(USART2.STATUS & USART_DREIF_bm))
            ;
        USART2.TXDATAL = str[i];
    }
}
