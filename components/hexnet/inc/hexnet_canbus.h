/*
 * SPDX-FileCopyrightText: 2021-2024 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Unlicense OR CC0-1.0
 */
        
 #ifndef HEXNET_CANBUS_H
 #define HEXNET_CANBUS_H
 
 #include "freertos/FreeRTOS.h"
 #include "freertos/task.h"
 #include "freertos/event_groups.h"
 #include "esp_system.h"
 #include "esp_log.h"
 #include "nvs_flash.h"
 #include "esp_bt.h"
 #include <stdio.h>
 #include <stdlib.h>
 #include <string.h>
 
 
 #include "esp_gap_ble_api.h"
 #include "esp_gatts_api.h"
 #include "esp_bt_defs.h"
 #include "esp_bt_main.h"
 #include "esp_bt_device.h"
 #include "esp_gatt_common_api.h"
 

void twai_ini(void);
void can_watchdog_task(void *pvParameter);
// Getter Functions
uint16_t get_voltage(); 
uint16_t get_outputs();
uint16_t get_inputs();
uint8_t get_analog_input(uint8_t index);
uint8_t get_dimmable_output(uint8_t index); 
uint8_t get_rgb_value(uint8_t index);
uint8_t get_canbus_connection_status();
void send_can_frame(uint32_t id, uint8_t *data);
uint8_t get_sensorTemp();
uint8_t get_sensorHum();

 #endif /* HEXNET_CANBUS_H */
 