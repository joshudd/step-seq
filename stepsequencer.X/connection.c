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
    strcat(buf, ",1A,05\r\n");
    usartWriteCommand(buf);
    usartReadUntil(buf, BLE_RADIO_PROMPT);

    // add MIDI characteristic
    strcpy(buf, "PC,");
    strcat(buf, MIDI_CHARACTERISTIC_UUID);
    strcat(buf, ",1A,05\r\n");
    usartWriteCommand(buf);
    usartReadUntil(buf, BLE_RADIO_PROMPT);
}

void usartInit() {
    PORTA.DIR &= ~PIN1_bm;
    PORTA.DIR |= PIN0_bm;
    
    USART0.BAUD = (uint16_t)USART_BAUD_VALUE(115200);

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
    // Print the command being sent
    serialPrintF("[me] command: ");
    serialPrintF(cmd);
    serialPrintF("\r\n");

    // Send each character of the command
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

void readBleData() {
    char buf[BUF_SIZE];
    usartReadUntil(buf, BLE_RADIO_PROMPT);

    if (strlen(buf) > 0) {
        serialPrintF("[central] data: ");
        serialPrintF(buf);
        serialPrintF("\r\n");
    }
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
