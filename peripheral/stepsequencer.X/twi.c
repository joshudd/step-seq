#include "twi.h"

#define TWI_WAIT_WRITE() while (!(TWI0.MSTATUS & TWI_WIF_bm));
#define TWI_WAIT_READ() while (!(TWI0.MSTATUS & TWI_RIF_bm));
#define TWI_READ true
#define TWI_WRITE false
#define TWI_CLIENT_NACK() TWI0.MSTATUS & TWI_RXACK_bm

void twi_init() {
    // setup TWI I/O
    twi_init_pins();
    
    // Standard 100kHz TWI, 4 Cycle Hold, 50ns SDA Hold Time
    TWI0.CTRLA = TWI_SDAHOLD_50NS_gc;

    // clear MSTATUS (write 1 to flags). BUSSTATE set to idle
    TWI0.MSTATUS = TWI_RIF_bm | TWI_WIF_bm | TWI_CLKHOLD_bm | TWI_RXACK_bm |
            TWI_ARBLOST_bm | TWI_BUSERR_bm | TWI_BUSSTATE_IDLE_gc;
    
    // TWI0.MBAUD = 10;
    //Set for 100kHz from a 16MHz oscillator, CLKDIV = 6x
    TWI0.MBAUD = 10;
    
    // Enable TWI in master mode
    TWI0.MCTRLA = TWI_ENABLE_bm;
}

void twi_init_pins(void) {
    // Set PA2 (SDA) and PA3 (SCL) as inputs initially
    PORTA.DIRCLR = PIN2_bm | PIN3_bm;

    // Enable pull-ups
    PORTA.PIN2CTRL = PORT_PULLUPEN_bm;
    PORTA.PIN3CTRL = PORT_PULLUPEN_bm;

    // Set pins high (for pull-up)
    PORTA.OUTSET = PIN2_bm | PIN3_bm;
    
    // Now set as outputs (TWI will control them)
    PORTA.DIRSET = PIN2_bm | PIN3_bm;
}

bool twi_write_bytes_to_display(uint8_t* data, uint8_t len) {
    uint8_t count = 0;
    while (count < len) {
        // write a byte
        TWI0.MDATA = data[count];

        TWI_WAIT_WRITE();
        
        // if the client NACKed, then abort the write
        if (TWI_CLIENT_NACK()) { 
            serialPrintF("[twi] NACK received at byte ");
            char debug[32];
            sprintf(debug, "%d\r\n", count);
            serialPrintF(debug);
            return false; 
        }
        
        count++;
    }
    
    return true;
}

void twi_start_write(uint8_t addr) {
    // address client device (write)
    TWI0.MADDR = (addr << 1) | TWI_WRITE;
    TWI_WAIT_WRITE();
    
    if (TWI_CLIENT_NACK()) {
        serialPrintF("[twi] Device NACK at address phase\r\n");
    }
}

void twi_stop(void) {
    TWI0.MCTRLB = TWI_MCMD_STOP_gc;
}

bool twi_send_bytes(uint8_t addr, uint8_t* data, uint8_t len) {
    twi_start_write(addr);
    bool success = twi_write_bytes_to_display(data, len);    // write the bytes to the client
    twi_stop();
    return success;
}
