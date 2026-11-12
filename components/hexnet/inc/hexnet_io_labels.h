#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "cJSON.h"
#include "esp_err.h"

#define HEXNET_LABEL_NAME_MAX  23
#define HEXNET_LABEL_ICON_MAX  23
#define HEXNET_LABEL_RELAYS    16
#define HEXNET_LABEL_DIMS      8
#define HEXNET_LABEL_SENSORS   5
#define HEXNET_LABEL_WATER     3

typedef struct {
    char name[HEXNET_LABEL_NAME_MAX + 1];
    char icon[HEXNET_LABEL_ICON_MAX + 1];
} hexnet_label_slot_t;

typedef struct {
    hexnet_label_slot_t relays[HEXNET_LABEL_RELAYS];
    hexnet_label_slot_t dimmers[HEXNET_LABEL_DIMS];
    hexnet_label_slot_t sensors[HEXNET_LABEL_SENSORS];
    hexnet_label_slot_t water[HEXNET_LABEL_WATER];
} hexnet_io_labels_t;

void hexnet_io_labels_init(void);
const hexnet_io_labels_t *hexnet_io_labels_get(void);
esp_err_t hexnet_io_labels_save(void);

/** Yerel röle isim/ikon güncelle (NVS debounce; UI dirty). */
esp_err_t hexnet_io_labels_set_relay(int slot, const char *name, const char *icon);

/** channels / panel_channels JSON (platform + tablet semasi). */
bool hexnet_io_labels_apply_json(const cJSON *json);

/** io_profile icine channels objesi ekle (MQTT/BLE SoT). */
void hexnet_io_labels_append_channels(cJSON *io_obj);

/**
 * CAN 0x118/0x119 chunk: kind 0=name 1=icon, part 0..3 × 6 byte.
 * part==0 alani sifirlar; son parca (trailing 0 veya part==3) yazinca NVS+dirty.
 */
void hexnet_io_labels_can_chunk(uint8_t kind, uint8_t slot, uint8_t part, const uint8_t six[6]);

bool hexnet_io_labels_peek_ui_dirty(void);
bool hexnet_io_labels_consume_ui_dirty(void);
