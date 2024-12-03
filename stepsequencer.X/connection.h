
#ifndef CONNECTION_H
#define	CONNECTION_H

#include "main.h"

void bleInit();

void usartInit();
void usartWriteChar(char c);
void usartWriteCommand(const char *cmd);
char usartReadChar();
void usartReadUntil(char *dest, const char *end_str);

// BLE commands
void listServicesAndCharacteristics();
void getConnectionStatus();
void startAdvertising();
void stopAdvertising();
void bond();
void readBleData();

// DEBUGGING USE
void serialInit(void);
void serialPrintF(char *str);

#endif	/* CONNECTION_H */
