#include "display.h"

static void display_start_command() // start a display command sequence
{
    twi_start_write(OLED_ADDRESS);
    uint8_t cmd = OLED_COMMAND;
    twi_write_bytes_to_display(&cmd, 1);
}

static void display_start_data() // start a display data sequence
{
    twi_start_write(OLED_ADDRESS);
    uint8_t data_cmd = OLED_DATA;
    twi_write_bytes_to_display(&data_cmd, 1);
}

static void display_send_commands(const uint8_t commands[], uint8_t length) // send a sequence of commands
{
    display_start_command();
    twi_write_bytes_to_display(commands, length);
    twi_stop();
}

static void set_display_region(uint8_t col_start, uint8_t col_end, uint8_t page_start, uint8_t page_end) // set display addressing for a specific region
{
    const uint8_t commands[] = {
        MEMORY_MODE, 0x00, // horizontal addressing mode
        COLUMN_ADDR, col_start, col_end,
        PAGE_ADDR, page_start, page_end};
    display_send_commands(commands, sizeof(commands));
}

void display_init(void)
{
    const uint8_t init_sequence[] = {
        OLED_COMMAND,
        DISPLAY_OFF,
        SET_DISPLAY_CLOCK_DIV, 0x80,
        SET_MULTIPLEX, 0x1F,
        SET_DISPLAY_OFFSET, 0x00,
        SET_START_LINE | 0x00,
        CHARGE_PUMP, 0x14,
        MEMORY_MODE, 0x00,
        SEG_REMAP | 0x01,
        COM_SCAN_DEC,
        SET_COM_PINS, 0x02,
        SET_CONTRAST, 0x7F,
        SET_PRECHARGE, 0x22,
        SET_VCOM_DETECT, 0x20,
        DISPLAY_RAM,
        SET_NORMAL_DISPLAY,
        DISPLAY_ON};

    display_send_commands(init_sequence, sizeof(init_sequence));
    set_display_region(0, DISPLAY_WIDTH - 1, 0, (DISPLAY_HEIGHT / 8) - 1);
    display_clear();
}

void display_clear(void)
{
    display_start_data();

    for (uint16_t i = 0; i < (DISPLAY_WIDTH * (DISPLAY_HEIGHT / 8)); i++)
    {
        uint8_t zero = 0x00;
        twi_write_bytes_to_display(&zero, 1);
    }
    twi_stop();
}

void write_string(const char *str, uint8_t line)
{
    set_display_region(0x00, 0x7F, line, line);
    display_clear();

    uint8_t display_buffer[128]; // width of display
    uint8_t buffer_pos = 0;

    while (*str && buffer_pos < 128) // build buffer with character data
    {
        char c = *str++;
        for (uint8_t i = 0; i < 5; i++)
        {
            display_buffer[buffer_pos++] = pgm_read_byte(&font[c * 5 + i]);
        }
        display_buffer[buffer_pos++] = 0x00; // space between chars
    }

    display_start_data();
    twi_write_bytes_to_display(display_buffer, buffer_pos);
    twi_stop();
}

void draw_step_square(uint8_t *display_buffer, uint8_t col, uint8_t page, NoteState steps[2][4], uint8_t active_step, bool blink) // draw a step square (8x8 active or inactive)
{
    uint8_t x = col - MARGIN_X;
    uint8_t step_col = x / COL_SPACING;
    uint8_t x_in_step = x % COL_SPACING;

    if (step_col < 4 && x_in_step < STEP_SIZE)
    {
        uint8_t step_row = (page == 2) ? 1 : 0;
        uint8_t step_active = steps[step_row][step_col].active;
        uint8_t is_active = (step_row * 4 + step_col) == active_step;

        const uint8_t *bitmap;
        if (is_active)
        {
            if (blink)
            {
                bitmap = STEP_ACTIVE_BITMAP_BLINK;
            }
            else
            {
                if (step_active)
                {
                    bitmap = STEP_ACTIVE_BITMAP_ON;
                }
                else
                {
                    bitmap = STEP_ACTIVE_BITMAP_OFF;
                }
            }
        }
        else if (step_active)
        {
            bitmap = STEP_ON_BITMAP;
        }
        else
        {
            bitmap = STEP_OFF_BITMAP;
        }

        display_buffer[col] = bitmap[x_in_step];
    }
    else
    {
        display_buffer[col] = 0x00;
    }
}

static void draw_step_indicator(uint8_t *display_buffer, uint8_t col, uint8_t active_step, uint8_t page) // draw a step indicator (tag underneath active step)
{
    uint8_t x = col - MARGIN_X;
    uint8_t step_col = x / COL_SPACING;

    bool is_valid_row = (active_step < 4 && page == 1) ||
                        (active_step >= 4 && page == 3);
    bool is_valid_col = (step_col == (active_step % 4));

    if (!is_valid_row || !is_valid_col) // if not on the correct page or active step, clear the pixel
    {
        display_buffer[col] = 0x00;
        return;
    }

    uint8_t indicator_start = MARGIN_X + (step_col * COL_SPACING) + ((STEP_SIZE - 5) / 2); // start of indicator
    uint8_t indicator_x = col - indicator_start;
    if (indicator_x < 5)
    { // font is 5 pixels wide
        char icon = 159;
        uint16_t font_index = (icon - 32) * 5 + indicator_x;
        display_buffer[col] = pgm_read_byte(&font[font_index]);
    }
    else
    {
        display_buffer[col] = 0x00;
    }
}

void display_step_sequence(NoteState steps[2][4], uint8_t active_step, bool blink) // draw the step sequence grid
{
    uint8_t steps_display_buffer[STEPS_DISPLAY_WIDTH];

    for (uint8_t page = 0; page < (STEPS_DISPLAY_HEIGHT / 8); page++)
    {
        set_display_region(0, DISPLAY_WIDTH - 1, page, page);
        display_start_data();

        memset(steps_display_buffer, 0, STEPS_DISPLAY_WIDTH);
        for (uint8_t col = 0; col < STEPS_DISPLAY_WIDTH; col++)
        {
            if (page == 0 || page == 2)
            { // step squares
                draw_step_square(steps_display_buffer, col, page, steps, active_step, blink);
            }
            else if (page == 1 || page == 3)
            { // indicator
                draw_step_indicator(steps_display_buffer, col, active_step, page);
            }
        }

        twi_write_bytes_to_display(steps_display_buffer, STEPS_DISPLAY_WIDTH);
        twi_stop();
    }
}

void display_divider(void) // draw a divider between the step sequence grid and the step info
{
    for (uint8_t page = 0; page < (DISPLAY_HEIGHT / 8); page++)
    {
        set_display_region(DISPLAY_WIDTH / 2, DISPLAY_WIDTH / 2, page, page);
        display_start_data();

        uint8_t line = 0xFF;
        twi_write_bytes_to_display(&line, 1);
    }
    twi_stop();
}

void display_step_info(bool is_playing, uint8_t active_step, const char *descriptor, const char *formatted_value, int raw_value)
{
    char info_str[16];
    uint8_t right_margin = DISPLAY_WIDTH / 2 + 4; // margin after divider

    set_display_region(right_margin, DISPLAY_WIDTH - 1, 0, 0);
    display_start_data();

    // line 1: step number
    sprintf(info_str, "step%d ", active_step + 1); // fix for 0 index
    for (char *c = info_str; *c; c++)              // write each character
    {
        for (uint8_t i = 0; i < 5; i++)
        {
            uint8_t font_data = pgm_read_byte(&font[(*c) * 5 + i]);
            twi_write_bytes_to_display(&font_data, 1);
        }
        uint8_t space = 0x00;
        twi_write_bytes_to_display(&space, 1);
    }
    twi_stop();

    // line 3: adc value
    if (formatted_value) // if there is a formatted value like note name
    {
        int value_len = strlen(formatted_value);
        char format_str[10];
        switch (value_len)
        {
        case 2:                               // "C4"
            sprintf(format_str, "%%s:%%s  "); // 3 spaces
            break;
        case 3:                              // "D#3"
            sprintf(format_str, "%%s:%%s "); // 2 spaces
            break;
        default:
            sprintf(format_str, "%%s:%%s "); // 1 space
            break;
        }

        sprintf(info_str, format_str, descriptor, formatted_value);
    }
    else // if there is a numerical value like velocity
    {
        int temp = abs(raw_value);
        int digit_count = (raw_value <= 0) ? 1 : 0;
        while (temp > 0)
        {
            digit_count++;
            temp /= 10;
        }

        // Add appropriate number of leading spaces

        // Add appropriate number of leading spaces
        char format_str[10];
        switch (digit_count)
        {
        case 1:
            sprintf(format_str, "%%s:%%d   "); // 3 leading spaces
            break;
        case 2:
            sprintf(format_str, "%%s:%%d  "); // 2 leading spaces
            break;
        default:
            sprintf(format_str, "%%s:%%d "); // No leading spaces
            break;
        }
        sprintf(info_str, format_str, descriptor, raw_value);
    }

    set_display_region(right_margin, DISPLAY_WIDTH - 1, 2, 2);
    display_start_data();

    // Write each character
    for (char *c = info_str; *c; c++)
    {
        for (uint8_t i = 0; i < 5; i++)
        {
            uint8_t font_data = pgm_read_byte(&font[(*c) * 5 + i]);
            twi_write_bytes_to_display(&font_data, 1);
        }
        // Add space between characters
        uint8_t space = 0x00;
        twi_write_bytes_to_display(&space, 1);
    }
    twi_stop();

    // line 4: play/pause indicator
    const uint8_t *icon = is_playing ? PLAY_ICON : PAUSE_ICON;
    set_display_region(DISPLAY_WIDTH - 10, DISPLAY_WIDTH - 2, 3, 3);
    display_start_data();
    twi_write_bytes_to_display((uint8_t *)icon, 8);
    twi_stop();
}
