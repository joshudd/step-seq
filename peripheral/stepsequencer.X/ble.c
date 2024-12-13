#include "ble.h"

bool connection_handled = false;

void ble_init()
{
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
    usart_write_command("$$$");
    usart_read_until(buf, BLE_RADIO_PROMPT);

    // Change BLE device name to specified value
    // There can be some lag between updating name here and
    // seeing it in the LightBlue phone interface
    strcpy(buf, "S-,");
    strcat(buf, "step-seq");
    strcat(buf, "\r\n");
    usart_write_command(buf);
    usart_read_until(buf, BLE_RADIO_PROMPT);

    strcpy(buf, "PZ\r\n");
    usart_write_command(buf);
    usart_read_until(buf, BLE_RADIO_PROMPT);

    usart_write_command("SN,step-seq\r\n");
    usart_read_until(buf, BLE_RADIO_PROMPT);

    // manufacturer name
    usart_write_command("SDN,codemaxxer inc.\r\n");
    usart_read_until(buf, BLE_RADIO_PROMPT);

    setup_service_and_characteristic();
}

void setup_service_and_characteristic()
{
    // add MIDI service
    char buf[BUF_SIZE];
    strcpy(buf, "PS,");
    strcat(buf, MIDI_SERVICE_UUID);
    strcat(buf, "\r\n");
    usart_write_command(buf);
    usart_read_until(buf, BLE_RADIO_PROMPT);

    // add MIDI characteristic
    strcpy(buf, "PC,");
    strcat(buf, MIDI_CHARACTERISTIC_UUID);
    strcat(buf, ",FF,10\r\n");
    usart_write_command(buf);
    usart_read_until(buf, BLE_RADIO_PROMPT);

    // set initial value
    strcpy(buf, "SHW,0072,00\r\n");
    usart_write_command(buf);
    usart_read_until(buf, BLE_RADIO_PROMPT);
}

void change_connection_parameters(uint16_t interval, uint16_t latency, uint16_t timeout)
{
    char command[64];
    sprintf(command, "T,%04X,%04X,%04X,%04X\r\n", interval, interval, latency, timeout);
    usart_write_command(command);
    char response[BUF_SIZE];
    usart_read_until(response, BLE_RADIO_PROMPT);
}

void enable_notifications(uint16_t handle, uint16_t configValue)
{
    char buf[BUF_SIZE];
    strcpy(buf, "CHW,");
    sprintf(buf + strlen(buf), "%04X", handle);
    sprintf(buf + strlen(buf), ",%04X\r\n", configValue);
    usart_write_command(buf);
    usart_read_until(buf, BLE_RADIO_PROMPT);
}

void handle_connection()
{
    char buf[BUF_SIZE];
    usart_write_command("B\r\n"); // bond
    usart_read_until(buf, BLE_RADIO_PROMPT);

    change_connection_parameters(0x000C, 0x0000, 0x00C8);
    enable_notifications(0x0073, 0x0100);

    ble_connected = true;
}

void read_ble_data()
{
    char buf[BUF_SIZE];
    usart_read_until(buf, BLE_RADIO_PROMPT);

    if (strlen(buf) > 0)
    {
        char *w_start = strstr(buf, "%W"); // handle connection if %W is received
        if (w_start != NULL && !connection_handled)
        {
            handle_connection();
            connection_handled = true;
        }
    }
}

void gatt_server_send_characteristic_notification(uint16_t handle, uint8_t *data, size_t length)
{
    char buf[BUF_SIZE];
    strcpy(buf, "SHW,");                         // start with the command
    sprintf(buf + strlen(buf), "%04X,", handle); // append the handle

    // append the data
    for (size_t i = 0; i < length; i++)
    {
        sprintf(buf + strlen(buf), "%02X", data[i]);
    }

    sprintf(buf + strlen(buf), "\r\n");
    usart_write_command(buf);                // write the characteristic
    usart_read_until(buf, BLE_RADIO_PROMPT); // wait for the response
}