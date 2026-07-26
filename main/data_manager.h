// data_manager.h

#ifndef DATA_MANAGER_H
#define DATA_MANAGER_H

#include <stdint.h>

#define MAX_BUFFER_SIZE 256

void data_manager_init();
void send_to_bluetooth(const uint8_t *data, int length);
void send_to_wifi(const uint8_t *data, int length);
void display_update(const uint8_t *data, int length);
void update_regs_data(uint16_t index, uint16_t value);
void toggle_regs_data(uint16_t index);
uint16_t* getSlavesRegsData();
int get_modbus_connection_status();
void check_connected_devices();



#endif // DATA_MANAGER_H