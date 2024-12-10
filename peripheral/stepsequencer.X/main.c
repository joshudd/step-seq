#include "main.h"

volatile bool ble_connected = false;

volatile bool red_down = false;
volatile bool yellow_down = false;
volatile int red_held_counter = 0;
volatile int yellow_held_counter = 0;
volatile bool handle_red = false;
volatile bool handle_yellow = false;

int currentStep = 0; // index of the current step

bool unlock_settings = false;
bool blink = false;
int blink_counter = 0;

enum SettingsMode {
    SETTINGS_MODE_NOTE,
    SETTINGS_MODE_VELOCITY,
    SETTINGS_MODE_DURATION,
    SETTINGS_MODE_ACTIVE
};

enum SettingsMode settings_mode = SETTINGS_MODE_NOTE;

NoteState sequence[2][4] = {
    {{60, 2, 80, 100, true}, {62, 4, 60, 100, false}, {64, 4, 100, 100, true}, {65, 4, 60, 100, true}}, 
    {{67, 4, 60, 100, true}, {69, 4, 60, 100, true}, {71, 4, 60, 100, false}, {72, 4, 60, 100, false}}
};

// settings step state
uint8_t current_step_row = 0; 
uint8_t current_step_col = 0; 

// playing step state
uint8_t current_play_row = 0;
uint8_t current_play_col = 0;

// previous playing step state
static int previous_play_row = -1;
static int previous_play_col = -1;


SettingInfo getSettingInfo(int row, int col) {
    NoteState* note = &sequence[row][col];
    SettingInfo info = {0};

    switch (settings_mode) {
        case SETTINGS_MODE_NOTE:
            info = (SettingInfo){
                .descriptor = "note",
                .value = note->note,
                .min_value = 0,
                .max_value = 127,
                .format_value = getMidiNoteName
            };
            break;
            
        case SETTINGS_MODE_VELOCITY:
            info = (SettingInfo){
                .descriptor = "velo",
                .value = note->velocity,
                .min_value = 0,
                .max_value = 127,
                .format_value = NULL
            };
            break;
            
        case SETTINGS_MODE_DURATION:
            info = (SettingInfo){
                .descriptor = "lgth",
                .value = note->duration,
                .min_value = 1,
                .max_value = 100,
                .format_value = NULL
            };
            break;
            
        case SETTINGS_MODE_ACTIVE:
            info = (SettingInfo){
                .descriptor = "actv",
                .value = note->active,
                .min_value = 0,
                .max_value = 1,
                .format_value = NULL
            };
            break;
    }
    return info;
}

volatile int16_t adcVal;
volatile uint8_t adc_enabled;
volatile uint8_t adc_counter = 0;
volatile int8_t adc_delta = 0;
volatile bool adc_value_ready = false;

volatile int16_t initial_adc_value = -1;  // -1 indicates not set
volatile int initial_setting_value = -1;

volatile bool is_playing = false; // is sequence running

/**
 * Interrupt Service Routine for the ADC
 */
ISR(ADC0_RESRDY_vect)
{
    ADC0.INTFLAGS = ADC_RESRDY_bm;
    adcVal = 255 - ADC0.RES;
    adc_value_ready = true;
}

/**
 * Interrupt Service Routine for the red and yellow buttons
 */
ISR(PORTA_PORT_vect) {
    if (RED_INTERRUPT) {
        _delay_ms(10);
        if (PORTA.IN & RED_BUTTON) {
            // rising edge
            red_down = false;
            handle_red = true;
        } else {
            red_down = true;
        }

        RED_INTERRUPT_CLEAR;
    } else if (YELLOW_INTERRUPT) {
        _delay_ms(10);
        if (PORTA.IN & YELLOW_BUTTON) {
            yellow_down = false;
            handle_yellow = true;
        } else {
            yellow_down = true;
        }

        YELLOW_INTERRUPT_CLEAR;
    }
}

/**
 * Interrupt Service Routine for the RTC (every 10ms)
 */
ISR(RTC_CNT_vect) {
    if(RTC.INTFLAGS & RTC_OVF_bm) { // check for OVF interrupt
        RTC.INTFLAGS = RTC_OVF_bm; // delete OVF interrupt flag

        if (is_playing) {
            playStep(sequence[current_play_row][current_play_col]);
            incrementPlayStep();
        }
    }
}

void incrementPlayStep() {
    current_play_col = (current_play_col + 1) % 4;
    if (current_play_col == 0) {
        current_play_row = (current_play_row + 1) % 2;
    }
}

void handleRedButton() {
    if (red_down) {
        red_held_counter++;

        if (red_held_counter == 15) {
            is_playing = !is_playing; // toggle play state on red button hold
            char print_string[32];
            sprintf(print_string, "[central] sequence %s\r\n", is_playing ? "playing" : "paused");
            serialPrintF(print_string);
        }
    }

    if (handle_red) {
        if (red_held_counter < 15) {
            if (!unlock_settings) {
                current_step_col = (current_step_col + 1) % 4;
                if (current_step_col == 0) {
                    current_step_row = (current_step_row + 1) % 2;
                }

                display_clear();
            }

            // playStep(sequence[current_play_row][current_play_col]);
        }

        if (!red_down) {
            red_held_counter = 0;
        }
        handle_red = false;
    }

    _delay_ms(1);
}

void handleYellowButton() {
    if (yellow_down) {
        yellow_held_counter++;

        if (yellow_held_counter == 15) {
            char print_string[32];
            unlock_settings = !unlock_settings;

            adc_enabled = !adc_enabled;
            if (adc_enabled) {
                initial_adc_value = -1;
                initial_setting_value = -1;
                ADC0_start();
            } else {
                ADC0_stop();
            }
        }
    }

    if (handle_yellow) {
        if (yellow_held_counter < 15) {
            settings_mode = (settings_mode + 1) % 4;  // 4 is the number of settings modes
            initial_adc_value = -1;
            initial_setting_value = -1;
            display_clear();
        }

        if (!yellow_down) {
            yellow_held_counter = 0;
        }
        handle_yellow = false;
    }

    _delay_ms(1);
}

void playStep(NoteState step) {
    if (step.note < 0 || step.note > 127) {
        serialPrintF("[error] Invalid note value\r\n");
        return;
    }

    if (step.velocity < 0 || step.velocity > 127) {
        serialPrintF("[error] Invalid velocity value\r\n");
        return;
    }

    int stepIndex = current_play_col + current_play_row * 4;

    char print_string[32];  
    sprintf(print_string, "[main] playing step %d\r\n", stepIndex+1);
    serialPrintF(print_string);

    // Always send note off if the previous step was active
    if (previous_play_col != -1 && previous_play_row != -1) {
        NoteState previousStep = sequence[previous_play_row][previous_play_col];
        if (previousStep.active) {
            send_midi_note(MIDI_NOTE_OFF, previousStep.note, 0); // turn off the previous note
        }
    }

    if (step.active) {
        // Send note on for the current step
        send_midi_note(MIDI_NOTE_ON, step.note, step.velocity); // turn on the new note
        previous_play_row = current_play_row;
        previous_play_col = current_play_col;
    } else {
        // if the step is not active and it matches the currently active step index, send note off
        if (previous_play_col == current_play_col && previous_play_row == current_play_row) {
            send_midi_note(MIDI_NOTE_OFF, step.note, 0);
            previous_play_row = -1;
            previous_play_col = -1;
        }
    }
}


void adjustSettingValue(int row, int col, int delta) {

    NoteState* note = &sequence[row][col];
    SettingInfo info = getSettingInfo(row, col);
    
    // Get the current value directly from the note structure
    int current_value;
    switch (settings_mode) {
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
    
    // Calculate new value using the actual current value
    int new_value = current_value + delta;
    new_value = (new_value < info.min_value) ? info.min_value : 
                (new_value > info.max_value) ? info.max_value : 
                new_value;
    
    // Update the note structure
    switch (settings_mode) {
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

void displayUpdate() {
    // get step info
    uint8_t current_step = current_step_row * 4 + current_step_col;
    SettingInfo info = getSettingInfo(current_step_row, current_step_col);
    
    // display update
    display_step_sequence(sequence, current_step, blink);
    display_divider();
    if (info.format_value) {
        const char* formatted = info.format_value(info.value);
        display_step_info(current_step, info.descriptor, formatted, info.value);
    } else {
        display_step_info(current_step, info.descriptor, NULL, info.value);
    }
}

void adcUpdate() {
    if (adc_enabled && adc_value_ready) {
        
        SettingInfo info = getSettingInfo(current_step_row, current_step_col);
        
        if (initial_adc_value == -1) {
            initial_adc_value = adcVal;
            initial_setting_value = info.value;
        } else {
            float scale_factor = (float)(info.max_value - info.min_value) / 255.0;
            int adc_delta = adcVal - initial_adc_value;
            int new_value = initial_setting_value + (int)(adc_delta * scale_factor);

            if (adcVal == 0) {
                new_value = info.min_value;
                initial_adc_value = adcVal;
                initial_setting_value = info.min_value;
            } else if (adcVal == 255) {
                new_value = info.max_value;
                initial_adc_value = adcVal;
                initial_setting_value = info.max_value;
            }

            new_value = (new_value < info.min_value) ? info.min_value : 
                        (new_value > info.max_value) ? info.max_value : 
                        new_value;

            if (new_value != info.value) {
                adjustSettingValue(current_step_row, current_step_col, new_value - info.value);
            }
        }
        
        adc_value_ready = false;
    }
}

void setup() {
    serialInit(); // DEBUGGING USE
    
    usartInit();
    bleInit();
    setupButtons();
    rtc_init();
    twi_init();
    display_init();
    ADC0_init();

    sei();
}

int main() {
    setup();

    int ble_count = 0;

    while (!ble_connected) {
        readBleData();
        _delay_ms(10);
    }
    serialPrintF("[ble] connected.\r\n");


    serialPrintF("\n[main] ready to play.\r\n");
    while (1) {
        // adc handling
        adcUpdate();

        // blink if settings are unlocked
        if (unlock_settings) {
            blink_counter++;
            if (blink_counter >= 10) {
                blink = !blink;
                blink_counter = 0;
            }
        } else {
            blink = false;
        }

        // display update
        displayUpdate();

        // button handling
        handleRedButton();
        handleYellowButton();

        // if (ble_count > 100) { // probably should be reading but the lag is annoying
        //     readBleData(); // check for acks
        //     ble_count = 0;
        // } else {
        //     ble_count++;
        // }
    }
}
