#include "connection.h"

void bleInit() {
    // Put BLE Radio in "Application Mode" by driving F3 high
    PORTF.DIRSET = PIN3_bm;
    PORTF.OUTSET = PIN3_bm;

    // Reset BLE Module - pull PD3 low, then back high after a delay
    PORTD.DIRSET = PIN3_bm | PIN2_bm;
    PORTD.OUTCLR = PIN3_bm;
    _delay_ms(10); // Leave reset signal pulled low
    PORTD.OUTSET = PIN3_bm;

    // The AVR-BLE hardware guide is wrong. Labels this as D3
    // Tell BLE module to expect data - set D2 low
    PORTD.OUTCLR = PIN2_bm;
    _delay_ms(200); // Give time for RN4870 to boot up

    char buf[BUF_SIZE];
    // Put RN4870 in Command Mode
    usartWriteCommand("$$$");
    usartReadUntil(buf, BLE_RADIO_PROMPT);

    // Change BLE device name to specified value
    // There can be some lag between updating name here and
    // seeing it in the LightBlue phone interface
    strcpy(buf, "S-,");
    strcat(buf, "step-seq");
    strcat(buf, "\r\n");
    usartWriteCommand(buf);
    usartReadUntil(buf, BLE_RADIO_PROMPT);

    strcpy(buf, "PZ\r\n");
    usartWriteCommand(buf);
    usartReadUntil(buf, BLE_RADIO_PROMPT);

    usartWriteCommand("SN,step-seq\r\n");
    usartReadUntil(buf, BLE_RADIO_PROMPT);

    // manufacturer name
    usartWriteCommand("SDN,codemaxxer inc.\r\n");
    usartReadUntil(buf, BLE_RADIO_PROMPT);

    setupServiceAndCharacteristic();

    listServicesAndCharacteristics();

    listClientInformation();
}

void setupServiceAndCharacteristic() {
    // add MIDI service
    char buf[BUF_SIZE];
    strcpy(buf, "PS,");
    strcat(buf, MIDI_SERVICE_UUID);
    strcat(buf, "\r\n");
    usartWriteCommand(buf);
    usartReadUntil(buf, BLE_RADIO_PROMPT);

    // add MIDI characteristic
    strcpy(buf, "PC,");
    strcat(buf, MIDI_CHARACTERISTIC_UUID);
    strcat(buf, ",FF,10\r\n");
    usartWriteCommand(buf);
    usartReadUntil(buf, BLE_RADIO_PROMPT);

    strcpy(buf, "SHW,0072,00\r\n");
    usartWriteCommand(buf);
    usartReadUntil(buf, BLE_RADIO_PROMPT);
}

void usartInit() {
    PORTA.DIR &= ~PIN1_bm;
    PORTA.DIR |= PIN0_bm;

    USART0.BAUD = (uint16_t)USART_BAUD_VALUE(115200);
    // USART0.BAUD = (uint16_t)USART_BAUD_VALUE(9600);

    USART0.CTRLB |= USART_TXEN_bm; 
    USART0.CTRLB |= USART_RXEN_bm; 
    USART0.CTRLB |= USART_RXMODE_NORMAL_gc; 
    
    USART0.CTRLC |= USART_CMODE_ASYNCHRONOUS_gc;
    USART0.CTRLC |= USART_CHSIZE_8BIT_gc; 
}

void usartWriteChar(char c) {
    while (!(USART0.STATUS & USART_DREIF_bm)){;}     
    USART0.TXDATAL = c;
}

void usartWriteCommand(const char *cmd) {
    char *cmd_copy = strdup(cmd);
    // Print the command being sent
    serialPrintF("[me] command: ");
    serialPrintF(cmd_copy);
    serialPrintF("\r\n");

    // Send each character of the command
    for (uint8_t i = 0; cmd[i] != '\0'; i++) {
        // char print_string[32];
        // sprintf(print_string, "%c", cmd[i]);
        // serialPrintF(print_string);
        usartWriteChar(cmd[i]);
    }
}

char usartReadChar() {
    while (!(USART0.STATUS & USART_RXCIF_bm)){;}
    return USART0.RXDATAL;
}

void usartReadUntil(char *dest, const char *end_str) {
    memset(dest, 0, BUF_SIZE);
    uint8_t end_len = strlen(end_str);
    uint8_t bytes_read = 0;
    uint32_t max_attempts = 100000; // Set a maximum number of attempts

    while (bytes_read < BUF_SIZE - 1 && max_attempts > 0) {
        if (USART0.STATUS & USART_RXCIF_bm) {
            dest[bytes_read] = usartReadChar();
            bytes_read++;

            if (bytes_read >= end_len && strcmp(dest + bytes_read - end_len, end_str) == 0) {
                break;
            }
        } else {
            _delay_us(1); // Small delay to prevent busy-waiting
        }
        max_attempts--; // Decrement the attempts counter
    }
    dest[bytes_read] = '\0';

    if (max_attempts > 0) {
        serialPrintF("[ble] response: ");
        serialPrintF(dest);
        serialPrintF("\r\n");
    }
}

// void usartReadUntil(char *dest, const char *end_str) {
//     serialPrintF("waiting for response\r\n");

//     memset(dest, 0, BUF_SIZE);
//     uint8_t end_len = strlen(end_str);
//     uint8_t bytes_read = 0;
//     while (bytes_read < end_len || strcmp(dest + bytes_read - end_len, end_str) != 0) {
//         dest[bytes_read] = usartReadChar();
//         bytes_read++;
//     }

//     serialPrintF("got response: ");
//     serialPrintF(dest);
//     serialPrintF("\r\n\n");
// }

void bleWriteCharacteristic(char *characteristicUUID, uint8_t *data, size_t length) {
    // Convert the UUID and data into a command format suitable for your BLE module
    char command[64];
    sprintf(command, "WRITE,%s,", characteristicUUID);

    for (size_t i = 0; i < length; i++) {
        sprintf(command + strlen(command), "%02X", data[i]);
    }

    usartWriteCommand(command);
}

void listServicesAndCharacteristics() {
    char buf[BUF_SIZE];
    usartWriteCommand("LS\r\n");
    usartReadUntil(buf, BLE_RADIO_PROMPT);
}

void getConnectionStatus() {
    char buf[BUF_SIZE];
    usartWriteCommand("GK\r\n");
    usartReadUntil(buf, BLE_RADIO_PROMPT);
}

void startAdvertising() {
    char buf[BUF_SIZE];
    usartWriteCommand("A,0050,005E\r\n"); // 80 ms interval for 60 seconds
    usartReadUntil(buf, BLE_RADIO_PROMPT);
}

void stopAdvertising() {
    char buf[BUF_SIZE];
    usartWriteCommand("Y\r\n");
    usartReadUntil(buf, BLE_RADIO_PROMPT);
}

void bond() {
    char buf[BUF_SIZE];
    usartWriteCommand("B\r\n");
    usartReadUntil(buf, BLE_RADIO_PROMPT);
}

void initializeClientOperation() {
    char buf[BUF_SIZE];
    usartWriteCommand("CI\r\n");
    usartReadUntil(buf, BLE_RADIO_PROMPT);
}

void listClientInformation() {
    char buf[BUF_SIZE];
    
    // Check connection status first
    usartWriteCommand("GK\r\n");
    usartReadUntil(buf, BLE_RADIO_PROMPT);
    
    initializeClientOperation();
    
    // Small delay to allow client initialization
    _delay_ms(1000);
    
    // Now try to list services
    // usartWriteCommand("LC\r\n");
    // usartReadUntil(buf, BLE_RADIO_PROMPT);
    
    // usartWriteCommand("LC,180A\r\n");
    // usartReadUntil(buf, BLE_RADIO_PROMPT);
    
    // Query details for the custom services
    // usartWriteCommand("LC,D0611E78BBB44591A5F8487910AE4366\r\n");
    // usartReadUntil(buf, BLE_RADIO_PROMPT);
    
    // usartWriteCommand("LC,9FA480E0496745429390D343DC5D04AE\r\n");
    // usartReadUntil(buf, BLE_RADIO_PROMPT);
    
    usartWriteCommand("LC,03B80E5AEDE84B33A7516CE34EC4C700\r\n");
    usartReadUntil(buf, BLE_RADIO_PROMPT);

    // After discovering services, enable MIDI notifications
    enableMidiNotifications();

    sendMidiNoteOn(0, 60, 100);  // Channel 0, middle C, velocity 100
    _delay_ms(500);              // Hold note for 500ms
    sendMidiNoteOff(0, 60);      // Release note
}

void readBleData() {
    char buf[BUF_SIZE];
    usartReadUntil(buf, BLE_RADIO_PROMPT);

    if (strlen(buf) > 0) {
        serialPrintF("[central] data: ");
        serialPrintF(buf);
        serialPrintF("\r\n");

        // Check for the %WC message and extract handle and config values
        char *wc_start = strstr(buf, "%WC,");
        if (wc_start != NULL) {
            serialPrintF("got %WC\r\n");
            enableNotifications(0x0072, 0x0100);
            // uint16_t handle;
            // uint16_t configValue;
            // if (sscanf(wc_start, "%%WC,%4hx,%4hx", &handle, &configValue) == 2) {
            //     enableNotifications(handle, configValue);
            // }
        }
    }
}

void enableNotifications(uint16_t handle, uint16_t configValue) {
    char buf[BUF_SIZE];
    strcpy(buf, "CHW,");
    sprintf(buf + strlen(buf), "%04X", handle);
    sprintf(buf + strlen(buf), ",%04X", configValue);
    serialPrintF("[me] enabling notifications: ");
    serialPrintF(buf);
    serialPrintF("\r\n");

    usartWriteCommand(buf);
    usartReadUntil(buf, BLE_RADIO_PROMPT);
}

// DEBUGGING USE
void serialInit(void) {
    PORTF.DIRSET = PIN0_bm;
    PORTF.DIRCLR = PIN1_bm;
    USART2.BAUD = (uint16_t)USART_BAUD_VALUE(9600);
    USART2.CTRLB |= USART_TXEN_bm;
}

// DEBUGGING USE
void serialPrintF(char *str) {
    for(size_t i = 0; i < strlen(str); i++) {
        while (!(USART2.STATUS & USART_DREIF_bm));      
        USART2.TXDATAL = str[i];
    }
}

#define MIDI_HANDLE "001B"
#define MIDI_CONFIG_HANDLE "001C"

void writeMidiData(uint8_t *midiData, size_t length) {
    char buf[BUF_SIZE];
    
    // Build the write command: SHW,handle,data
    strcpy(buf, "CHW,");
    strcat(buf, MIDI_HANDLE);
    strcat(buf, ",");
    
    // Convert MIDI data bytes to hex string
    for (size_t i = 0; i < length; i++) {
        char hex[3];
        sprintf(hex, "%02X", midiData[i]);
        strcat(buf, hex);
    }
    strcat(buf, "\r\n");
    
    // Send the command
    usartWriteCommand(buf);
    usartReadUntil(buf, BLE_RADIO_PROMPT);
}

void enableMidiNotifications() {
    initializeClientOperation();
    _delay_ms(200);

    char buf[BUF_SIZE];

    // Enable notifications by writing 0x0001 to the config handle
    strcpy(buf, "CHW,");
    strcat(buf, MIDI_CONFIG_HANDLE);
    strcat(buf, ",0100\r\n");

    usartWriteCommand(buf);
    usartReadUntil(buf, BLE_RADIO_PROMPT);
    
    // Enable notifications by writing 0x0001 to the config handle
    strcpy(buf, "CHW,");
    strcat(buf, MIDI_CONFIG_HANDLE);
    strcat(buf, ",0100\r\n");

    usartWriteCommand(buf);
    usartReadUntil(buf, BLE_RADIO_PROMPT);

    _delay_ms(1000);
    
    // Enable indications by writing 0x0002 to the config handle
    strcpy(buf, "CHW,");
    strcat(buf, MIDI_HANDLE);
    strcat(buf, ",0200\r\n");
    
    usartWriteCommand(buf);
    usartReadUntil(buf, BLE_RADIO_PROMPT);
}

// Example usage function
void sendMidiNoteOn(uint8_t channel, uint8_t note, uint8_t velocity) {
    uint8_t midiData[] = {
        0x00,  // Header byte (timestamp high)
        0x00,  // Header byte (timestamp low)
        0x90 | (channel & 0x0F),  // Note On status byte
        note & 0x7F,              // Note number
        velocity & 0x7F           // Velocity
    };
    
    writeMidiData(midiData, sizeof(midiData));
}

void sendMidiNoteOff(uint8_t channel, uint8_t note) {
    uint8_t midiData[] = {
        0x00,  // Header byte (timestamp high)
        0x00,  // Header byte (timestamp low)
        0x80 | (channel & 0x0F),  // Note Off status byte
        note & 0x7F,              // Note number
        0x00                      // Velocity (0 for note off)
    };
    
    writeMidiData(midiData, sizeof(midiData));
}
