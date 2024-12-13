#include "twi.h"

#define TWI_WAIT_WRITE()                 \
    while (!(TWI0.MSTATUS & TWI_WIF_bm)) \
        ;
#define TWI_WAIT_READ()                  \
    while (!(TWI0.MSTATUS & TWI_RIF_bm)) \
        ;
#define TWI_READ true
#define TWI_WRITE false
#define TWI_CLIENT_NACK() TWI0.MSTATUS &TWI_RXACK_bm
#define TWI_IS_CLOCKHELD() TWI0.MSTATUS &TWI_CLKHOLD_bm
#define TWI_IS_BUSERR() TWI0.MSTATUS &TWI_BUSERR_bm

void twi_init()
{
    twi_init_pins();
    TWI0.CTRLA = TWI_SDAHOLD_50NS_gc;
    TWI0.MSTATUS = TWI_RIF_bm | TWI_WIF_bm | TWI_CLKHOLD_bm | TWI_RXACK_bm |
                   TWI_ARBLOST_bm | TWI_BUSERR_bm | TWI_BUSSTATE_IDLE_gc;

    // Set for 100kHz from a 16MHz oscillator, CLKDIV = 6x
    TWI0.MBAUD = 10;

    TWI0.MCTRLA = TWI_ENABLE_bm;
    TWI0.DBGCTRL = TWI_DBGRUN_bm;
}

void twi_init_pins(void)
{
    PORTA.DIRCLR = PIN2_bm | PIN3_bm;
    PORTA.PIN2CTRL = PORT_PULLUPEN_bm;
    PORTA.PIN3CTRL = PORT_PULLUPEN_bm;
    PORTA.OUTSET = PIN2_bm | PIN3_bm;
    PORTA.DIRSET = PIN2_bm | PIN3_bm;
}

bool twi_write_bytes_to_display(const uint8_t *data, uint8_t len)
{
    uint8_t count = 0;
    while (count < len)
    {
        TWI0.MDATA = data[count];
        TWI_WAIT_WRITE();

        count++;
    }

    return true;
}

void twi_start_write(uint8_t addr)
{
    TWI0.MADDR = (addr << 1) | TWI_WRITE;
    TWI_WAIT_WRITE();
}

void twi_stop(void)
{
    TWI0.MCTRLB = TWI_MCMD_STOP_gc;
}

bool twi_send_bytes(uint8_t addr, uint8_t *data, uint8_t len)
{
    twi_start_write(addr);
    bool success = twi_write_bytes_to_display(data, len); // write the bytes to the client
    twi_stop();
    return success;
}
