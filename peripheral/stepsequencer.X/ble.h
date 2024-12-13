
#ifndef CONNECTION_H
#define	CONNECTION_H

#include "main.h"
#include "config.h"

#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>

void ble_init();
void enable_notifications(uint16_t handle, uint16_t configValue);
void setup_service_and_characteristic();
void read_ble_data();
void gatt_server_send_characteristic_notification(uint16_t handle, uint8_t *data, size_t length);

#endif	/* CONNECTION_H */
