#pragma once

#include <stdbool.h>
#include "lvgl.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    VANGO_PAGE_HOME = 0,
    VANGO_PAGE_DIMMER,
    VANGO_PAGE_RGB,
    VANGO_PAGE_WATER,
    VANGO_PAGE_SENSORS,
    VANGO_PAGE_MOTORS,
    VANGO_PAGE_SETTINGS,
    VANGO_PAGE_COUNT
} vango_page_t;

/* 800×480 chrome — settings tile yüksekliği ile senkron */
#define VANGO_DASH_STATUS_H  46
#define VANGO_DASH_NAV_H     54
#define VANGO_DASH_GAP       6
#define VANGO_DASH_DOTS_H    6
#define VANGO_DASH_TILE_H    (480 - VANGO_DASH_NAV_H - VANGO_DASH_STATUS_H - VANGO_DASH_GAP - VANGO_DASH_DOTS_H)

void vango_dash_init(void);
void vango_dash_show(vango_page_t page);
lv_obj_t *vango_dash_get_screen(void);

/** Live telemetry → status bar / water page */
void vango_dash_set_battery_v(float volts);
void vango_dash_set_temp_c(int temp_c);
void vango_dash_set_th(int temp_c, int hum_pct, bool linked);
void vango_dash_set_water(int clean_pct, int waste_pct, int grey_pct);
/** Depo toplam hacmi (litre). 0 = tanımsız, yüzde altında litre yazılmaz. */
int vango_dash_tank_capacity(int idx);
void vango_dash_set_tank_capacity(int idx, int liters);
/** Sag ust: IO(CAN) / BLE / WiFi durum ikonlari. */
void vango_dash_set_link_status(bool io_can, bool ble, bool wifi);
/** Eski API — WiFi ikonunu gunceller (IO/BLE dokunmaz). */
void vango_dash_set_wifi(bool connected);
void vango_dash_set_motor_link(bool connected);
/** Same metallic gear as the motor dial, scaled for nav and settings. */
const lv_img_dsc_t *vango_dash_motor_icon(bool bright);
/** Per-motor arm switch. Tablet default is armed. false stops that motor. */
void vango_dash_motor_set_armed(int slot, bool armed);
/** Padlock under the gear. true = locked (blocks move). Default locked. */
void vango_dash_motor_set_locked(int slot, bool locked);
void vango_dash_set_dimmer_link(bool connected);
void vango_dash_sync_dimmers(void);
void vango_dash_sync_relays(void);
/** IO CAN labels/types: home röle isim+ikon yenile (full rebuild yok). */
void vango_dash_refresh_channel_meta(void);

/** Demo preview (relays/dim/water). CAN bağlıyken zorla kapalı. */
bool vango_dash_get_demo(void);
void vango_dash_set_demo(bool on);

/** UART/shot test: CAN 0x720 / 0x730 / 0x750 (link yoksa no-op). */
void vango_dash_test_relay(int slot, bool on);
void vango_dash_test_dim(int slot, int pct);
void vango_dash_test_motor(int cmd);

/** Software brightness 10..100 (black overlay). No PWM backlight on this board. */
void vango_dash_set_brightness(int pct);
int vango_dash_get_brightness(void);

/** Theme/language changed (legacy or dash) — refresh dash labels/UI. */
void vango_dash_on_prefs_changed(void);
/** Accent-only: chrome + pages except Settings (Tema secimi reset olmasin). */
void vango_dash_on_accent_changed(void);

/** After leaving legacy settings screens, return here. */
void vango_dash_set_return_from_legacy(bool enable);
bool vango_dash_consume_return_from_legacy(void);

#ifdef __cplusplus
}
#endif
