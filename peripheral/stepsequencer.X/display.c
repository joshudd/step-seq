#include "display.h"

void display_init(void) {
    serialPrintF("[display] Starting OLED initialization...\r\n");
    
    _delay_ms(100);
    
    // Initialize with explicit width settings
    const uint8_t init_sequence[] = {
        OLED_COMMAND,
        DISPLAY_OFF,                // 0xAE
        SET_DISPLAY_CLOCK_DIV, 0x80,// 0xD5, 0x80
        SET_MULTIPLEX, 0x1F,        // 0xA8, 0x1F (HEIGHT - 1 = 31)
        SET_DISPLAY_OFFSET, 0x00,   // 0xD3, 0x00
        SET_START_LINE | 0x00,      // 0x40
        CHARGE_PUMP, 0x14,          // 0x8D, 0x14
        MEMORY_MODE, 0x00,          // 0x20, 0x00 (horizontal addressing)
        SEG_REMAP | 0x01,           // 0xA0 | 0x01
        COM_SCAN_DEC,               // 0xC8
        SET_COM_PINS, 0x02,         // 0xDA, 0x02 (sequential COM, disable remap)
        SET_CONTRAST, 0x7F,         // 0x81, 0x7F
        SET_PRECHARGE, 0x22,        // 0xD9, 0x22
        SET_VCOM_DETECT, 0x20,      // 0xDB, 0x20
        DISPLAY_RAM,                // 0xA4
        SET_NORMAL_DISPLAY,         // 0xA6
        DISPLAY_ON                  // 0xAF
    };
    
    // Send initialization sequence
    for (uint8_t i = 0; i < sizeof(init_sequence); i++) {
        twi_start_write(OLED_ADDRESS);
        twi_write_bytes_to_display((uint8_t[]){OLED_COMMAND, init_sequence[i]}, 2);
        twi_stop();
        _delay_us(100);
    }
    
    // Set addressing bounds
    twi_start_write(OLED_ADDRESS);
    twi_write_bytes_to_display((uint8_t[]){
        OLED_COMMAND,
        COLUMN_ADDR,
        0,                          // Column start
        DISPLAY_WIDTH - 1,          // Column end (127)
        PAGE_ADDR,
        0,                          // Page start
        (DISPLAY_HEIGHT / 8) - 1    // Page end (3)
    }, 7);
    twi_stop();
    
    // Clear all pixels
    display_clear();
    
    serialPrintF("[display] Display initialization complete.\r\n");
}

void display_clear(void) {
    twi_start_write(OLED_ADDRESS);
    uint8_t data_cmd = OLED_DATA;
    twi_write_bytes_to_display(&data_cmd, 1);
    
    for (uint16_t i = 0; i < (DISPLAY_WIDTH * (DISPLAY_HEIGHT/8)); i++) {
        uint8_t zero = 0x00;
        twi_write_bytes_to_display(&zero, 1);
    }
    twi_stop();
}

// Test function to draw a single horizontal line
void test_display(void) {
    // Set column and page address for the first horizontal line
    twi_start_write(OLED_ADDRESS);
    twi_write_bytes_to_display((uint8_t[]){
        OLED_COMMAND,
        COLUMN_ADDR, 0, DISPLAY_WIDTH - 1,  // Full width
        PAGE_ADDR, 0, 0                     // First page only
    }, 7);
    twi_stop();
    
    // Write line data for the first horizontal line
    twi_start_write(OLED_ADDRESS);
    uint8_t data_cmd = OLED_DATA;
    twi_write_bytes_to_display(&data_cmd, 1);
    
    for (uint8_t i = 0; i < DISPLAY_WIDTH; i++) {
        uint8_t line = 0x01;  // Light up the first bit in each byte
        twi_write_bytes_to_display(&line, 1);
    }
    twi_stop();

    // Set column and page address for the second horizontal line
    twi_start_write(OLED_ADDRESS);
    twi_write_bytes_to_display((uint8_t[]){
        OLED_COMMAND,
        COLUMN_ADDR, 0, DISPLAY_WIDTH - 1,  // Full width
        PAGE_ADDR, 1, 1                     // Second page only
    }, 7);
    twi_stop();
    
    // Write line data for the second horizontal line
    twi_start_write(OLED_ADDRESS);
    twi_write_bytes_to_display(&data_cmd, 1);
    
    for (uint8_t i = 0; i < DISPLAY_WIDTH; i++) {
        uint8_t line = 0x01;  // Light up the first bit in each byte
        twi_write_bytes_to_display(&line, 1);
    }
    twi_stop();
}

void write_string(const char *str, uint8_t line) {
    // Set addressing mode and position
    twi_start_write(OLED_ADDRESS);
    twi_write_bytes_to_display((uint8_t[]){
        OLED_COMMAND,
        MEMORY_MODE, 0x00,          // horizontal addressing mode
        COLUMN_ADDR, 0x00, 0x7F,    // start = 0, end = 127
        PAGE_ADDR, line, line       // start = 0, end = 3
    }, 8);
    twi_stop();

    // Clear display first
    display_clear();

    // Now write the string
    uint8_t data_command = OLED_DATA;
    twi_start_write(OLED_ADDRESS);
    twi_write_bytes_to_display(&data_command, 1);
    
    uint8_t x_pos = 0;
    while (*str && x_pos < 128) {
        char c = *str++;

        char debug[32];
        sprintf(debug, "[display] Writing character: %c\r\n", c);
        serialPrintF(debug);
        sprintf(debug, "[display] x_pos: %d\r\n", x_pos);
        serialPrintF(debug);
        
        // Each character is 5 columns wide
        for (uint8_t i = 0; i < 5; i++) {
            // Get the column data for this character
            uint8_t col = pgm_read_byte(&font[c * 5 + i]);
            // Write the column data
            twi_write_bytes_to_display(&col, 1);
            x_pos++;
        }
        
        // Add a one-pixel space between characters
        uint8_t space = 0x00;
        twi_write_bytes_to_display(&space, 1);
        x_pos++;
    }
    
    twi_stop();
}