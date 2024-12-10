#include "main.h"

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

// Step steps[NUM_STEPS] = {
//     {true, 60, 127},  // Middle C, max velocity
//     {false, 62, 0},   // D, off
//     {true, 64, 100},  // E, velocity 100
//     {false, 65, 0},   // F, off
//     {true, 67, 127},  // G, max velocity
//     {false, 69, 0},   // A, off
//     {true, 71, 90},   // B, velocity 90
//     {false, 72, 0}    // C, off
// };

NoteState sequence[2][4] = {
    {{60, 2, 80, 100, true}, {62, 4, 0, 100, false}, {64, 4, 100, 100, true}, {65, 4, 0, 100, false}},     // Top row
    {{67, 4, 30, 100, false}, {69, 4, 0, 100, false}, {71, 4, 90, 100, true}, {72, 4, 0, 100, true}}      // Bottom row
};

// settings step state
uint8_t current_step_row = 0; 
uint8_t current_step_col = 2; 

// playing step state
uint8_t current_play_row = 0;
uint8_t current_play_col = 2;

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
                .descriptor = "durn",
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

volatile int16_t adcVal;     // Current ADC value
volatile uint8_t adc_enabled; // Flag for ADC state
volatile uint8_t adc_counter = 0;
volatile int8_t adc_delta = 0;
volatile bool adc_value_ready = false;

volatile int16_t initial_adc_value = -1;  // -1 indicates not set
volatile int initial_setting_value = -1;

// Simplify the ISR to just grab the value and set a flag
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
    if(RTC.INTFLAGS & RTC_OVF_bm){ // check for OVF interrupt
        RTC.INTFLAGS = RTC_OVF_bm; // delete OVF interrupt flag

        // playStep(sequence[current_play_row][current_play_col]);

        // current_play_col = (current_play_col + 1) % 4;
        // if (current_play_col == 0) {
        //     current_play_row = (current_play_row + 1) % 2;
        // }
    }
}

void handleRedButton() {
    if (red_down) {
        red_held_counter++;

        if (red_held_counter == 15) {
            char print_string[32];
            sprintf(print_string, "[me] red button held: %d\r\n", red_held_counter);
            serialPrintF(print_string);
        }
    }

    if (handle_red) {
        if (red_held_counter < 15) {
            char print_string[32];
            sprintf(print_string, "[me] red button pressed: %d\r\n", red_held_counter);
            serialPrintF(print_string);

            if (!unlock_settings) {
                current_step_col = (current_step_col + 1) % 4;
                if (current_step_col == 0) {
                    current_step_row = (current_step_row + 1) % 2;
                }

                display_clear();
            }


            // playStep(steps[currentStep]);

            // listClientInformation();

            // startAdvertising();
            // establishConnection();
            // getConnectionStatus();

            // initateClient();
            // char buf[BUF_SIZE];
            // strcpy(buf, "SHW,0072,05\r\n");
            // usartWriteCommand(buf);
            // usartReadUntil(buf, BLE_RADIO_PROMPT);

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
            sprintf(print_string, "[me] unlock_settings: %d\r\n", unlock_settings);
            serialPrintF(print_string);

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
            char print_string[32];
            sprintf(print_string, "[me] yellow button pressed: %d\r\n", yellow_held_counter);
            serialPrintF(print_string);

            settings_mode = (settings_mode + 1) % 4;  // 4 is the number of settings modes
            initial_adc_value = -1;
            initial_setting_value = -1;
            display_clear();

            // First establish connection (if not already connected)
            // You can use the 'C' command to connect to a specific device
            // Example: usartWriteCommand("C,0,001BDC079C31\r\n");
            
            // if (initializeClientRole()) {
            //     // After initializing client role, discover services
            //     serialPrintF("[me] Discovering services...\r\n");
            //     usartWriteCommand("CI\r\n");
                
            //     char response[BUF_SIZE];
            //     usartReadUntil(response, BLE_RADIO_PROMPT);
                
            //     if (strstr(response, "AOK") != NULL) {
            //         // Now we can read/write characteristics
            //         _delay_ms(100); // Give time for service discovery
            //         readCharacteristic(0x0073);
            //         _delay_ms(100);
            //         writeCharacteristic(0x0073, "1");
            //     } else {
            //         serialPrintF("[ble] Failed to discover services\r\n");
            //     }
            // }
        }

        if (!yellow_down) {
            yellow_held_counter = 0;
        }
        handle_yellow = false;
    }

    _delay_ms(1);
}

void playStep(NoteState step) {
    // serialPrintf("Playing step %d\n", currentStep);
    if (step.active) {
        sendMidiMessage(MIDI_NOTE_ON, step.note, step.velocity);
    } else {
        sendMidiMessage(MIDI_NOTE_OFF, step.note, 0);
    }
}

void sendMidiData(uint8_t *midiData, size_t length) {
    // Function to send MIDI data over BLE
    bleWriteCharacteristic(MIDI_CHARACTERISTIC_UUID, midiData, length);
}

// void readCharacteristic(uint16_t handle) {
//     char command[BUF_SIZE];
//     sprintf(command, "SHR,%04X\r\n", handle);
//     usartWriteCommand(command);

//     char response[BUF_SIZE];
//     usartReadUntil(response, BLE_RADIO_PROMPT);
//     serialPrintF("[ble] Read response: ");
//     serialPrintF(response);
//     serialPrintF("\r\n");
// }

// void writeCharacteristic(uint16_t handle, const char *value) {
//     char command[BUF_SIZE];
//     sprintf(command, "SHW,%04X,%s\r\n", handle, value);
//     usartWriteCommand(command);

//     char response[BUF_SIZE];
//     usartReadUntil(response, BLE_RADIO_PROMPT);
//     serialPrintF("[ble] Write response: ");
//     serialPrintF(response);
//     serialPrintF("\r\n");
// }

bool initializeClientRole() {
    // Send CI command to initialize client role
    serialPrintF("[me] Initializing client role...\r\n");
    usartWriteCommand("CI\r\n");
    
    char response[BUF_SIZE];
    usartReadUntil(response, BLE_RADIO_PROMPT);
    
    // Check if initialization was successful
    if (strstr(response, "AOK") != NULL) {
        serialPrintF("[ble] Client role initialized successfully\r\n");
        return true;
    } else {
        serialPrintF("[ble] Failed to initialize client role\r\n");
        return false;
    }
}

void readCharacteristic(uint16_t handle) {
    // Send CHR command to read characteristic
    char command[BUF_SIZE];
    sprintf(command, "CHR,%04X\r\n", handle);
    usartWriteCommand(command);

    char response[BUF_SIZE];
    usartReadUntil(response, BLE_RADIO_PROMPT);
    serialPrintF("[ble] Read response: ");
    serialPrintF(response);
    serialPrintF("\r\n");
}

void writeCharacteristic(uint16_t handle, const char *value) {
    // Send CHW command to write characteristic
    char command[BUF_SIZE];
    sprintf(command, "CHW,%04X,%s\r\n", handle, value);
    usartWriteCommand(command);

    char response[BUF_SIZE];
    usartReadUntil(response, BLE_RADIO_PROMPT);
    serialPrintF("[ble] Write response: ");
    serialPrintF(response);
    serialPrintF("\r\n");
}

void enableNotifications1(uint16_t handle) {
    char command[BUF_SIZE];
    sprintf(command, "CHW,%04X,0100\r\n", handle);
    usartWriteCommand(command);

    char response[BUF_SIZE];
    usartReadUntil(response, BLE_RADIO_PROMPT);
    serialPrintF("[ble] Notification enable response: ");
    serialPrintF(response);
    serialPrintF("\r\n");
}

void initateClient() {
    char command[BUF_SIZE];
    // sprintf(command, "CHW,%04X,0100\r\n", handle);
    usartWriteCommand("CI\r\n");

    char response[BUF_SIZE];
    usartReadUntil(response, BLE_RADIO_PROMPT);
    serialPrintF("[ble] Notification enable response: ");
    serialPrintF(response);
    serialPrintF("\r\n");
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

void setup() {
    serialInit(); // DEBUGGING USE
    
    usartInit();
    setupButtons();
    rtc_init();
    twi_init();
    display_init();
    // bleInit();
    ADC0_init();

    sei();
}

int main() {
    setup();
    int16_t last_adc_value = 0;

    while (1) {
        // adc handling
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

        // blink if settings are unlocked
        if (unlock_settings) {
            blink_counter++;
            if (blink_counter >= 5) {
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
    }
}
