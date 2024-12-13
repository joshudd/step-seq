#include "main.h"

// global state management variables
volatile bool ble_connected = false;
volatile bool is_playing = false;

// state structure initialization blocks
ButtonState button_state = {0};
ADCState adc_state = {
    .initial_value = -1,
    .initial_setting_value = -1};
SequencerState sequencer_state = {
    .mode = SETTINGS_MODE_NOTE,
    .playing = {
        .previous_row = -1,
        .previous_col = -1}};
DisplayState display_state = {0};

// sequence initialization with midi parameters
NoteState sequence[SEQUENCE_ROWS][SEQUENCE_COLS] = {
    {{60, MIDI_DEFAULT_VELOCITY, 100, true}, {62, MIDI_DEFAULT_VELOCITY, 100, false}, {64, MIDI_DEFAULT_VELOCITY, 100, true}, {65, MIDI_DEFAULT_VELOCITY, 100, true}},
    {{67, MIDI_DEFAULT_VELOCITY, 100, true}, {69, MIDI_DEFAULT_VELOCITY, 100, true}, {71, MIDI_DEFAULT_VELOCITY, 100, false}, {72, MIDI_DEFAULT_VELOCITY, 100, false}}};

/**
 * Interrupt Service Routine for the ADC
 */
ISR(ADC0_RESRDY_vect)
{
    adc_state.value = 255 - adc_read();
    adc_state.value_ready = true;
}

/**
 * Interrupt Service Routine for the red and yellow buttons
 */
ISR(PORTA_PORT_vect)
{
    if (RED_INTERRUPT)
    {
        _delay_ms(10); // implement debounce delay
        if (PORTA.IN & RED_BUTTON)
        {
            // rising edge
            button_state.red_down = false;
            button_state.handle_red = true;
        }
        else
        {
            button_state.red_down = true;
        }

        RED_INTERRUPT_CLEAR;
    }
    else if (YELLOW_INTERRUPT)
    {
        _delay_ms(10); // implement debounce delay
        if (PORTA.IN & YELLOW_BUTTON)
        {
            button_state.yellow_down = false;
            button_state.handle_yellow = true;
        }
        else
        {
            button_state.yellow_down = true;
        }

        YELLOW_INTERRUPT_CLEAR;
    }
}

/**
 * Interrupt Service Routine for the RTC (every 10ms)
 */
ISR(RTC_CNT_vect)
{
    if (RTC.INTFLAGS & RTC_OVF_bm)
    {                              // check for OVF interrupt
        RTC.INTFLAGS = RTC_OVF_bm; // delete OVF interrupt flag

        if (is_playing)
        {
            play_step(sequence[sequencer_state.playing.current_row][sequencer_state.playing.current_col]);
            sequencer_state.playing.current_col = (sequencer_state.playing.current_col + 1) % 4; // increment sequence position
            if (sequencer_state.playing.current_col == 0)
            {
                sequencer_state.playing.current_row = (sequencer_state.playing.current_row + 1) % 2; // advance to next row
            }
        }
    }
}

void handle_red_button()
{
    if (button_state.red_down)
    {
        button_state.red_held_counter++;

        if (button_state.red_held_counter == BUTTON_HOLD_THRESHOLD)
        {
            is_playing = !is_playing; // toggle play state on red button hold
        }
    }

    if (button_state.handle_red)
    {
        if (button_state.red_held_counter < BUTTON_HOLD_THRESHOLD)
        {
            adc_state.initial_value = -1;
            adc_state.initial_setting_value = -1;

            // advance sequence position
            sequencer_state.editing.current_col = (sequencer_state.editing.current_col + 1) % 4;
            if (sequencer_state.editing.current_col == 0)
            {
                sequencer_state.editing.current_row = (sequencer_state.editing.current_row + 1) % 2;
            }
        }

        if (!button_state.red_down)
        {
            button_state.red_held_counter = 0;
        }
        button_state.handle_red = false;
    }

    _delay_ms(1);
}

void handle_yellow_button()
{
    if (button_state.yellow_down)
    {
        button_state.yellow_held_counter++;

        // implement settings mode toggle on hold
        if (button_state.yellow_held_counter == BUTTON_HOLD_THRESHOLD)
        {
            sequencer_state.unlock_settings = !sequencer_state.unlock_settings;
            adc_state.enabled = !adc_state.enabled;

            if (adc_state.enabled)
            {
                adc_state.initial_value = -1;
                adc_state.initial_setting_value = -1;
                adc_start();
            }
            else
            {
                adc_stop();
            }
        }
    }

    if (button_state.handle_yellow)
    {
        if (button_state.yellow_held_counter < BUTTON_HOLD_THRESHOLD)
        {
            adc_state.initial_value = -1;
            adc_state.initial_setting_value = -1;
            sequencer_state.mode = (sequencer_state.mode + 1) % 4;
        }

        if (!button_state.yellow_down)
        {
            button_state.yellow_held_counter = 0;
        }
        button_state.handle_yellow = false;
    }

    _delay_ms(1);
}

void play_step(NoteState step)
{
    // handle note-off for previous step
    if (sequencer_state.playing.previous_col != -1 && sequencer_state.playing.previous_row != -1)
    {
        NoteState previous_step = sequence[sequencer_state.playing.previous_row][sequencer_state.playing.previous_col];
        if (previous_step.active)
        {
            send_midi_note(MIDI_NOTE_OFF, previous_step.note, 0);
        }
    }

    // process current step
    if (step.active)
    {
        send_midi_note(MIDI_NOTE_ON, step.note, step.velocity);
        sequencer_state.playing.previous_row = sequencer_state.playing.current_row;
        sequencer_state.playing.previous_col = sequencer_state.playing.current_col;
    }
    else
    {
        if (sequencer_state.playing.previous_col == sequencer_state.playing.current_col &&
            sequencer_state.playing.previous_row == sequencer_state.playing.current_row)
        {
            send_midi_note(MIDI_NOTE_OFF, step.note, 0);
            sequencer_state.playing.previous_row = -1;
            sequencer_state.playing.previous_col = -1;
        }
    }
}

SettingInfo get_setting_info(int row, int col)
{
    NoteState *note = &sequence[row][col];
    SettingInfo info = {0};

    // determine setting information based on current mode
    switch (sequencer_state.mode)
    {
    case SETTINGS_MODE_NOTE:
        info = (SettingInfo){
            .descriptor = "note",
            .value = note->note,
            .min_value = MIDI_MIN_NOTE,
            .max_value = MIDI_MAX_NOTE,
            .format_value = (format_value_func)get_midi_note_name};
        break;

    case SETTINGS_MODE_VELOCITY:
        info = (SettingInfo){
            .descriptor = "velo",
            .value = note->velocity,
            .min_value = 0,
            .max_value = 127,
            .format_value = NULL};
        break;

    case SETTINGS_MODE_DURATION:
        info = (SettingInfo){
            .descriptor = "lgth",
            .value = note->duration,
            .min_value = 1,
            .max_value = 100,
            .format_value = NULL};
        break;

    case SETTINGS_MODE_ACTIVE:
        info = (SettingInfo){
            .descriptor = "actv",
            .value = note->active,
            .min_value = 0,
            .max_value = 1,
            .format_value = NULL};
        break;
    }
    return info;
}

// updates the value of a setting for a note at the specified position by the given delta
void adjust_setting_value(int row, int col, int delta)
{
    NoteState *note = &sequence[row][col];
    SettingInfo info = get_setting_info(row, col);

    // retrieve current value based on mode
    int current_value = 0;
    switch (sequencer_state.mode)
    {
    case SETTINGS_MODE_NOTE:
        current_value = note->note;
        break;
    case SETTINGS_MODE_VELOCITY:
        current_value = note->velocity;
        break;
    case SETTINGS_MODE_DURATION:
        current_value = note->duration;
        break;
    case SETTINGS_MODE_ACTIVE:
        current_value = note->active;
        break;
    }

    // calculate and clamp new value within valid range
    int new_value = current_value + delta;
    new_value = (new_value < info.min_value) ? info.min_value : (new_value > info.max_value) ? info.max_value
                                                                                             : new_value;

    // update appropriate parameter based on mode
    switch (sequencer_state.mode)
    {
    case SETTINGS_MODE_NOTE:
        note->note = new_value;
        break;
    case SETTINGS_MODE_VELOCITY:
        note->velocity = new_value;
        break;
    case SETTINGS_MODE_DURATION:
        note->duration = new_value;
        break;
    case SETTINGS_MODE_ACTIVE:
        note->active = new_value != 0;
        break;
    }
}

// updates the display with current sequence and setting information
void display_update()
{
    SettingInfo info = get_setting_info(sequencer_state.editing.current_row, sequencer_state.editing.current_col);
    uint8_t current_step = sequencer_state.editing.current_row * 4 + sequencer_state.editing.current_col;

    // render sequence visualization and current step information
    display_step_sequence(sequence, current_step, display_state.blink);
    display_divider();
    if (info.format_value)
    {
        const char *formatted = info.format_value(info.value);
        display_step_info(is_playing, current_step, info.descriptor, formatted, info.value);
    }
    else
    {
        display_step_info(is_playing, current_step, info.descriptor, NULL, info.value);
    }
}

// processes adc input to adjust the currently selected setting
void adc_update()
{
    if (adc_state.enabled && adc_state.value_ready)
    {
        SettingInfo info = get_setting_info(sequencer_state.editing.current_row, sequencer_state.editing.current_col);

        // extend range slightly beyond limits for smoother control
        int virtual_min = info.min_value - ((info.max_value - info.min_value) >> 3);
        int virtual_max = info.max_value + ((info.max_value - info.min_value) >> 3);

        // initialize reference points on first update
        if (adc_state.initial_value == -1)
        {
            adc_state.initial_value = adc_state.value;
            adc_state.initial_setting_value = info.value;
        }
        else
        {
            int adc_delta = adc_state.value - adc_state.initial_value;

            // process only significant changes
            if (abs(adc_delta) > ADC_NOISE_THRESHOLD)
            {
                float scale_factor;

                // calculate scaling based on direction of movement
                if (adc_delta > 0)
                {
                    scale_factor = (float)(virtual_max - adc_state.initial_setting_value) / (255.0 - adc_state.initial_value);
                }
                else
                {
                    scale_factor = (float)(adc_state.initial_setting_value - virtual_min) / adc_state.initial_value;
                }

                int new_value = adc_state.initial_setting_value + (int)(adc_delta * scale_factor);

                // handle potentiometer extremes
                if (adc_state.value == 0)
                {
                    new_value = info.min_value;
                }
                else if (adc_state.value == 255)
                {
                    new_value = info.max_value;
                }

                // ensure value stays within bounds
                int clamped_value = (new_value < info.min_value) ? info.min_value : (new_value > info.max_value) ? info.max_value
                                                                                                                 : new_value;

                // update if value has changed
                if (clamped_value != info.value)
                {
                    adjust_setting_value(sequencer_state.editing.current_row, sequencer_state.editing.current_col, clamped_value - info.value);

                    // store new reference points
                    adc_state.initial_setting_value = new_value;
                    adc_state.initial_value = adc_state.value;
                }
            }
        }

        adc_state.value_ready = false;
    }
}

// handles blinking state for settings mode indication
void blink_update()
{
    if (sequencer_state.unlock_settings)
    {
        display_state.blink_counter++;
        if (display_state.blink_counter >= DISPLAY_BLINK_INTERVAL)
        {
            display_state.blink = !display_state.blink;
            display_state.blink_counter = 0;
        }
    }
    else
    {
        display_state.blink = false;
    }
}

void setup()
{
    serial_init(); // DEBUGGING USE
    usart_init();
    ble_init();
    gpio_init();
    rtc_init();
    twi_init();
    display_init();
    adc_init();

    sei();
}

int main()
{
    setup();

    // wait for bluetooth connection
    write_string("connecting...", 0);
    while (!ble_connected)
    {
        read_ble_data();
        _delay_ms(10);
    }
    write_string("connected", 0);
    _delay_ms(500);
    write_string("ready to play!!", 2);
    _delay_ms(500);
    display_clear();

    // main loop
    while (1)
    {
        adc_update();
        blink_update();
        display_update();
        handle_red_button();
        handle_yellow_button();
    }
}
