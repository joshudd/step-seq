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

    // Calculate string length and prepare buffer
    uint8_t str_len = strlen(str);
    uint8_t buffer_size = str_len * 6;  // 5 columns per char + 1 space
    uint8_t display_buffer[128];        // Max width of display
    uint8_t buffer_pos = 0;

    // Fill buffer with character data
    while (*str && buffer_pos < 128) {
        char c = *str++;
        
        // Copy character bitmap data to buffer
        for (uint8_t i = 0; i < 5; i++) {
            display_buffer[buffer_pos++] = pgm_read_byte(&font[c * 5 + i]);
        }
        
        // Add space between characters
        display_buffer[buffer_pos++] = 0x00;
    }

    // Write entire buffer to display
    twi_start_write(OLED_ADDRESS);
    uint8_t data_command = OLED_DATA;
    twi_write_bytes_to_display(&data_command, 1);
    twi_write_bytes_to_display(display_buffer, buffer_pos);
    twi_stop();
}

void draw_snake(void) {
    // Set addressing mode and full display bounds
    twi_start_write(OLED_ADDRESS);
    twi_write_bytes_to_display((uint8_t[]){
        OLED_COMMAND,
        MEMORY_MODE, 0x00,          // horizontal addressing mode
        COLUMN_ADDR, 0, DISPLAY_WIDTH - 1,  // Full width
        PAGE_ADDR, 0, (DISPLAY_HEIGHT / 8) - 1  // All pages
    }, 8);
    twi_stop();

    // Start data transmission
    twi_start_write(OLED_ADDRESS);
    uint8_t data_cmd = OLED_DATA;
    twi_write_bytes_to_display(&data_cmd, 1);

    // Process each page (8 rows at a time)
    for (uint8_t page = 0; page < (DISPLAY_HEIGHT / 8); page++) {
        for (uint8_t col = 0; col < DISPLAY_WIDTH; col++) {
            uint8_t byte = 0;
            
            // Calculate snake position for this column
            uint8_t x = col;
            for (uint8_t bit = 0; bit < 8; bit++) {
                uint8_t y = (page * 8) + bit;
                
                // Snake head (around x=10, y=16)
                if (x >= 10 && x < 13 && y >= 15 && y < 18) {
                    byte |= (1 << bit);
                }
                
                // Snake body
                uint8_t segment = (x - 13) / 20;  // Every 20 pixels is a new segment
                if (segment < 8 && x >= 13) {  // 8 segments max
                    uint8_t pos_in_segment = (x - 13) % 20;
                    uint8_t base_y = 16;  // Center position
                    int8_t offset = (segment % 2 == 0) ? 
                        (pos_in_segment / 2) :     // Going up
                        -(pos_in_segment / 2);     // Going down
                    
                    if (y == (base_y + offset)) {
                        byte |= (1 << bit);
                    }
                }
            }
            
            twi_write_bytes_to_display(&byte, 1);
        }
    }
    
    twi_stop();
}

void display_custom_bitmap(void) {
    // Set addressing mode and full display bounds
    twi_start_write(OLED_ADDRESS);
    twi_write_bytes_to_display((uint8_t[]){
        OLED_COMMAND,
        MEMORY_MODE, 0x00,          // horizontal addressing mode
        COLUMN_ADDR, 0, DISPLAY_WIDTH - 1,  // Full width
        PAGE_ADDR, 0, (DISPLAY_HEIGHT / 8) - 1  // All pages
    }, 8);
    twi_stop();

    // Write bitmap data
    twi_start_write(OLED_ADDRESS);
    uint8_t data_cmd = OLED_DATA;
    twi_write_bytes_to_display(&data_cmd, 1);

    // Example small bitmap (9x4 pixels)
    const uint8_t bitmap[4][9] = {
        {1,0,0,0,1,0,0,0,1},  // First row
        {0,1,1,1,0,1,1,1,0},  // Second row
        {0,1,1,1,0,1,1,1,0},  // Third row
        {0,1,1,1,0,1,1,1,0},  // Fourth row
    };
    
    // Process each page (8 rows at a time)
    for (uint8_t page = 0; page < (DISPLAY_HEIGHT / 8); page++) {
        for (uint8_t col = 0; col < DISPLAY_WIDTH; col++) {
            uint8_t byte = 0;
            
            // Only process pixels if we're within the bitmap bounds
            if (col < 9) {  // bitmap width
                for (uint8_t bit = 0; bit < 8; bit++) {
                    uint8_t y = (page * 8) + bit;
                    if (y < 4) {  // bitmap height
                        if (bitmap[y][col]) {
                            byte |= (1 << bit);
                        }
                    }
                }
            }
            
            twi_write_bytes_to_display(&byte, 1);
        }
    }
    
    twi_stop();
}

void display_bitmap(const uint8_t bitmap[DISPLAY_HEIGHT][DISPLAY_WIDTH]) {
    // Set addressing mode and full display bounds
    twi_start_write(OLED_ADDRESS);
    twi_write_bytes_to_display((uint8_t[]){
        OLED_COMMAND,
        MEMORY_MODE, 0x00,          // horizontal addressing mode
        COLUMN_ADDR, 0, DISPLAY_WIDTH - 1,  // Full width
        PAGE_ADDR, 0, (DISPLAY_HEIGHT / 8) - 1  // All pages
    }, 8);
    twi_stop();

    // Write bitmap data
    twi_start_write(OLED_ADDRESS);
    uint8_t data_cmd = OLED_DATA;
    twi_write_bytes_to_display(&data_cmd, 1);

    // Process 8 rows at a time (one page)
    for (uint8_t page = 0; page < (DISPLAY_HEIGHT / 8); page++) {
        for (uint8_t col = 0; col < DISPLAY_WIDTH; col++) {
            uint8_t byte = 0;
            // Combine 8 vertical pixels into one byte
            for (uint8_t bit = 0; bit < 8; bit++) {
                if (bitmap[(page * 8) + bit][col]) {
                    byte |= (1 << bit);
                }
            }
            twi_write_bytes_to_display(&byte, 1);
        }
    }
    twi_stop();
}

// Define step states
#define STEP_OFF    0
#define STEP_ON     1
#define STEP_ACTIVE 2  // Currently playing step

// Define bitmaps for step states (8x8 pixels each)
const uint8_t STEP_OFF_BITMAP[] = {
    0x7E, // ▐██████▌
    0x81, // █      █
    0x81, // █      █
    0x81, // █      █
    0x81, // █      █
    0x81, // █      █
    0x81, // █      █
    0x7E  // ▐██████▌
};

const uint8_t STEP_ON_BITMAP[] = {
    0x7E, // ████████
    0x81, // █      █
    0xBD, // █ ████ █
    0xBD, // █ ████ █
    0xBD, // █ ████ █
    0xBD, // █ ████ █
    0x81, // █      █
    0x7E  // ████████
};

// const uint8_t STEP_ON_BITMAP[] = {
//     0x00, // ████████
//     0x3C, // █      █
//     0x3C, // █ ████ █
//     0x3C, // █ ████ █
//     0x3C, // █ ████ █
//     0x3C, // █ ████ █
//     0x3C, // █      █
//     0x00  // ████████
// };

const uint8_t STEP_ACTIVE_BITMAP[] = {
    0x55, // ▐██████▌
    0xAA, // █ █ █ █
    0x55, // ▌█ █ █▐
    0xAA, // █ █ █ █
    0x55, // ▌█ █ █▐
    0xAA, // █ █ █ █
    0x55, // ▌█ █ █▐
    0xAA  // ▐██████▌
};

// const uint8_t STEP_ACTIVE_BITMAP[] = {
//     0x7E, // ████████
//     0xFF, // █ █ █ █
//     0xFF, // █ █ █ █
//     0xFF, // █ █ █ █
//     0xFF, // █ █ █ █
//     0xFF, // █ █ █ █
//     0xFF, // █ █ █ █
//     0x7E  // ████████
// };

// Constants for layout (moved to global scope)
#define STEP_SIZE     8     // 8x8 bitmap
#define MARGIN_X      8     // Left margin
#define MARGIN_Y      8     // Top margin
#define COL_SPACING   12    // Space between columns

// Helper function to set up display addressing
static void set_full_display_addressing(void) {
    twi_start_write(OLED_ADDRESS);
    twi_write_bytes_to_display((uint8_t[]){
        OLED_COMMAND,
        MEMORY_MODE, 0x00,          // horizontal addressing mode
        COLUMN_ADDR, 0, DISPLAY_WIDTH - 1,  
        PAGE_ADDR, 0, (DISPLAY_HEIGHT / 8) - 1
    }, 8);
    twi_stop();
}

// Helper function to draw a step square
static void draw_step_square(uint8_t* display_buffer, uint8_t col, uint8_t page, 
                           const uint8_t steps[2][4], uint8_t active_step) {
    uint8_t x = col - MARGIN_X;
    uint8_t step_col = x / COL_SPACING;
    uint8_t x_in_step = x % COL_SPACING;
    
    if (step_col < 4 && x_in_step < STEP_SIZE) {
        // Convert display page (0,2) to step row (0,1)
        uint8_t step_row = (page == 2) ? 1 : 0;
        uint8_t step_state = steps[step_row][step_col];
        uint8_t is_active = (step_row * 4 + step_col) == active_step;
        
        const uint8_t* bitmap;
        if (is_active) {
            bitmap = STEP_ACTIVE_BITMAP;
        } else if (step_state == STEP_ON) {
            bitmap = STEP_ON_BITMAP;
        } else {
            bitmap = STEP_OFF_BITMAP;
        }
        
        display_buffer[col] = bitmap[x_in_step];
    } else {
        display_buffer[col] = 0x00;  // Clear pixels outside step area
    }
}

// Helper function to draw step numbers
static void draw_step_indicator(uint8_t* display_buffer, uint8_t col, uint8_t active_step, uint8_t page) {
    uint8_t x = col - MARGIN_X;
    uint8_t step_col = x / COL_SPACING;

    // Only process if we're on the correct page for the active step
    bool is_valid_row = (active_step < 4 && page == 1) || 
                        (active_step >= 4 && page == 3);

    bool is_valid_col = (step_col == (active_step % 4));

    // Only draw indicator if this is the active step's position and we're on the correct page
    if (!is_valid_row || !is_valid_col) {
        display_buffer[col] = 0x00;
        return;
    }

    // Improved indicator positioning
    uint8_t number_start = MARGIN_X + (step_col * COL_SPACING) + ((STEP_SIZE - 5) / 2);
    uint8_t number_x = col - number_start;

    if (number_x < 5) {  // font is 5 pixels wide
        char number = 49+7+103;
        uint16_t font_index = (number - 32) * 5 + number_x;
        display_buffer[col] = pgm_read_byte(&font[font_index]);
    } else {
        display_buffer[col] = 0x00;
    }
}

void display_step_sequence(const uint8_t steps[2][4], uint8_t active_step) {
    uint8_t steps_display_buffer[STEPS_DISPLAY_WIDTH];
    
    for (uint8_t page = 0; page < (STEPS_DISPLAY_HEIGHT / 8); page++) {
        // Set addressing for this specific page
        twi_start_write(OLED_ADDRESS);
        twi_write_bytes_to_display((uint8_t[]){
            OLED_COMMAND,
            MEMORY_MODE, 0x00,          // horizontal addressing mode
            COLUMN_ADDR, 0, DISPLAY_WIDTH - 1,  
            PAGE_ADDR, page, page       // Only set current page
        }, 8);
        twi_stop();

        // Start data transmission for this page
        twi_start_write(OLED_ADDRESS);
        uint8_t data_cmd = OLED_DATA;
        twi_write_bytes_to_display(&data_cmd, 1);
        
        memset(steps_display_buffer, 0, STEPS_DISPLAY_WIDTH);
        
        for (uint8_t col = 0; col < STEPS_DISPLAY_WIDTH; col++) {
            if (page == 0 || page == 2) {  // step squares
                draw_step_square(steps_display_buffer, col, page, steps, active_step);
            } else if (page == 1 || page == 3) {  // indicator
                draw_step_indicator(steps_display_buffer, col, active_step, page);
            }
        }
        
        twi_write_bytes_to_display(steps_display_buffer, STEPS_DISPLAY_WIDTH);
        twi_stop();
    }
}

void display_divider(void) {
    // For each page, write 0xFF to set all pixels in the column
    for (uint8_t page = 0; page < (DISPLAY_HEIGHT/8); page++) {
        // Set addressing for this specific page
        twi_start_write(OLED_ADDRESS);
        twi_write_bytes_to_display((uint8_t[]){
            OLED_COMMAND,
            MEMORY_MODE, 0x00,          // horizontal addressing mode
            COLUMN_ADDR, DISPLAY_WIDTH/2, DISPLAY_WIDTH/2,  // Single column in middle
            PAGE_ADDR, page, page       // Only set current page
        }, 8);
        twi_stop();

        // Start data transmission for this page
        twi_start_write(OLED_ADDRESS);
        uint8_t data_cmd = OLED_DATA;
        twi_write_bytes_to_display(&data_cmd, 1);
        
        uint8_t line = 0xFF;  // All pixels on in this byte
        twi_write_bytes_to_display(&line, 1);
    }
    twi_stop();
}

void display_step_info(const uint8_t steps[2][4], uint8_t active_step, int16_t adcVal) {
    char info_str[16];
    uint8_t right_margin = DISPLAY_WIDTH/2 + 4;  // Start after divider with small margin
    
    // Display step number on first line
    sprintf(info_str, "Step: %d", active_step + 1);  // +1 for human-readable numbering
    
    // Set addressing for step number
    twi_start_write(OLED_ADDRESS);
    twi_write_bytes_to_display((uint8_t[]){
        OLED_COMMAND,
        MEMORY_MODE, 0x00,          // horizontal addressing mode
        COLUMN_ADDR, right_margin, DISPLAY_WIDTH - 1,
        PAGE_ADDR, 0, 0            // First line
    }, 8);
    twi_stop();
    
    // Write step number
    twi_start_write(OLED_ADDRESS);
    uint8_t data_cmd = OLED_DATA;
    twi_write_bytes_to_display(&data_cmd, 1);
    
    // Write each character
    for (char *c = info_str; *c; c++) {
        for (uint8_t i = 0; i < 5; i++) {
            uint8_t font_data = pgm_read_byte(&font[(*c) * 5 + i]);
            twi_write_bytes_to_display(&font_data, 1);
        }
        // Add space between characters
        uint8_t space = 0x00;
        twi_write_bytes_to_display(&space, 1);
    }
    twi_stop();
    
    // Display ADC value on second line
    sprintf(info_str, "ADC: %d", adcVal);
    
    // Set addressing for ADC value
    twi_start_write(OLED_ADDRESS);
    twi_write_bytes_to_display((uint8_t[]){
        OLED_COMMAND,
        MEMORY_MODE, 0x00,          // horizontal addressing mode
        COLUMN_ADDR, right_margin, DISPLAY_WIDTH - 1,
        PAGE_ADDR, 2, 2            // Third line (page 2)
    }, 8);
    twi_stop();
    
    // Write ADC value
    twi_start_write(OLED_ADDRESS);
    twi_write_bytes_to_display(&data_cmd, 1);
    
    // Write each character
    for (char *c = info_str; *c; c++) {
        for (uint8_t i = 0; i < 5; i++) {
            uint8_t font_data = pgm_read_byte(&font[(*c) * 5 + i]);
            twi_write_bytes_to_display(&font_data, 1);
        }
        // Add space between characters
        uint8_t space = 0x00;
        twi_write_bytes_to_display(&space, 1);
    }
    twi_stop();
}