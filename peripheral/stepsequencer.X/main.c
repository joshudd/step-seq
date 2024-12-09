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

/**
 * Interrupt Service Routine for the red and yellow buttons
 */
ISR(PORTA_PORT_vect) {
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

        listClientInformation();

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
        
        if (initializeClientRole()) {
            // After initializing client role, discover services
            serialPrintF("[me] Discovering services...\r\n");
            usartWriteCommand("CI\r\n");
            
            char response[BUF_SIZE];
            usartReadUntil(response, BLE_RADIO_PROMPT);
            
            if (strstr(response, "AOK") != NULL) {
                // Now we can read/write characteristics
                _delay_ms(100); // Give time for service discovery
                readCharacteristic(0x0073);
                _delay_ms(100);
                writeCharacteristic(0x0073, "1");
            } else {
                serialPrintF("[ble] Failed to discover services\r\n");
            }
        }

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

const uint8_t font5x7[] PROGMEM = {
    // Space
    0x00, 0x00, 0x00, 0x00, 0x00,
    // H
    0x7F, 0x08, 0x08, 0x08, 0x7F,
    // e
    0x38, 0x54, 0x54, 0x54, 0x18,
    // l
    0x7F, 0x40, 0x40, 0x40, 0x40,
    // o
    0x38, 0x44, 0x44, 0x44, 0x38,
    // W
    0x7F, 0x20, 0x10, 0x20, 0x7F,
    // r
    0x7C, 0x08, 0x04, 0x04, 0x08,
    // d
    0x38, 0x44, 0x44, 0x44, 0x7F,
    // !
    0x00, 0x00, 0x7D, 0x00, 0x00
};

#define OLED_ADDRESS 0x3C  // Common I2C address for 0.91" OLED displays
#define OLED_COMMAND 0x00  // Command byte
#define OLED_DATA 0x40    // Data byte

// Display commands
#define DISPLAY_ON 0xAF
#define DISPLAY_OFF 0xAE
#define SET_CONTRAST 0x81
#define SET_NORMAL_DISPLAY 0xA6
#define SET_DISPLAY_OFFSET 0xD3
#define SET_COM_PINS 0xDA
#define SET_VCOM_DETECT 0xDB
#define SET_DISPLAY_CLOCK_DIV 0xD5
#define SET_PRECHARGE 0xD9
#define SET_MULTIPLEX 0xA8
#define SET_START_LINE 0x40
#define MEMORY_MODE 0x20
#define COLUMN_ADDR 0x21
#define PAGE_ADDR 0x22
#define COM_SCAN_INC 0xC0
#define COM_SCAN_DEC 0xC8
#define SEG_REMAP 0xA0
#define CHARGE_PUMP 0x8D

void init_display(void) {
    serialPrintF("[debug] Initializing display...\r\n");
    
    // Add power-on delay
    _delay_ms(100);
    
    twi_start_write(OLED_ADDRESS);
    twi_write_bytes_to_display((uint8_t[]){
        OLED_COMMAND,
        DISPLAY_OFF,
        SET_DISPLAY_CLOCK_DIV, 0x80,
        SET_MULTIPLEX, 0x1F,        // 1/32 duty
        SET_DISPLAY_OFFSET, 0x00,
        SET_START_LINE | 0x00,
        CHARGE_PUMP, 0x14,          // Enable charge pump
        MEMORY_MODE, 0x00,          // Horizontal addressing mode
        SEG_REMAP | 0x01,          // Rotate display 180
        COM_SCAN_DEC,
        SET_COM_PINS, 0x02,
        SET_CONTRAST, 0x8F,         // Reduced contrast
        SET_PRECHARGE, 0xF1,
        SET_VCOM_DETECT, 0x40,
        SET_NORMAL_DISPLAY,
        DISPLAY_ON
    }, 18);
    twi_stop();
    
    // Add initialization delay
    _delay_ms(100);
    
    // Clear display
    twi_start_write(OLED_ADDRESS);
    twi_write_bytes_to_display((uint8_t[]){
        OLED_COMMAND,
        COLUMN_ADDR, 0, 127,        // Column start and end
        PAGE_ADDR, 0, 3            // Page start and end
    }, 7);
    twi_stop();
    
    // Fill with zeros to clear the display
    twi_start_write(OLED_ADDRESS);
    uint8_t command = OLED_DATA;
    twi_write_bytes_to_display(&command, 1);
    
    uint8_t zero = 0x00;
    for(int i = 0; i < 512; i++) {  // 128 * 4 pages = 512 bytes
        twi_write_bytes_to_display(&zero, 1);
    }
    twi_stop();
    
    _delay_ms(100);
    serialPrintF("[debug] Display initialized.\r\n");
}

void write_string(const char *str) {
    serialPrintF("[debug] Writing string to display...\r\n");
    twi_start_write(OLED_ADDRESS);
    twi_write_bytes_to_display((uint8_t[]){
        OLED_COMMAND,
        COLUMN_ADDR, 0, 127,        // Column start and end
        PAGE_ADDR, 0, 3            // Page start and end
    }, 7);
    twi_stop();

    twi_start_write(OLED_ADDRESS);
    uint8_t command = OLED_DATA;
    twi_write_bytes_to_display(&command, 1);
    
    while (*str) {
        for (uint8_t i = 0; i < 5; i++) {
            uint8_t line = pgm_read_byte(&font5x7[(*str - 32) * 5 + i]);
            twi_write_bytes_to_display(&line, 1);
        }
        uint8_t space = 0x00;
        twi_write_bytes_to_display(&space, 1);
        str++;
    }
    twi_stop();
    serialPrintF("[debug] String written to display.\r\n");
}

void setup() {
    serialInit(); // DEBUGGING USE
    
    usartInit();
    // bleInit();
    setupButtons();
    rtc_init();
    twi_init();
    
    sei();
}

int main() {
    setup();
    serialPrintF("[debug] Starting...\r\n");
    
    _delay_ms(100);  // Power-on delay
    
    init_display();
    _delay_ms(100);  // Wait after init
    
    // Test pattern - write alternating lines
    write_string("Test 123");
    _delay_ms(2000);
    
    while (1) {
        handleRedButton();
        handleYellowButton();
        
        // Toggle display every few seconds for testing
        static uint32_t last_toggle = 0;
        static bool display_on = true;
        
        if ((last_toggle++ % 5000) == 0) {
            twi_start_write(OLED_ADDRESS);
            twi_write_bytes_to_display((uint8_t[]){
                OLED_COMMAND,
                display_on ? DISPLAY_OFF : DISPLAY_ON
            }, 2);
            twi_stop();
            display_on = !display_on;
        }
    }
}