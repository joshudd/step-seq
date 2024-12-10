#include "connection.h"

bool connectionHandled = false;
bool log_commands = false;

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

    // listServicesAndCharacteristics();
    // listClientInformation();

    serialPrintF("[ble] initialized\r\n");
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

void bond() {
    char buf[BUF_SIZE];
    usartWriteCommand("B\r\n");
    usartReadUntil(buf, BLE_RADIO_PROMPT);
}

void changeConnectionParameters(uint16_t interval, uint16_t latency, uint16_t timeout) {
    char command[64];
    sprintf(command, "T,%04X,%04X,%04X,%04X\r\n", interval, interval, latency, timeout);
    usartWriteCommand(command);
    char response[BUF_SIZE];
    usartReadUntil(response, BLE_RADIO_PROMPT);
}

void enableNotifications(uint16_t handle, uint16_t configValue) {
    char buf[BUF_SIZE];
    strcpy(buf, "CHW,");
    sprintf(buf + strlen(buf), "%04X", handle);
    sprintf(buf + strlen(buf), ",%04X\r\n", configValue);
    usartWriteCommand(buf);
    usartReadUntil(buf, BLE_RADIO_PROMPT);
}

void handleConnection() {
    serialPrintF("[ble] handling connection\r\n");
    bond();
    changeConnectionParameters(0x000C, 0x0000, 0x00C8);
    enableNotifications(0x0073, 0x0100);

    ble_connected = true;
}

void readBleData() {
    char buf[BUF_SIZE];
    usartReadUntil(buf, BLE_RADIO_PROMPT);

    if (strlen(buf) > 0) {
        if (log_commands) {
            serialPrintF("[ble] reading data: ");
            serialPrintF(buf);
            serialPrintF("\r\n");
        }

        // Check for the %W message and extract handle and config values
        char *w_start = strstr(buf, "%W");
        if (w_start != NULL && !connectionHandled) {
            handleConnection();
            connectionHandled = true;
        }
    }
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

    serialPrintF("[usart] initialized\r\n");
}

void usartWriteChar(char c) {
    while (!(USART0.STATUS & USART_DREIF_bm)){;}     
    USART0.TXDATAL = c;
}

void usartWriteCommand(const char *cmd) {
    char *cmd_copy = strdup(cmd);
    
    if (log_commands) {
        serialPrintF("[ble] command: ");
        serialPrintF(cmd_copy);
        serialPrintF("\r\n");
    }

    for (uint8_t i = 0; cmd[i] != '\0'; i++) {
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
        max_attempts--;
    }
    dest[bytes_read] = '\0';

    if (max_attempts > 0 && log_commands) {
        serialPrintF("[ble] response: ");
        serialPrintF(dest);
        serialPrintF("\r\n");
    }
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

bool initializeClientOperation() {
    char buf[BUF_SIZE];
    usartWriteCommand("CI\r\n");
    usartReadUntil(buf, BLE_RADIO_PROMPT);

    if (strstr(buf, "AO") != NULL) {
        return true;
    } else {
        return false;
    }
}

void listClientInformation() {
    char buf[BUF_SIZE];
    
    // Check connection status first
    usartWriteCommand("GK\r\n");
    usartReadUntil(buf, BLE_RADIO_PROMPT);
    
    initializeClientOperation();
    
    _delay_ms(1000);
    
    // usartWriteCommand("LC\r\n");
    // usartReadUntil(buf, BLE_RADIO_PROMPT);
    
    // usartWriteCommand("LC,180A\r\n");
    // usartReadUntil(buf, BLE_RADIO_PROMPT);
    
    usartWriteCommand("LC,03B80E5AEDE84B33A7516CE34EC4C700\r\n");
    usartReadUntil(buf, BLE_RADIO_PROMPT);
}

// DEBUGGING USE
void serialInit(void) {
    PORTF.DIRSET = PIN0_bm;
    PORTF.DIRCLR = PIN1_bm;
    USART2.BAUD = (uint16_t)USART_BAUD_VALUE(9600);
    USART2.CTRLB |= USART_TXEN_bm;

    serialPrintF("\n\n\n\n\n\nstarting...\r\n[serial] initialized\r\n");
}

// DEBUGGING USE
void serialPrintF(char *str) {
    for(size_t i = 0; i < strlen(str); i++) {
        while (!(USART2.STATUS & USART_DREIF_bm));      
        USART2.TXDATAL = str[i];
    }
}
