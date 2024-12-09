#include "main.h"

volatile int red_released = 0;
volatile int yellow_released = 0;
int red_count = 0;
int yellow_count = 0;

int currentStep = 0; // index of the current step

Step steps[NUM_STEPS] = {
    {true, 60, 127},  // Middle C, max velocity
    {false, 62, 0},   // D, off
    {true, 64, 100},  // E, velocity 100
    {false, 65, 0},   // F, off
    {true, 67, 127},  // G, max velocity
    {false, 69, 0},   // A, off
    {true, 71, 90},   // B, velocity 90
    {false, 72, 0}    // C, off
};

uint16_t adcVal;

// Interrupt Service Routine for the ADC
ISR(ADC0_RESRDY_vect)
{
    /* Clear flag by writing '1': */
    ADC0.INTFLAGS = ADC_RESRDY_bm;
    adcVal = ADC0.RES;
    // TCA0_update();

    // just prints value of adcVal
    char print_string[32];
    sprintf(print_string, "[adc] ADC0_RESRDY_vect: %d\r\n", adcVal);
    serialPrintF(print_string);
}

/**
 * Interrupt Service Routine for the red and yellow buttons
 */
ISR(PORTA_PORT_vect) {
    serialPrintF("[me] PORTA_PORT_vect\r\n");
   if (RED_INTERRUPT) {
       red_released = 1;
       RED_INTERRUPT_CLEAR;
   } else if (YELLOW_INTERRUPT) {
       yellow_released = 1;
       YELLOW_INTERRUPT_CLEAR;
   }
}

/**
 * Interrupt Service Routine for the RTC (every 10ms)
 */
ISR(RTC_CNT_vect) {
    if(RTC.INTFLAGS & RTC_OVF_bm){ // check for OVF interrupt
        RTC.INTFLAGS = RTC_OVF_bm; // delete OVF interrupt flag
        // playStep(steps[currentStep]);
        currentStep = (currentStep + 1) % NUM_STEPS;
    }
}

void handleRedButton() {
    if (red_released) {
        char print_string[32];
        sprintf(print_string, "[me] red button pressed: %d\r\n", red_count);
        serialPrintF(print_string);

        ADC0_start();

        // listClientInformation();

        // startAdvertising();
        // establishConnection();
        // getConnectionStatus();

        // initateClient();
        // char buf[BUF_SIZE];
        // strcpy(buf, "SHW,0072,05\r\n");
        // usartWriteCommand(buf);
        // usartReadUntil(buf, BLE_RADIO_PROMPT);
        
        _delay_ms(10);
        red_released = 0;
        red_count++;
    }
}

void handleYellowButton() {
    if (yellow_released) {
        char print_string[32];
        sprintf(print_string, "[me] yellow button pressed: %d\r\n", yellow_count);
        serialPrintF(print_string);

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

        ADC0_stop();

        _delay_ms(10);
        yellow_released = 0;
        yellow_count++;
    }
}

void playStep(Step step) {
    // serialPrintf("Playing step %d\n", currentStep);
    if (step.isActive) {
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

void setup() {
    serialInit(); // DEBUGGING USE
    
    usartInit();
    // bleInit();
    setupButtons();
    rtc_init();
    twi_init();
    display_init();
    ADC0_init();

    sei();
}

int main() {
    setup();

    serialPrintF("[me] main\r\n");

    // test_display();  // Draw test line

    write_string("line 1", 0);
    write_string("line 2", 1);
    write_string("line 3", 2);
    write_string("line 4", 3);

    while (1) {
        handleRedButton();
        handleYellowButton();
        _delay_ms(100);
    }
}