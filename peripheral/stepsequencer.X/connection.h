
#ifndef CONNECTION_H
#define	CONNECTION_H

#include "main.h"

#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>

void usartInit();
void usartWriteChar(char c);
void usartWriteCommand(const char *cmd);
char usartReadChar();
void usartReadUntil(char *dest, const char *end_str);

// BLE commands
void bleInit();
void bleWriteCharacteristic(char *characteristicUUID, uint8_t *data, size_t length);
void enableNotifications(uint16_t handle, uint16_t configValue);
void setupServiceAndCharacteristic();
void listServicesAndCharacteristics();
void getConnectionStatus();
void startAdvertising();
void stopAdvertising();
void bond();
bool initializeClientOperation();
void readBleData();

// DEBUGGING USE
void serialInit(void);
void serialPrintF(char *str);

#endif	/* CONNECTION_H */
