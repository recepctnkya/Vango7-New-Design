#include "ui_vango_dash.h"
#include "ui_vango_settings.h"
#include "ui_vango_rgb.h"
#include "ui_vango_theme.h"
#include "ui_vango_i18n.h"
#include "hexnet_io_profile.h"
#include "hexnet_io_labels.h"
#include "hexnet_icon_map.h"
#include "hexnet_canbus.h"
#include "display_manager.h"
#include "fonts/vango_fonts.h"
#include "icons/vango_icons.h"
#include "icons/vango_panel_icons.h"
#include "ui.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "esp_heap_caps.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "nvs.h"

#define TAG "VANGO_DASH"

/* 800x480 layout — content and nav must not overlap */
#define DASH_W              800
#define DASH_H              480
#define DASH_STATUS_H       VANGO_DASH_STATUS_H
#define DASH_NAV_H          VANGO_DASH_NAV_H
#define DASH_SAFE_BOTTOM    0
/* Ust cyan cizgi ile icerik arasi ~6 px (ayirac yeni panel eklemez) */
#define DASH_GAP            VANGO_DASH_GAP
#define DASH_DOTS_H         VANGO_DASH_DOTS_H
#define DASH_NAV_Y          (DASH_H - DASH_SAFE_BOTTOM - DASH_NAV_H)
#define DASH_CONTENT_Y      DASH_STATUS_H
#define DASH_CONTENT_H      (DASH_NAV_Y - DASH_STATUS_H - DASH_GAP)
#define DASH_TILE_H         VANGO_DASH_TILE_H
#define DASH_STATUS_RULE    0x00BBD6u
#define DASH_NAV_RULE       0x24445Bu
#define DASH_NAV_BG         0x0D1729u
#define RELAY_SUBPAGE_COUNT 2
#define RELAYS_PER_PAGE     6
#define RELAY_TOTAL         (RELAY_SUBPAGE_COUNT * RELAYS_PER_PAGE)

static lv_obj_t *s_scr;
static lv_obj_t *s_content;
static lv_obj_t *s_tileview;
static lv_obj_t *s_tiles[VANGO_PAGE_COUNT];
static lv_obj_t *s_nav_btns[VANGO_PAGE_COUNT];
static lv_obj_t *s_nav_lbls[VANGO_PAGE_COUNT];
static lv_obj_t *s_nav_icos[VANGO_PAGE_COUNT];
static lv_obj_t *s_dots[RELAY_SUBPAGE_COUNT];
static vango_page_t s_page = VANGO_PAGE_HOME;
static bool s_syncing_tile;
static lv_obj_t *s_relay_tv;
static int s_relay_subpage;
static bool s_syncing_relay_tv;

static lv_obj_t *s_lbl_bat;
static lv_obj_t *s_lbl_temp;
static lv_obj_t *s_lbl_w_clean;
static lv_obj_t *s_lbl_w_waste;
static lv_obj_t *s_lbl_w_grey;
static lv_obj_t *s_title;
static lv_obj_t *s_btn_back;
static lv_obj_t *s_img_logo;
static lv_obj_t *s_nav_bar;
static lv_obj_t *s_status_bar;
static lv_obj_t *s_status_rule;
static lv_obj_t *s_io_ico;   /* IO modul CAN — 24x24 mikroçip */
static lv_obj_t *s_ble_lbl;
static lv_obj_t *s_wifi_lbl;
static bool s_link_io;
static bool s_link_ble;
static bool s_link_wifi;
static int64_t s_io_wait_until_us; /* baglaniyor (turuncu) penceresi */
static lv_obj_t *s_dim_veil;
static int s_brightness = 100;

static void brightness_load(void);
static void brightness_apply(void);
static void build_dim_veil(lv_obj_t *parent);

static int s_w_clean = -1, s_w_waste = -1, s_w_grey = -1;
static int s_temp_c = 0;
static float s_bat_v = 0.0f;

#define WATER_RIM       14
#define WATER_WAVE_AMP  5
#define WATER_WAVE_MAX_W 200

static lv_obj_t *s_water_pct_lbl[3];
static lv_obj_t *s_water_ltr_lbl[3];
static int s_tank_cap[3];
static lv_obj_t *s_water_fill[3];
static lv_obj_t *s_water_neck[3];
static lv_obj_t *s_water_wave[3];
#define WATER_BUB_N 4
static lv_obj_t *s_water_bub[3][WATER_BUB_N];
static uint16_t s_bub_rise;
static lv_color_t s_wave_buf[3][WATER_WAVE_MAX_W * (WATER_WAVE_AMP * 2)];
static lv_color_t s_wave_color[3];
static int s_wave_span[3];
static int s_wave_amp[3];
static lv_coord_t s_water_max[3];
static lv_coord_t s_water_px[3];
static uint16_t s_water_phase;
static lv_obj_t *s_home_bat;
static lv_obj_t *s_home_temp;
static lv_obj_t *s_home_w[3];
#define HOME_RELAYS 12
/* Local tap: keep optimistic UI until CAN catches up (or grace expires). */
#define RELAY_PENDING_MS 2500
static bool s_demo = false;
static bool s_relay_on[HOME_RELAYS];
static uint32_t s_relay_pending_ms[HOME_RELAYS]; /* lv_tick when local cmd sent; 0=idle */
static bool s_relay_pending_want[HOME_RELAYS];
static lv_obj_t *s_relay_card[HOME_RELAYS];
static lv_obj_t *s_relay_ico[HOME_RELAYS];
static lv_obj_t *s_relay_nm[HOME_RELAYS];
static lv_obj_t *s_relay_st[HOME_RELAYS];
static lv_obj_t *s_demo_badge;
static bool s_page_built[VANGO_PAGE_COUNT];
static int s_auto_dark_cached = -1;

void vango_dash_on_prefs_changed(void); /* forward for auto-theme timer */

static void auto_theme_tick_cb(lv_timer_t *t)
{
    (void)t;
    /* System mode follows IO SoT via CAN — no local day/night schedule. */
    (void)s_auto_dark_cached;
}

static const char *nav_label(int i)
{
    static const char *en[] = {
        "Home", "Dimmer", "RGB", "Water", "Sensor", "Motor", "Settings"
    };
    static const char *tr[] = {
        "Ana Sayfa", "Dimmer", "RGB", "Su Sistemi", "Sensör", "Motor", "Ayarlar"
    };
    static const char *de[] = {
        "Start", "Dimmer", "RGB", "Wasser", "Sensor", "Motor", "Einstellungen"
    };
    if (i < 0 || i >= VANGO_PAGE_COUNT) {
        return "";
    }
    return vango_tx(en[i], tr[i], de[i]);
}

static void set_header_home(bool home)
{
    if (home) {
        lv_obj_add_flag(s_btn_back, LV_OBJ_FLAG_HIDDEN);
        if (s_img_logo) {
            lv_obj_clear_flag(s_img_logo, LV_OBJ_FLAG_HIDDEN);
        }
    } else {
        lv_obj_clear_flag(s_btn_back, LV_OBJ_FLAG_HIDDEN);
        if (s_img_logo) {
            lv_obj_add_flag(s_img_logo, LV_OBJ_FLAG_HIDDEN);
        }
    }
    /* logo 34 + gap 6 → VANGO x=48 (üst menü home) */
    lv_obj_set_pos(s_title, 48, 12);
}

static void style_card(lv_obj_t *obj)
{
    lv_obj_set_style_bg_color(obj, lv_color_hex(COL_CARD), 0);
    lv_obj_set_style_bg_opa(obj, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(obj, 18, 0);
    lv_obj_set_style_border_width(obj, 1, 0);
    lv_obj_set_style_border_color(obj, lv_color_hex(COL_CARD_BORDER), 0);
    lv_obj_set_style_border_opa(obj, LV_OPA_COVER, 0);
    lv_obj_set_style_shadow_width(obj, 0, 0);
    lv_obj_set_style_pad_all(obj, 0, 0);
    lv_obj_clear_flag(obj, LV_OBJ_FLAG_SCROLLABLE);
}

static lv_obj_t *make_label(lv_obj_t *parent, const char *txt, const lv_font_t *font, uint32_t color)
{
    lv_obj_t *l = lv_label_create(parent);
    lv_label_set_text(l, txt);
    lv_obj_set_style_text_font(l, font, 0);
    lv_obj_set_style_text_color(l, lv_color_hex(color), 0);
    return l;
}

static void update_relay_dots(int subpage)
{
    bool show = false;
    for (int i = 0; i < RELAY_SUBPAGE_COUNT; i++) {
        if (!s_dots[i]) {
            continue;
        }
        if (show) {
            lv_obj_clear_flag(s_dots[i], LV_OBJ_FLAG_HIDDEN);
            bool on = (i == subpage);
            lv_obj_set_style_bg_color(s_dots[i], lv_color_hex(on ? COL_ACCENT : COL_DOT_OFF), 0);
            lv_obj_set_size(s_dots[i], on ? 10 : 8, on ? 10 : 8);
        } else {
            lv_obj_add_flag(s_dots[i], LV_OBJ_FLAG_HIDDEN);
        }
    }
}

static void update_dots(vango_page_t page)
{
    (void)page;
    update_relay_dots(s_relay_subpage);
}

static void nav_set_active(vango_page_t page)
{
    for (int i = 0; i < VANGO_PAGE_COUNT; i++) {
        bool on = (i == (int)page);
        if (s_nav_btns[i]) {
            lv_obj_set_style_bg_color(s_nav_btns[i], lv_color_hex(COL_NAV_SEL), 0);
            lv_obj_set_style_bg_opa(s_nav_btns[i], on ? LV_OPA_COVER : LV_OPA_TRANSP, 0);
        }
        if (s_nav_lbls[i]) {
            lv_obj_set_style_text_color(s_nav_lbls[i], lv_color_hex(on ? COL_ACCENT : COL_MUTED), 0);
        }
        if (s_nav_icos[i]) {
            if (i == VANGO_PAGE_MOTORS) {
                lv_img_set_src(s_nav_icos[i], vango_dash_motor_icon(true));
                lv_obj_set_style_img_recolor_opa(s_nav_icos[i], LV_OPA_TRANSP, 0);
                lv_obj_set_style_img_opa(s_nav_icos[i], on ? LV_OPA_COVER : LV_OPA_60, 0);
            } else if (i == VANGO_PAGE_RGB) {
                /* TRUE_COLOR RGB ikon — recolor yok */
                lv_obj_set_style_img_recolor_opa(s_nav_icos[i], LV_OPA_TRANSP, 0);
                lv_obj_set_style_img_opa(s_nav_icos[i], on ? LV_OPA_COVER : LV_OPA_70, 0);
            } else {
                lv_obj_set_style_img_recolor(s_nav_icos[i], lv_color_hex(on ? COL_ACCENT : COL_MUTED), 0);
                lv_obj_set_style_img_recolor_opa(s_nav_icos[i], LV_OPA_COVER, 0);
            }
        }
    }
}

static void apply_page_chrome(vango_page_t page);
static void ensure_page_built(vango_page_t page);
static void tileview_scroll_begin_cb(lv_event_t *e);
static void back_cb(lv_event_t *e);

static void apply_page_chrome(vango_page_t page)
{
    lv_label_set_text(s_title, "VanGO");
    lv_obj_set_style_text_color(s_title, lv_color_hex(COL_GREEN), 0);
    set_header_home(true);
    nav_set_active(page);
    update_dots(page);
}

static void light_btn_apply(lv_obj_t *card, bool on)
{
    lv_obj_t *glow = lv_obj_get_user_data(card);
    lv_obj_t *ico = glow ? lv_obj_get_user_data(glow) : NULL;
    lv_obj_t *st = ico ? lv_obj_get_user_data(ico) : NULL;

    if (on) {
        lv_obj_add_state(card, LV_STATE_CHECKED);
        lv_obj_set_style_border_color(card, lv_color_hex(COL_GLOW_Y), 0);
        lv_obj_set_style_border_opa(card, LV_OPA_COVER, 0);
        lv_obj_set_style_bg_color(card, lv_color_hex(COL_LIGHT_ON), 0);
        if (glow) {
            lv_obj_set_style_bg_opa(glow, LV_OPA_40, 0);
        }
        if (ico) {
            lv_obj_set_style_img_recolor_opa(ico, LV_OPA_TRANSP, 0);
        }
        if (st) {
            lv_label_set_text(st, vango_tx_on());
            lv_obj_set_style_text_color(st, lv_color_hex(COL_GLOW_Y), 0);
        }
    } else {
        lv_obj_clear_state(card, LV_STATE_CHECKED);
        lv_obj_set_style_border_color(card, lv_color_hex(COL_CARD_BORDER), 0);
        lv_obj_set_style_bg_color(card, lv_color_hex(COL_CARD), 0);
        if (glow) {
            lv_obj_set_style_bg_opa(glow, LV_OPA_10, 0);
        }
        if (ico) {
            lv_obj_set_style_img_recolor(ico, lv_color_hex(COL_MUTED), 0);
            lv_obj_set_style_img_recolor_opa(ico, LV_OPA_70, 0);
        }
        if (st) {
            lv_label_set_text(st, vango_tx_off());
            lv_obj_set_style_text_color(st, lv_color_hex(COL_MUTED), 0);
        }
    }
}

static void light_btn_cb(lv_event_t *e)
{
    lv_obj_t *card = lv_event_get_target(e);
    bool on = !lv_obj_has_state(card, LV_STATE_CHECKED);
    light_btn_apply(card, on);
    ESP_LOGI(TAG, "Light button -> %s", on ? "ON" : "OFF");
}

static void add_light_card(lv_obj_t *parent, int x, int y, int w, int h, const char *name, bool on)
{
    lv_obj_t *card = lv_obj_create(parent);
    lv_obj_set_size(card, w, h);
    lv_obj_set_pos(card, x, y);
    style_card(card);
    lv_obj_add_flag(card, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_clear_flag(card, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_border_width(card, 2, 0);
    lv_obj_add_event_cb(card, light_btn_cb, LV_EVENT_CLICKED, NULL);

    const int ico_sz = (w < 200) ? 52 : 64;
    const int ico_zoom = (w < 200) ? 180 : 200;

    lv_obj_t *title = make_label(card, name, &vango_font_14, COL_TEXT);
    lv_obj_align(title, LV_ALIGN_TOP_LEFT, 10, 8);

    lv_obj_t *glow = lv_obj_create(card);
    lv_obj_set_size(glow, ico_sz, ico_sz);
    lv_obj_align(glow, LV_ALIGN_LEFT_MID, 8, 10);
    lv_obj_set_style_radius(glow, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_color(glow, lv_color_hex(COL_GLOW_Y), 0);
    lv_obj_set_style_bg_opa(glow, LV_OPA_30, 0);
    lv_obj_set_style_border_width(glow, 0, 0);
    lv_obj_clear_flag(glow, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_flag(glow, LV_OBJ_FLAG_EVENT_BUBBLE);

    lv_obj_t *ico = lv_img_create(card);
    lv_img_set_src(ico, &vango_icon_light);
    lv_img_set_zoom(ico, ico_zoom);
    lv_obj_align(ico, LV_ALIGN_LEFT_MID, 12, 10);
    lv_obj_clear_flag(ico, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_flag(ico, LV_OBJ_FLAG_EVENT_BUBBLE);

    lv_obj_t *st = make_label(card, on ? vango_tx_on() : vango_tx_off(), &vango_font_14, on ? COL_GLOW_Y : COL_MUTED);
    lv_obj_align(st, LV_ALIGN_BOTTOM_RIGHT, -10, -10);
    lv_obj_clear_flag(st, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_flag(st, LV_OBJ_FLAG_EVENT_BUBBLE);

    lv_obj_set_user_data(card, glow);
    lv_obj_set_user_data(glow, ico);
    lv_obj_set_user_data(ico, st);

    light_btn_apply(card, on);
}

/* CAN 0x730: IO v0.60 8 dimmer; 0x200 dims0..3 + 0x210 dims4..7. */
#define DIM_DRIVE_SLOTS 8

extern const char *lblDimBtnNames[8];
extern const char *lblDimBtnNames_TR[8];
extern const char *lblDimBtnNames_DE[8];

#define DIM_COL_CARD    COL_ACCENT_CARD
#define DIM_COL_BORDER  COL_ACCENT_DIM
#define DIM_COL_ACTIVE  0xF5C542u
#define DIM_COL_IDLE    0x566777u
#define DIM_COL_SWITCH  COL_ACCENT
#define DIM_COL_TRACK   0x5C6B7Au
#define DIM_COL_PAGE    COL_BG

/* Shown only while demo is on and CAN is down. Not written to the bus. */
static const uint8_t s_dim_preview[HEXNET_IO_DIM_SLOTS] = {
    75, 60, 40, 90, 0, 50, 30, 80
};

typedef struct {
    lv_obj_t *card;
    lv_obj_t *sw;
    lv_obj_t *slider;
    lv_obj_t *pct;
    lv_obj_t *ico;
    lv_obj_t *name;
    int slot;
    bool drive;
    uint8_t brightness; /* setpoint, kept while the output is off */
    bool enabled;       /* switch; independent from a 0% setpoint */
} dim_card_t;

static dim_card_t s_dims[HEXNET_IO_DIM_SLOTS];
static int s_dim_count;
static bool s_dim_link;
static bool s_dim_sync;
static uint32_t s_dim_touch_ms;

static void dim_touch(void)
{
    s_dim_touch_ms = lv_tick_get();
}

static const char *dim_cfg_name(int type)
{
    if (type < 1 || type > 8) {
        type = 1;
    }
    return vango_tx(lblDimBtnNames[type - 1], lblDimBtnNames_TR[type - 1],
                    lblDimBtnNames_DE[type - 1]);
}

static void dim_send(int slot, int value)
{
    uint8_t data[8] = {0};
    if (slot < 0 || slot >= DIM_DRIVE_SLOTS || !s_dim_link) {
        return;
    }
    if (value < 0) {
        value = 0;
    }
    if (value > 100) {
        value = 100;
    }
    data[0] = (uint8_t)slot;
    data[1] = (uint8_t)value;
    send_can_frame(0x730, data);
}

static void dim_pct_text(lv_obj_t *lbl, const char *text, int pct)
{
    char buf[16];
    if (!lbl) {
        return;
    }
    if (text) {
        lv_label_set_text(lbl, text);
        return;
    }
    if (pct < 0) {
        pct = 0;
    }
    if (pct > 100) {
        pct = 100;
    }
    snprintf(buf, sizeof(buf), "%d %%", pct);
    lv_label_set_text(lbl, buf);
}

static void dim_icon_paint(lv_obj_t *ico, int pct, bool lit)
{
    int v = pct;
    lv_color_t col;
    lv_opa_t opa;
    if (!ico) {
        return;
    }
    if (!lit) {
        v = 0;
    }
    if (v < 0) {
        v = 0;
    }
    if (v > 100) {
        v = 100;
    }
    /* Value 0 is a visible 30% glow. 100 is fully lit. Color stays yellow. */
    v = 30 + (v * 70) / 100;
    col = lv_color_hex(0xF5C542);
    opa = (lv_opa_t)((v * 255) / 100);
    lv_obj_set_style_img_recolor(ico, col, 0);
    lv_obj_set_style_img_recolor_opa(ico, LV_OPA_COVER, 0);
    lv_obj_set_style_img_opa(ico, opa, 0);
}

static bool dim_ui_live(const dim_card_t *d)
{
    if (!d) {
        return false;
    }
    if (s_demo) {
        return true;
    }
    return d->drive && s_dim_link;
}

static void dim_apply(dim_card_t *d)
{
    const bool live = dim_ui_live(d);
    const bool lit = live && d->enabled;
    if (!d || !d->card) {
        return;
    }
    s_dim_sync = true;
    if (d->sw) {
        if (lit) {
            lv_obj_add_state(d->sw, LV_STATE_CHECKED);
        } else {
            lv_obj_clear_state(d->sw, LV_STATE_CHECKED);
        }
        if (live) {
            lv_obj_add_flag(d->sw, LV_OBJ_FLAG_CLICKABLE);
        } else {
            lv_obj_clear_flag(d->sw, LV_OBJ_FLAG_CLICKABLE);
        }
    }
    if (d->slider) {
        lv_slider_set_value(d->slider, d->brightness, LV_ANIM_OFF);
        lv_obj_set_style_bg_color(d->slider, lv_color_hex(lit ? DIM_COL_ACTIVE : DIM_COL_IDLE), LV_PART_INDICATOR);
        lv_obj_set_style_bg_opa(d->slider, LV_OPA_COVER, LV_PART_INDICATOR);
        lv_obj_set_style_bg_color(d->slider, lv_color_hex(lit ? 0xFFFFFFu : 0x9AA8B5u), LV_PART_KNOB);
        if (lit) {
            lv_obj_add_flag(d->slider, LV_OBJ_FLAG_CLICKABLE);
        } else {
            lv_obj_clear_flag(d->slider, LV_OBJ_FLAG_CLICKABLE);
        }
    }
    s_dim_sync = false;

    if (d->pct) {
        if (!live && !d->drive) {
            lv_obj_set_style_text_font(d->pct, &vango_font_12, 0);
            dim_pct_text(d->pct, vango_tx("No drive", "Sürücü yok", "Kein Antrieb"), 0);
        } else if (!live && !s_dim_link) {
            lv_obj_set_style_text_font(d->pct, &vango_font_12, 0);
            dim_pct_text(d->pct, vango_tx("No link", "Bağlantı yok", "Keine Verbindung"), 0);
        } else {
            lv_obj_set_style_text_font(d->pct, &vango_font_20, 0);
            dim_pct_text(d->pct, NULL, d->brightness);
        }
        lv_obj_set_style_text_color(d->pct, lv_color_hex(lit ? 0xFFFFFFu : 0x9AA8B5u), 0);
    }
    if (d->name) {
        lv_obj_set_style_text_color(d->name, lv_color_hex(lit ? 0xFFFFFFu : 0xC5D0DAu), 0);
    }
    dim_icon_paint(d->ico, d->brightness, lit);
    lv_obj_set_style_opa(d->card, LV_OPA_COVER, 0);
}

static void dim_sw_cb(lv_event_t *e)
{
    dim_card_t *d = lv_event_get_user_data(e);
    bool on;
    if (s_dim_sync || !dim_ui_live(d)) {
        return;
    }
    on = lv_obj_has_state(d->sw, LV_STATE_CHECKED);
    d->enabled = on;
    /* Closing sends 0 and leaves brightness untouched. Opening resends it. */
    dim_touch();
    dim_send(d->slot, on ? d->brightness : 0);
    dim_apply(d);
}

static void dim_sl_cb(lv_event_t *e)
{
    dim_card_t *d = lv_event_get_user_data(e);
    lv_event_code_t code = lv_event_get_code(e);
    int v;
    if (s_dim_sync || !d || !d->slider || !d->enabled || !dim_ui_live(d)) {
        return;
    }
    v = lv_slider_get_value(d->slider);
    if (v < 0) {
        v = 0;
    }
    if (v > 100) {
        v = 100;
    }
    if (code == LV_EVENT_VALUE_CHANGED) {
        lv_obj_set_style_text_font(d->pct, &vango_font_20, 0);
        dim_pct_text(d->pct, NULL, v);
        dim_icon_paint(d->ico, v, true);
        return;
    }
    if (code != LV_EVENT_RELEASED && code != LV_EVENT_PRESS_LOST) {
        return;
    }
    d->brightness = (uint8_t)v;
    dim_touch();
    dim_send(d->slot, v);
    dim_apply(d);
}

static void add_dim_card(lv_obj_t *parent, int x, int y, int w, int h, int slot, int type)
{
    dim_card_t *d = &s_dims[s_dim_count];
    uint8_t live = 0;

    memset(d, 0, sizeof(*d));
    d->slot = slot;
    d->drive = slot < DIM_DRIVE_SLOTS;
    if (s_demo && !s_dim_link && slot >= 0 && slot < HEXNET_IO_DIM_SLOTS) {
        live = s_dim_preview[slot];
    } else if (d->drive && s_dim_link) {
        live = get_dimmable_output((uint8_t)slot);
        if (live > 100) {
            live = 100;
        }
    }
    d->brightness = live;
    d->enabled = live > 0;

    const int body_x = 54;
    const int name_w = w - body_x - 158;
    const int name_h = (int)lv_font_get_line_height(&vango_font_16) * 2;

    d->card = lv_obj_create(parent);
    lv_obj_set_size(d->card, w, h);
    lv_obj_set_pos(d->card, x, y);
    style_card(d->card);
    lv_obj_set_style_bg_color(d->card, lv_color_hex(DIM_COL_CARD), 0);
    lv_obj_set_style_radius(d->card, 18, 0);
    lv_obj_set_style_border_width(d->card, 2, 0);
    lv_obj_set_style_border_color(d->card, lv_color_hex(DIM_COL_BORDER), 0);
    lv_obj_set_style_border_opa(d->card, LV_OPA_80, 0);
    lv_obj_set_style_shadow_width(d->card, 14, 0);
    lv_obj_set_style_shadow_spread(d->card, 0, 0);
    lv_obj_set_style_shadow_ofs_y(d->card, 0, 0);
    lv_obj_set_style_shadow_color(d->card, lv_color_hex(DIM_COL_BORDER), 0);
    lv_obj_set_style_shadow_opa(d->card, LV_OPA_40, 0);

    d->ico = lv_img_create(d->card);
    lv_img_set_src(d->ico, &vango_icon_light);
    lv_img_set_zoom(d->ico, 150);
    lv_obj_align(d->ico, LV_ALIGN_LEFT_MID, -2, 0);
    lv_obj_set_style_img_recolor_opa(d->ico, LV_OPA_COVER, 0);
    lv_obj_clear_flag(d->ico, LV_OBJ_FLAG_CLICKABLE);

    lv_obj_t *rule = lv_obj_create(d->card);
    lv_obj_set_size(rule, 1, h - 18);
    lv_obj_align(rule, LV_ALIGN_LEFT_MID, 42, 0);
    lv_obj_set_style_bg_color(rule, lv_color_hex(DIM_COL_BORDER), 0);
    lv_obj_set_style_bg_opa(rule, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(rule, 0, 0);
    lv_obj_set_style_radius(rule, 0, 0);
    lv_obj_set_style_pad_all(rule, 0, 0);
    lv_obj_clear_flag(rule, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);

    d->name = make_label(d->card, dim_cfg_name(type), &vango_font_16, 0xFFFFFF);
    lv_obj_set_pos(d->name, body_x, 6);
    lv_obj_set_width(d->name, name_w > 48 ? name_w : 48);
    lv_obj_set_height(d->name, name_h);
    lv_label_set_long_mode(d->name, LV_LABEL_LONG_DOT);

    d->pct = make_label(d->card, "0 %", &vango_font_20, 0xFFFFFF);
    lv_obj_set_width(d->pct, 96);
    lv_obj_set_height(d->pct, 22);
    lv_label_set_long_mode(d->pct, LV_LABEL_LONG_DOT);
    lv_obj_set_style_text_align(d->pct, LV_TEXT_ALIGN_RIGHT, 0);
    lv_obj_align(d->pct, LV_ALIGN_TOP_RIGHT, -60, 8);

    d->sw = lv_switch_create(d->card);
    lv_obj_set_size(d->sw, 50, 26);
    lv_obj_align(d->sw, LV_ALIGN_TOP_RIGHT, -8, 6);
    lv_obj_set_style_bg_color(d->sw, lv_color_hex(DIM_COL_IDLE), LV_PART_MAIN);
    lv_obj_set_style_bg_color(d->sw, lv_color_hex(DIM_COL_IDLE), LV_PART_INDICATOR);
    lv_obj_set_style_bg_color(d->sw, lv_color_hex(DIM_COL_SWITCH), LV_PART_INDICATOR | LV_STATE_CHECKED);
    lv_obj_set_style_bg_color(d->sw, lv_color_hex(0xFFFFFFu), LV_PART_KNOB);
    lv_obj_add_event_cb(d->sw, dim_sw_cb, LV_EVENT_VALUE_CHANGED, d);

    d->slider = lv_slider_create(d->card);
    lv_slider_set_range(d->slider, 0, 100);
    lv_obj_set_width(d->slider, w - 80);
    lv_obj_set_height(d->slider, 14);
    lv_obj_set_pos(d->slider, 66, h - 24);
    lv_obj_set_style_bg_color(d->slider, lv_color_hex(DIM_COL_TRACK), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(d->slider, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_bg_color(d->slider, lv_color_hex(DIM_COL_ACTIVE), LV_PART_INDICATOR);
    lv_obj_set_style_bg_opa(d->slider, LV_OPA_COVER, LV_PART_INDICATOR);
    lv_obj_set_style_bg_color(d->slider, lv_color_hex(0xFFFFFFu), LV_PART_KNOB);
    lv_obj_set_style_border_width(d->slider, 0, LV_PART_KNOB);
    lv_obj_set_style_shadow_width(d->slider, 0, LV_PART_KNOB);
    lv_obj_set_style_radius(d->slider, LV_RADIUS_CIRCLE, LV_PART_MAIN);
    lv_obj_set_style_radius(d->slider, LV_RADIUS_CIRCLE, LV_PART_INDICATOR);
    lv_obj_set_style_radius(d->slider, LV_RADIUS_CIRCLE, LV_PART_KNOB);
    lv_obj_set_style_pad_all(d->slider, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(d->slider, 6, LV_PART_KNOB);
    lv_obj_set_ext_click_area(d->slider, 14);
    lv_obj_add_event_cb(d->slider, dim_sl_cb, LV_EVENT_VALUE_CHANGED, d);
    lv_obj_add_event_cb(d->slider, dim_sl_cb, LV_EVENT_RELEASED, d);
    lv_obj_add_event_cb(d->slider, dim_sl_cb, LV_EVENT_PRESS_LOST, d);

    s_dim_count++;
    dim_apply(d);
}

static void build_dimmer_page(lv_obj_t *parent)
{
    const hexnet_io_profile_t *p = hexnet_io_profile_get();
    int slots[HEXNET_IO_DIM_SLOTS];
    int types[HEXNET_IO_DIM_SLOTS];
    int n = 0;
    const int head_h = 8;
    const int margin_x = 8;
    const int gap_x = 10;
    const int gap_y = 6;
    const int cols = 2;
    const int cell_w = (DASH_W - 2 * margin_x - gap_x) / cols;
    const int cell_h = (DASH_TILE_H - head_h - 3 * gap_y) / 4;
    int i;

    s_dim_count = 0;
    memset(s_dims, 0, sizeof(s_dims));
    s_dim_link = get_canbus_connection_status() != 0;
    lv_obj_set_style_bg_color(parent, lv_color_hex(DIM_COL_PAGE), 0);
    lv_obj_set_style_bg_opa(parent, LV_OPA_COVER, 0);

    for (i = 0; i < HEXNET_IO_DIM_SLOTS; i++) {
        if (p && (p->dim_enable_mask & (1u << i))) {
            slots[n] = i;
            types[n] = p->dim_type[i];
            n++;
        }
    }
    if (n == 0) {
        lv_obj_t *msg = make_label(parent,
                                   vango_tx("No active dimmer", "Aktif dimmer bulunmuyor", "Kein aktiver Dimmer"),
                                   &vango_font_18, COL_MUTED);
        lv_obj_align(msg, LV_ALIGN_CENTER, 0, head_h / 2);
        return;
    }

    for (i = 0; i < n; i++) {
        int col = i % cols;
        int row = i / cols;
        int x = margin_x + col * (cell_w + gap_x);
        int y = head_h + row * (cell_h + gap_y);
        add_dim_card(parent, x, y, cell_w, cell_h, slots[i], types[i]);
    }
}

static const char *relay_name(int idx)
{
    static const char *names_tr[RELAY_TOTAL] = {
        "Röle 1", "Röle 2", "Röle 3", "Röle 4", "Röle 5", "Röle 6",
        "Röle 7", "Röle 8", "Röle 9", "Röle 10", "Röle 11", "Röle 12"
    };
    static const char *names_en[RELAY_TOTAL] = {
        "Relay 1", "Relay 2", "Relay 3", "Relay 4", "Relay 5", "Relay 6",
        "Relay 7", "Relay 8", "Relay 9", "Relay 10", "Relay 11", "Relay 12"
    };
    static const char *names_de[RELAY_TOTAL] = {
        "Relais 1", "Relais 2", "Relais 3", "Relais 4", "Relais 5", "Relais 6",
        "Relais 7", "Relais 8", "Relais 9", "Relais 10", "Relais 11", "Relais 12"
    };
    if (idx < 0 || idx >= RELAY_TOTAL) {
        return "";
    }
    return vango_tx(names_en[idx], names_tr[idx], names_de[idx]);
}

static void fill_relay_grid(lv_obj_t *parent, int start_idx)
{
    for (int i = 0; i < RELAYS_PER_PAGE; i++) {
        int idx = start_idx + i;
        int col = i % 3;
        int row = i / 3;
        add_light_card(parent, 20 + col * 250, 8 + row * 136, 236, 120, relay_name(idx), false);
    }
}

static void relay_tv_changed_cb(lv_event_t *e)
{
    if (s_syncing_relay_tv) {
        return;
    }
    lv_obj_t *tv = lv_event_get_target(e);
    lv_obj_t *tile = lv_tileview_get_tile_act(tv);
    if (!tile) {
        return;
    }
    int sub = (int)(intptr_t)lv_obj_get_user_data(tile);
    if (sub < 0 || sub >= RELAY_SUBPAGE_COUNT || sub == s_relay_subpage) {
        return;
    }
    s_relay_subpage = sub;
    update_relay_dots(sub);
}

extern const char *lblBtnNames[18];
extern const char *lblBtnNames_TR[18];
extern const char *lblBtnNames_DE[18];

static int relay_cfg_type(int slot)
{
    const hexnet_io_profile_t *p = hexnet_io_profile_get();
    const hexnet_io_labels_t *L = hexnet_io_labels_get();
    int type = 0;
    if (L && slot >= 0 && slot < HEXNET_LABEL_RELAYS && L->relays[slot].icon[0]) {
        type = (int)hexnet_icon_key_to_preset(L->relays[slot].icon);
    } else if (p && slot >= 0 && slot < HEXNET_IO_RELAY_SLOTS) {
        type = p->relay_type[slot];
    }
    if (type < 1 || type > 18) {
        type = 1;
    }
    return type;
}

static const char *relay_cfg_name(int slot)
{
    const hexnet_io_labels_t *L = hexnet_io_labels_get();
    const hexnet_io_profile_t *p = hexnet_io_profile_get();
    int type;
    if (L && slot >= 0 && slot < HEXNET_LABEL_RELAYS && L->relays[slot].name[0]) {
        return L->relays[slot].name; /* tablet ozel isim (IO SoT) */
    }
    type = 0;
    if (p && slot >= 0 && slot < HEXNET_IO_RELAY_SLOTS) {
        type = p->relay_type[slot];
    }
    if (type >= 1 && type <= 18) {
        return vango_tx(lblBtnNames[type - 1], lblBtnNames_TR[type - 1],
                        lblBtnNames_DE[type - 1]);
    }
    return relay_name(slot);
}

static const void *relay_cfg_image(int slot)
{
    const hexnet_io_labels_t *L = hexnet_io_labels_get();
    const lv_img_dsc_t *panel;
    const void *img;
    if (L && slot >= 0 && slot < HEXNET_LABEL_RELAYS && L->relays[slot].icon[0]) {
        panel = vango_panel_icon_by_key(L->relays[slot].icon);
        if (panel) {
            return panel;
        }
    }
    img = get_image_for_button(relay_cfg_type(slot) - 1);
    return img ? img : (const void *)&vango_nav_power;
}

/** Panel ayari: kanal aktif mi (ana sayfada kenar rengi). */
static bool relay_slot_enabled(int slot)
{
    const hexnet_io_profile_t *p = hexnet_io_profile_get();
    return p && slot >= 0 && slot < HEXNET_IO_RELAY_SLOTS &&
           (p->relay_enable_mask & (1u << slot)) != 0;
}

static bool relay_slot_live(int slot)
{
    bool enabled = relay_slot_enabled(slot);
    if (s_demo) {
        return enabled;
    }
    return enabled && get_canbus_connection_status() != 0;
}

static void relay_mark_pending(int slot, bool on)
{
    if (slot < 0 || slot >= HOME_RELAYS) {
        return;
    }
    s_relay_pending_ms[slot] = lv_tick_get();
    if (s_relay_pending_ms[slot] == 0) {
        s_relay_pending_ms[slot] = 1; /* 0 reserved = idle */
    }
    s_relay_pending_want[slot] = on;
}

static void relay_clear_pending(int slot)
{
    if (slot < 0 || slot >= HOME_RELAYS) {
        return;
    }
    s_relay_pending_ms[slot] = 0;
}

static bool relay_pending_active(int slot)
{
    if (slot < 0 || slot >= HOME_RELAYS || s_relay_pending_ms[slot] == 0) {
        return false;
    }
    if (lv_tick_elaps(s_relay_pending_ms[slot]) >= RELAY_PENDING_MS) {
        relay_clear_pending(slot);
        return false;
    }
    return true;
}

static void relay_send(int slot, bool on)
{
    uint8_t data[8] = {0};
    if (slot < 0 || slot >= HOME_RELAYS || get_canbus_connection_status() == 0) {
        return;
    }
    data[0] = (uint8_t)slot;
    data[1] = on ? 1u : 0u;
    relay_mark_pending(slot, on);
    send_can_frame(0x720, data);
}

static void relay_paint_one(int slot)
{
    lv_obj_t *card = s_relay_card[slot];
    bool enabled;
    bool live;
    bool on;
    uint32_t border;
    if (!card || slot < 0 || slot >= HOME_RELAYS) {
        return;
    }
    enabled = relay_slot_enabled(slot);
    live = relay_slot_live(slot);
    on = s_relay_on[slot];
    /* Aktif = mavi kenar; pasif = kenar yok (COL_CARD_BORDER bile mavi tonlu kalir). */
    border = enabled ? COL_ACCENT : COL_CARD;
    lv_obj_set_style_bg_color(card, lv_color_hex((live && on) ? 0x14532Du : COL_CARD), 0);
    lv_obj_set_style_border_color(card, lv_color_hex(border), 0);
    lv_obj_set_style_border_width(card, enabled ? 2 : 0, 0);
    lv_obj_set_style_border_opa(card, enabled ? LV_OPA_COVER : LV_OPA_TRANSP, 0);
    lv_obj_set_style_opa(card, enabled ? LV_OPA_COVER : LV_OPA_60, 0);
    if (live) {
        lv_obj_add_flag(card, LV_OBJ_FLAG_CLICKABLE);
    } else {
        lv_obj_clear_flag(card, LV_OBJ_FLAG_CLICKABLE);
    }
    if (s_relay_nm[slot]) {
        lv_label_set_text(s_relay_nm[slot], relay_cfg_name(slot));
        lv_obj_set_style_text_color(s_relay_nm[slot],
                                    lv_color_hex(enabled ? COL_TEXT : COL_MUTED), 0);
    }
    if (s_relay_ico[slot]) {
        const lv_img_dsc_t *src = (const lv_img_dsc_t *)relay_cfg_image(slot);
        lv_img_set_src(s_relay_ico[slot], src);
        /* ALPHA_8BIT: zoom 256 zorunlu (degilse bos cizer) */
        lv_img_set_zoom(s_relay_ico[slot], 256);
        lv_obj_set_style_transform_zoom(s_relay_ico[slot], LV_IMG_ZOOM_NONE, 0);
        lv_obj_set_style_img_recolor(s_relay_ico[slot],
                                     lv_color_hex(enabled
                                                      ? ((live && on) ? COL_GLOW_G : COL_ACCENT)
                                                      : COL_MUTED), 0);
        lv_obj_set_style_img_recolor_opa(s_relay_ico[slot], LV_OPA_COVER, 0);
    }
    /* Açık/Kapalı satırı yok — durum kart rengi + ikon rengi ile */
    if (s_relay_st[slot]) {
        lv_obj_add_flag(s_relay_st[slot], LV_OBJ_FLAG_HIDDEN);
    }
}

void vango_dash_refresh_channel_meta(void)
{
    int i;
    const hexnet_io_labels_t *L = hexnet_io_labels_get();
    ESP_LOGW(TAG, "channel meta paint cards=%d nm0='%s' L0='%s'",
             s_relay_card[0] ? 1 : 0,
             (s_relay_nm[0] && lv_obj_is_valid(s_relay_nm[0]))
                 ? lv_label_get_text(s_relay_nm[0]) : "-",
             (L && L->relays[0].name[0]) ? L->relays[0].name : "-");
    for (i = 0; i < HOME_RELAYS; i++) {
        relay_paint_one(i);
        if (s_relay_card[i]) {
            lv_obj_invalidate(s_relay_card[i]);
        }
    }
    if (s_relay_nm[0] && lv_obj_is_valid(s_relay_nm[0])) {
        const char *painted = lv_label_get_text(s_relay_nm[0]);
        ESP_LOGW(TAG, "channel meta after paint nm0='%s'", painted ? painted : "-");
    }
}

void vango_dash_test_relay(int slot, bool on)
{
    if (slot < 0 || slot >= HOME_RELAYS) {
        return;
    }
    s_relay_on[slot] = on;
    relay_send(slot, on); /* marks pending — sync won't flicker grey */
    relay_paint_one(slot);
}

void vango_dash_test_dim(int slot, int pct)
{
    if (slot < 0 || slot >= DIM_DRIVE_SLOTS) {
        return;
    }
    if (pct < 0) {
        pct = 0;
    }
    if (pct > 100) {
        pct = 100;
    }
    dim_send(slot, pct);
    if (slot < s_dim_count) {
        dim_card_t *d = &s_dims[slot];
        d->brightness = (uint8_t)pct;
        d->enabled = pct > 0;
        if (d->slider) {
            lv_slider_set_value(d->slider, pct, LV_ANIM_OFF);
        }
        if (d->sw) {
            if (pct > 0) {
                lv_obj_add_state(d->sw, LV_STATE_CHECKED);
            } else {
                lv_obj_clear_state(d->sw, LV_STATE_CHECKED);
            }
        }
    }
}

void vango_dash_test_motor(int cmd)
{
    uint8_t data[8] = {0};
    data[0] = (uint8_t)cmd;
    /* Test: link kontrolu yok — 0x750 her zaman bas */
    send_can_frame(0x750, data);
}

static void relay_click_cb(lv_event_t *e)
{
    int slot = (int)(intptr_t)lv_event_get_user_data(e);
    if (slot < 0 || slot >= HOME_RELAYS || !relay_slot_live(slot)) {
        return;
    }
    s_relay_on[slot] = !s_relay_on[slot];
    relay_send(slot, s_relay_on[slot]);
    relay_paint_one(slot);
}

static void motor_paint(void);
static void motor_halt(void);
static void water_show(int clean, int waste, int grey);

static int s_demo_clean = 82;
static int s_demo_waste = 18;
static int s_demo_grey = 12;
static int s_demo_water_dir = -1;

static void water_demo_apply(void)
{
    if (!s_demo || get_canbus_connection_status()) {
        return;
    }
    water_show(s_demo_clean, s_demo_waste, s_demo_grey);
}

static void water_demo_tick(lv_timer_t *timer)
{
    (void)timer;
    if (!s_demo || get_canbus_connection_status()) {
        return;
    }
    s_demo_clean += s_demo_water_dir;
    s_demo_waste -= s_demo_water_dir;
    s_demo_grey -= s_demo_water_dir;
    if (s_demo_clean < 0) {
        s_demo_clean = 0;
    }
    if (s_demo_clean > 100) {
        s_demo_clean = 100;
    }
    if (s_demo_waste < 0) {
        s_demo_waste = 0;
    }
    if (s_demo_waste > 100) {
        s_demo_waste = 100;
    }
    if (s_demo_grey < 0) {
        s_demo_grey = 0;
    }
    if (s_demo_grey > 100) {
        s_demo_grey = 100;
    }
    if (s_demo_clean <= 8 || s_demo_waste >= 98 || s_demo_grey >= 98) {
        s_demo_water_dir = 1;
    } else if (s_demo_clean >= 90) {
        s_demo_water_dir = -1;
    }
    water_show(s_demo_clean, s_demo_waste, s_demo_grey);
}

static void demo_badge_refresh(void)
{
    if (!s_demo_badge) {
        return;
    }
    if (s_demo) {
        lv_label_set_text(s_demo_badge, vango_tx("DEMO", "DEMO", "DEMO"));
        lv_obj_clear_flag(s_demo_badge, LV_OBJ_FLAG_HIDDEN);
    } else {
        lv_obj_add_flag(s_demo_badge, LV_OBJ_FLAG_HIDDEN);
    }
}

static void demo_apply_side_effects(void)
{
    int i;
    if (!s_demo && get_canbus_connection_status()) {
        uint16_t bits = get_outputs();
        for (i = 0; i < HOME_RELAYS; i++) {
            s_relay_on[i] = (bits & (1u << i)) != 0;
        }
    }
    for (i = 0; i < HOME_RELAYS; i++) {
        relay_paint_one(i);
    }
    for (i = 0; i < s_dim_count; i++) {
        dim_apply(&s_dims[i]);
    }
    if (!s_demo) {
        motor_halt();
        if (!get_canbus_connection_status()) {
            water_show(-1, -1, -1);
        }
    } else {
        water_demo_apply();
    }
    motor_paint();
    demo_badge_refresh();
}

bool vango_dash_get_demo(void)
{
    return s_demo;
}

void vango_dash_set_demo(bool on)
{
    if (on && get_canbus_connection_status() != 0) {
        on = false;
    }
    if (s_demo == on) {
        demo_badge_refresh();
        return;
    }
    s_demo = on;
    demo_apply_side_effects();
}

static void build_home_page(lv_obj_t *parent)
{
    /* Üst+alt margin ve ara boşluk tile yüksekliğine sığmalı — yoksa alt sıra kesilir. */
    const int margin = 6;
    const int gap = 6;
    const int cols = 6;
    const int rows = 2;
    const int card_w = (DASH_W - 2 * margin - (cols - 1) * gap) / cols;
    const int card_h = (DASH_TILE_H - 2 * margin - (rows - 1) * gap) / rows;
    uint16_t bits = get_canbus_connection_status() ? get_outputs() : 0;
    int i;

    for (i = 0; i < HOME_RELAYS; i++) {
        int col = i % cols;
        int row = i / cols;
        int x = margin + col * (card_w + gap);
        int y = margin + row * (card_h + gap);
        lv_obj_t *card = lv_obj_create(parent);
        lv_obj_t *nm;
        lv_obj_t *ico;
        s_relay_on[i] = (bits & (1u << i)) != 0;
        s_relay_card[i] = card;
        lv_obj_set_size(card, card_w, card_h);
        lv_obj_set_pos(card, x, y);
        style_card(card);
        lv_obj_set_style_border_width(card, 2, 0);
        lv_obj_add_event_cb(card, relay_click_cb, LV_EVENT_CLICKED, (void *)(intptr_t)i);

        /* Panel ikon: ALPHA_8BIT native (56→96), zoom=256 */
        ico = lv_img_create(card);
        {
            const lv_img_dsc_t *src = (const lv_img_dsc_t *)relay_cfg_image(i);
            lv_img_set_src(ico, src);
            lv_img_set_zoom(ico, 256);
        }
        lv_obj_align(ico, LV_ALIGN_TOP_MID, 0, 10);
        lv_obj_set_style_img_recolor(ico, lv_color_hex(COL_MUTED), 0);
        lv_obj_set_style_img_recolor_opa(ico, LV_OPA_COVER, 0);
        lv_obj_clear_flag(ico, LV_OBJ_FLAG_CLICKABLE);
        lv_obj_add_flag(ico, LV_OBJ_FLAG_EVENT_BUBBLE);
        s_relay_ico[i] = ico;

        nm = make_label(card, relay_cfg_name(i), &vango_font_18, COL_TEXT);
        lv_obj_set_width(nm, card_w - 6);
        lv_obj_set_height(nm, 42);
        lv_label_set_long_mode(nm, LV_LABEL_LONG_WRAP);
        lv_obj_set_style_text_align(nm, LV_TEXT_ALIGN_CENTER, 0);
        lv_obj_set_style_text_line_space(nm, -1, 0);
        lv_obj_align(nm, LV_ALIGN_BOTTOM_MID, 0, -14);
        lv_obj_add_flag(nm, LV_OBJ_FLAG_EVENT_BUBBLE);
        s_relay_nm[i] = nm;

        s_relay_st[i] = NULL;
        relay_paint_one(i);
    }
}

/** Röleler — 12 çıkış, 2 sayfa × 6 (yan kaydırma) */
static void build_relays_page(lv_obj_t *parent)
{
    s_relay_tv = lv_tileview_create(parent);
    lv_obj_set_size(s_relay_tv, DASH_W, DASH_TILE_H);
    lv_obj_set_pos(s_relay_tv, 0, 0);
    lv_obj_set_style_bg_color(s_relay_tv, lv_color_hex(COL_BG), 0);
    lv_obj_set_style_bg_opa(s_relay_tv, LV_OPA_COVER, 0);
    lv_obj_set_style_pad_all(s_relay_tv, 0, 0);
    lv_obj_add_event_cb(s_relay_tv, tileview_scroll_begin_cb, LV_EVENT_SCROLL_BEGIN, NULL);
    lv_obj_add_event_cb(s_relay_tv, relay_tv_changed_cb, LV_EVENT_VALUE_CHANGED, NULL);

    for (int p = 0; p < RELAY_SUBPAGE_COUNT; p++) {
        lv_dir_t dir = LV_DIR_LEFT | LV_DIR_RIGHT;
        if (p == 0) {
            dir = LV_DIR_RIGHT;
        } else if (p == RELAY_SUBPAGE_COUNT - 1) {
            dir = LV_DIR_LEFT;
        }
        lv_obj_t *tile = lv_tileview_add_tile(s_relay_tv, p, 0, dir);
        lv_obj_set_user_data(tile, (void *)(intptr_t)p);
        lv_obj_set_style_bg_color(tile, lv_color_hex(COL_BG), 0);
        lv_obj_set_style_bg_opa(tile, LV_OPA_COVER, 0);
        lv_obj_set_style_pad_all(tile, 0, 0);
        lv_obj_clear_flag(tile, LV_OBJ_FLAG_SCROLLABLE);
        fill_relay_grid(tile, p * RELAYS_PER_PAGE);
    }

    s_relay_subpage = 0;
    s_syncing_relay_tv = true;
    lv_obj_set_tile_id(s_relay_tv, 0, 0, LV_ANIM_OFF);
    s_syncing_relay_tv = false;
    update_relay_dots(0);
}

static const lv_img_dsc_t *tank_drop_icon(int idx)
{
    if (idx == 0) {
        return &vango_icon_drop_clean;
    }
    if (idx == 1) {
        return &vango_icon_drop_waste;
    }
    return &vango_icon_drop_grey;
}

/** 0..100 geçerli seviye. Aralık dışı veya kopuk okuma geçersizdir. */
static int water_normalize(int pct)
{
    if (pct < 0 || pct > 100) {
        return -1;
    }
    return pct;
}

static void water_wave_paint(int idx)
{
    lv_obj_t *wave = s_water_wave[idx];
    int amp = s_wave_amp[idx];
    int w = s_wave_span[idx];
    int bh;
    int x;
    int y;
    int swing;
    lv_color_t water;
    lv_color_t empty;
    lv_img_dsc_t *dsc;
    if (!wave || amp < 2 || w < 4) {
        return;
    }
    if (w > WATER_WAVE_MAX_W) {
        w = WATER_WAVE_MAX_W;
    }
    bh = amp * 2;
    dsc = lv_canvas_get_img(wave);
    if (!dsc || dsc->header.w != (uint32_t)w || dsc->header.h != (uint32_t)bh) {
        lv_canvas_set_buffer(wave, s_wave_buf[idx], w, bh, LV_IMG_CF_TRUE_COLOR);
        dsc = lv_canvas_get_img(wave);
    }
    water = s_wave_color[idx];
    empty = lv_color_hex(0x071827u);
    swing = amp - 1;
    for (x = 0; x < w; x++) {
        int ang = (s_water_phase + (x * 480) / w) % 360;
        int ang2 = ((s_water_phase * 2) + 70 + (x * 960) / w) % 360;
        int s = (lv_trigo_sin((int16_t)ang) * 3 + lv_trigo_sin((int16_t)ang2)) / 4;
        int surf = amp * 256 - (s * swing * 256) / 32767;
        int sy = surf >> 8;
        int frac = surf & 255;
        if (sy < 0) {
            sy = 0;
        }
        if (sy >= bh) {
            sy = bh - 1;
        }
        for (y = 0; y < bh; y++) {
            lv_color_t c;
            if (y < sy) {
                c = empty;
            } else if (y == sy) {
                c = lv_color_mix(empty, water, (lv_opa_t)frac);
            } else {
                c = water;
            }
            lv_img_buf_set_px_color(dsc, x, y, c);
        }
    }
    lv_img_cache_invalidate_src(dsc);
    lv_obj_invalidate(wave);
}

static int water_wave_amp_for(int idx, lv_coord_t h)
{
    int amp = WATER_WAVE_AMP;
    int above;
    int below;
    int body_max;
    if (h < 4) {
        return 0;
    }
    above = s_water_max[idx] - h;
    below = h;
    body_max = s_water_max[idx] - WATER_RIM + 1;
    if (amp > above) {
        amp = above;
    }
    if (amp > below) {
        amp = below;
    }
    if (body_max > 0 && h + amp > body_max) {
        amp = body_max - h;
    }
    if (amp < 2) {
        return 0;
    }
    return amp;
}

static void water_place_surface(int idx, lv_coord_t h)
{
    lv_obj_t *fill = s_water_fill[idx];
    lv_obj_t *neck = s_water_neck[idx];
    lv_obj_t *wave = s_water_wave[idx];
    lv_coord_t body_max;
    lv_coord_t body_h;
    lv_coord_t neck_h = 0;
    lv_coord_t solid;
    int amp;
    if (!fill) {
        return;
    }
    if (h > s_water_max[idx]) {
        h = s_water_max[idx];
    }
    if (h < 0) {
        h = 0;
    }
    s_water_px[idx] = h;
    if (h < 1) {
        lv_obj_set_height(fill, 0);
        lv_obj_add_flag(fill, LV_OBJ_FLAG_HIDDEN);
        if (neck) {
            lv_obj_add_flag(neck, LV_OBJ_FLAG_HIDDEN);
        }
        if (wave) {
            lv_obj_add_flag(wave, LV_OBJ_FLAG_HIDDEN);
        }
        s_wave_amp[idx] = 0;
        return;
    }
    amp = water_wave_amp_for(idx, h);
    s_wave_amp[idx] = amp;
    solid = (lv_coord_t)(h - amp);
    body_max = s_water_max[idx] - WATER_RIM + 1;
    if (body_max < 1) {
        body_max = solid;
    }
    body_h = solid;
    if (body_h > body_max) {
        neck_h = (lv_coord_t)(body_h - body_max);
        body_h = body_max;
    }
    lv_obj_clear_flag(fill, LV_OBJ_FLAG_HIDDEN);
    lv_obj_set_height(fill, body_h);
    if (neck) {
        if (neck_h < 2) {
            lv_obj_add_flag(neck, LV_OBJ_FLAG_HIDDEN);
        } else {
            lv_obj_clear_flag(neck, LV_OBJ_FLAG_HIDDEN);
            lv_obj_set_height(neck, neck_h);
            lv_obj_set_y(neck, WATER_RIM - neck_h);
        }
    }
    if (wave) {
        if (amp < 2) {
            lv_obj_add_flag(wave, LV_OBJ_FLAG_HIDDEN);
        } else {
            lv_obj_clear_flag(wave, LV_OBJ_FLAG_HIDDEN);
            water_wave_paint(idx);
            /* Ortalama yüzey tam h yüksekliğinde. Dalga üstte ve altta eşit. */
            lv_obj_align(wave, LV_ALIGN_BOTTOM_MID, 0, -(solid + 1));
        }
    }
}

static void water_fill_anim_cb(void *var, int32_t v)
{
    int idx = (int)(intptr_t)lv_obj_get_user_data((lv_obj_t *)var);
    if (idx < 0 || idx > 2) {
        return;
    }
    water_place_surface(idx, (lv_coord_t)v);
}

static void water_set_fill(int idx, lv_coord_t h)
{
    lv_obj_t *fill = s_water_fill[idx];
    if (!fill) {
        return;
    }
    lv_anim_del(fill, water_fill_anim_cb);
    if (h < 1) {
        water_place_surface(idx, 0);
        return;
    }
    lv_coord_t cur = s_water_px[idx];
    if (cur == h) {
        water_place_surface(idx, h);
        return;
    }
    lv_anim_t a;
    lv_anim_init(&a);
    lv_anim_set_var(&a, fill);
    lv_anim_set_exec_cb(&a, water_fill_anim_cb);
    lv_anim_set_values(&a, cur, h);
    lv_anim_set_time(&a, 180);
    lv_anim_set_path_cb(&a, lv_anim_path_ease_out);
    lv_anim_start(&a);
}

static void water_place_readout(int idx, bool show_liters)
{
    lv_obj_t *pct = s_water_pct_lbl[idx];
    lv_obj_t *ltr = s_water_ltr_lbl[idx];
    if (pct) {
        lv_obj_align(pct, LV_ALIGN_CENTER, 0, show_liters ? -14 : 0);
        lv_obj_move_foreground(pct);
    }
    if (!ltr) {
        return;
    }
    if (show_liters) {
        lv_obj_clear_flag(ltr, LV_OBJ_FLAG_HIDDEN);
        lv_obj_align(ltr, LV_ALIGN_CENTER, 0, 16);
        lv_obj_move_foreground(ltr);
    } else {
        lv_obj_add_flag(ltr, LV_OBJ_FLAG_HIDDEN);
    }
}

static void tank_cap_load(void)
{
    nvs_handle_t h;
    uint16_t buf[3];
    size_t sz = sizeof(buf);
    memset(buf, 0, sizeof(buf));
    if (nvs_open("hexnet", NVS_READONLY, &h) != ESP_OK) {
        return;
    }
    if (nvs_get_blob(h, "tank_l", buf, &sz) == ESP_OK && sz == sizeof(buf)) {
        for (int i = 0; i < 3; i++) {
            s_tank_cap[i] = buf[i];
        }
    }
    nvs_close(h);
}

static void tank_cap_save(void)
{
    nvs_handle_t h;
    uint16_t buf[3];
    if (nvs_open("hexnet", NVS_READWRITE, &h) != ESP_OK) {
        return;
    }
    for (int i = 0; i < 3; i++) {
        buf[i] = (uint16_t)s_tank_cap[i];
    }
    nvs_set_blob(h, "tank_l", buf, sizeof(buf));
    nvs_commit(h);
    nvs_close(h);
}

/* LVGL/PSRAM stack'ten NVS yasak (cache assert → reset). Ayri task. */
static void tank_cap_save_task(void *arg)
{
    (void)arg;
    vTaskDelay(pdMS_TO_TICKS(80));
    tank_cap_save();
    vTaskDelete(NULL);
}

static void tank_cap_schedule_save(void)
{
    BaseType_t ok = xTaskCreate(tank_cap_save_task, "tank_nvs", 3072, NULL, 2, NULL);
    if (ok != pdPASS) {
        ESP_LOGW(TAG, "tank_nvs task fail — skip flash");
    }
}

static void water_apply_tank(int idx, int pct)
{
    if (idx < 0 || idx > 2) {
        return;
    }
    const bool ok = (pct >= 0 && pct <= 100);
    if (s_water_pct_lbl[idx]) {
        char buf[16];
        if (ok) {
            snprintf(buf, sizeof(buf), "%d%%", pct);
        } else {
            snprintf(buf, sizeof(buf), "--%%");
        }
        lv_label_set_text(s_water_pct_lbl[idx], buf);
    }
    {
        bool show_l = false;
        if (s_water_ltr_lbl[idx] && ok && s_tank_cap[idx] > 0) {
            char buf[16];
            int remain = (s_tank_cap[idx] * pct) / 100;
            snprintf(buf, sizeof(buf), "%d L", remain);
            lv_label_set_text(s_water_ltr_lbl[idx], buf);
            show_l = true;
        }
        water_place_readout(idx, show_l);
    }
    lv_coord_t h = 0;
    if (ok && pct > 0 && s_water_max[idx] > 0) {
        h = (lv_coord_t)((s_water_max[idx] * pct) / 100);
    }
    water_set_fill(idx, h);
}

static lv_obj_t *make_wave(lv_obj_t *vessel, int w, uint32_t col, int idx)
{
    lv_obj_t *canvas = lv_canvas_create(vessel);
    if (w > WATER_WAVE_MAX_W) {
        w = WATER_WAVE_MAX_W;
    }
    if (w < 4) {
        w = 4;
    }
    s_wave_span[idx] = w;
    s_wave_color[idx] = lv_color_hex(col);
    s_wave_amp[idx] = WATER_WAVE_AMP;
    lv_canvas_set_buffer(canvas, s_wave_buf[idx], w, WATER_WAVE_AMP * 2, LV_IMG_CF_TRUE_COLOR);
    lv_obj_set_style_border_width(canvas, 0, 0);
    lv_obj_clear_flag(canvas, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(canvas, LV_OBJ_FLAG_HIDDEN);
    return canvas;
}

static void water_bubbles_tick(void)
{
    static const int xpct[WATER_BUB_N] = { 10, 24, 76, 88 };
    /* 16ths of a pixel per 60 ms tick. Steps stay 1 px; rise is about 8–11 px/s. */
    static const int speed[WATER_BUB_N] = { 8, 12, 9, 11 };
    static const int seed[WATER_BUB_N] = { 17, 53, 29, 71 };
    static const int sway_px[WATER_BUB_N] = { 2, 1, 2, 1 };
    int idx;
    int i;

    s_bub_rise = (uint16_t)(s_bub_rise + 1);
    for (idx = 0; idx < 3; idx++) {
        lv_obj_t *fill = s_water_fill[idx];
        lv_coord_t fh;
        lv_coord_t fw;
        int span;
        if (!fill) {
            continue;
        }
        fh = lv_obj_get_height(fill);
        fw = lv_obj_get_width(fill);
        span = (int)fh - 18;
        for (i = 0; i < WATER_BUB_N; i++) {
            lv_obj_t *b = s_water_bub[idx][i];
            unsigned along;
            int y;
            int edge;
            int opa;
            int wob;
            int x;
            if (!b) {
                continue;
            }
            if (lv_obj_has_flag(fill, LV_OBJ_FLAG_HIDDEN) || span < 20) {
                lv_obj_add_flag(b, LV_OBJ_FLAG_HIDDEN);
                continue;
            }
            along = (((unsigned)s_bub_rise * (unsigned)speed[i]) / 16u
                     + (unsigned)seed[i] * (unsigned)(idx + 2)) % (unsigned)span;
            y = span - 1 - (int)along;
            edge = span / 6;
            if (edge < 10) {
                edge = 10;
            }
            if (y < edge) {
                opa = (y * LV_OPA_50) / edge;
            } else if (y > span - edge) {
                opa = ((span - y) * LV_OPA_50) / edge;
            } else {
                opa = LV_OPA_50;
            }
            wob = (lv_trigo_sin((int16_t)((s_bub_rise * 2 + seed[i] * 24 + idx * 40) % 360))
                   * sway_px[i]) / 32767;
            x = (fw * xpct[i]) / 100 + wob;
            if (x < 2) {
                x = 2;
            }
            if (x > (int)fw - 8) {
                x = (int)fw - 8;
            }
            lv_obj_clear_flag(b, LV_OBJ_FLAG_HIDDEN);
            lv_obj_set_pos(b, x, y);
            lv_obj_set_style_bg_opa(b, (lv_opa_t)opa, 0);
        }
    }
}

static void water_live_tick(lv_timer_t *timer)
{
    int idx;
    (void)timer;
    s_water_phase = (uint16_t)((s_water_phase + 4) % 360);
    for (idx = 0; idx < 3; idx++) {
        lv_obj_t *wave = s_water_wave[idx];
        if (!wave || lv_obj_has_flag(wave, LV_OBJ_FLAG_HIDDEN)) {
            continue;
        }
        water_wave_paint(idx);
    }
    water_bubbles_tick();
}

static void add_tank_card(lv_obj_t *parent, int x, int y, int w, int h,
                          const char *name, uint32_t fill_col, int idx)
{
    const int pad = w / 20;
    const int scale_w = 40;
    const int head_h = 46;
    const int vessel_w = w - pad * 2 - scale_w;
    const int vessel_h = h - head_h - pad;
    static const char *marks[] = { "100%", "75%", "50%", "25%", "0%" };
    int i;

    lv_obj_t *card = lv_obj_create(parent);
    lv_obj_set_size(card, w, h);
    lv_obj_set_pos(card, x, y);
    style_card(card);
    lv_obj_set_style_bg_color(card, lv_color_hex(0x0D2030u), 0);
    lv_obj_set_style_radius(card, 16, 0);
    lv_obj_set_style_border_width(card, 1, 0);
    lv_obj_set_style_border_color(card, lv_color_hex(0x294455u), 0);
    lv_obj_set_style_border_opa(card, LV_OPA_COVER, 0);
    lv_obj_clear_flag(card, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *ico = lv_img_create(card);
    lv_img_set_src(ico, tank_drop_icon(idx));
    lv_img_set_zoom(ico, 140);
    lv_obj_align(ico, LV_ALIGN_TOP_LEFT, pad, 6);
    if (idx == 1) {
        lv_obj_set_style_img_recolor(ico, lv_color_hex(fill_col), 0);
        lv_obj_set_style_img_recolor_opa(ico, LV_OPA_COVER, 0);
    }
    lv_obj_clear_flag(ico, LV_OBJ_FLAG_CLICKABLE);

    lv_obj_t *title = make_label(card, name, &vango_font_16, 0xFFFFFF);
    lv_obj_align(title, LV_ALIGN_TOP_LEFT, pad + 34, 16);

    lv_obj_t *vessel = lv_obj_create(card);
    lv_obj_set_size(vessel, vessel_w, vessel_h);
    lv_obj_set_pos(vessel, pad, head_h);
    lv_obj_set_style_bg_opa(vessel, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(vessel, 0, 0);
    lv_obj_set_style_radius(vessel, 0, 0);
    lv_obj_set_style_pad_all(vessel, 0, 0);
    lv_obj_clear_flag(vessel, LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_CLICKABLE);

    /* Üst kenar yuvarlak. Alt gövde radius 0, köşeler dik. */
    lv_obj_t *cap = lv_obj_create(vessel);
    lv_obj_set_size(cap, vessel_w, WATER_RIM * 2);
    lv_obj_set_pos(cap, 0, 0);
    lv_obj_set_style_bg_color(cap, lv_color_hex(0x071827u), 0);
    lv_obj_set_style_bg_opa(cap, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(cap, WATER_RIM, 0);
    lv_obj_set_style_clip_corner(cap, true, 0);
    lv_obj_set_style_border_width(cap, 1, 0);
    lv_obj_set_style_border_color(cap, lv_color_hex(0x8AA4B8u), 0);
    lv_obj_set_style_border_opa(cap, LV_OPA_70, 0);
    lv_obj_set_style_pad_all(cap, 0, 0);
    lv_obj_clear_flag(cap, LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_CLICKABLE);

    lv_obj_t *body = lv_obj_create(vessel);
    lv_obj_set_size(body, vessel_w, vessel_h - WATER_RIM);
    lv_obj_set_pos(body, 0, WATER_RIM);
    lv_obj_set_style_bg_color(body, lv_color_hex(0x071827u), 0);
    lv_obj_set_style_bg_opa(body, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(body, 0, 0);
    lv_obj_set_style_border_width(body, 1, 0);
    lv_obj_set_style_border_side(body,
                                 LV_BORDER_SIDE_LEFT | LV_BORDER_SIDE_RIGHT | LV_BORDER_SIDE_BOTTOM, 0);
    lv_obj_set_style_border_color(body, lv_color_hex(0x8AA4B8u), 0);
    lv_obj_set_style_border_opa(body, LV_OPA_70, 0);
    lv_obj_set_style_pad_all(body, 0, 0);
    lv_obj_clear_flag(body, LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_CLICKABLE);
    s_water_max[idx] = (lv_coord_t)(vessel_h - 2);

    lv_obj_t *fill = lv_obj_create(vessel);
    lv_obj_set_width(fill, vessel_w - 2);
    lv_obj_set_height(fill, 0);
    lv_obj_align(fill, LV_ALIGN_BOTTOM_MID, 0, -1);
    lv_obj_set_style_bg_color(fill, lv_color_hex(fill_col), 0);
    lv_obj_set_style_bg_opa(fill, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(fill, 0, 0);
    lv_obj_set_style_border_width(fill, 0, 0);
    lv_obj_clear_flag(fill, LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_flag(fill, LV_OBJ_FLAG_HIDDEN);
    lv_obj_set_user_data(fill, (void *)(intptr_t)idx);
    s_water_fill[idx] = fill;
    {
        static const int bub_d[WATER_BUB_N] = { 4, 3, 5, 3 };
        for (i = 0; i < WATER_BUB_N; i++) {
            lv_obj_t *b = lv_obj_create(fill);
            lv_obj_set_size(b, bub_d[i], bub_d[i]);
            lv_obj_set_style_radius(b, LV_RADIUS_CIRCLE, 0);
            lv_obj_set_style_bg_color(b, lv_color_hex(0xFFFFFFu), 0);
            lv_obj_set_style_bg_opa(b, LV_OPA_50, 0);
            lv_obj_set_style_border_width(b, 0, 0);
            lv_obj_clear_flag(b, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);
            lv_obj_add_flag(b, LV_OBJ_FLAG_HIDDEN);
            s_water_bub[idx][i] = b;
        }
    }

    lv_obj_t *neck = lv_obj_create(cap);
    lv_obj_set_width(neck, vessel_w - 2);
    lv_obj_set_height(neck, 0);
    lv_obj_set_pos(neck, 1, WATER_RIM);
    lv_obj_set_style_bg_color(neck, lv_color_hex(fill_col), 0);
    lv_obj_set_style_bg_opa(neck, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(neck, 0, 0);
    lv_obj_set_style_border_width(neck, 0, 0);
    lv_obj_clear_flag(neck, LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_flag(neck, LV_OBJ_FLAG_HIDDEN);
    s_water_neck[idx] = neck;

    s_water_wave[idx] = make_wave(vessel, vessel_w - 2, fill_col, idx);

    for (i = 0; i < 5; i++) {
        lv_obj_t *mk = make_label(card, marks[i], &vango_font_12, 0xB0BFCCu);
        int my = head_h + ((vessel_h - 1) * i) / 4 - 6;
        lv_obj_set_pos(mk, pad + vessel_w + 4, my);
        lv_obj_clear_flag(mk, LV_OBJ_FLAG_CLICKABLE);
    }

    lv_obj_t *pct = make_label(vessel, "--%", &vango_font_26, 0xFFFFFF);
    lv_obj_align(pct, LV_ALIGN_CENTER, 0, 0);
    lv_obj_clear_flag(pct, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_move_foreground(pct);
    s_water_pct_lbl[idx] = pct;

    lv_obj_t *ltr = make_label(vessel, "", &vango_font_16, 0xFFFFFF);
    lv_obj_align(ltr, LV_ALIGN_CENTER, 0, 16);
    lv_obj_clear_flag(ltr, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_flag(ltr, LV_OBJ_FLAG_HIDDEN);
    lv_obj_move_foreground(ltr);
    s_water_ltr_lbl[idx] = ltr;
}

static void build_water_page(lv_obj_t *parent)
{
    const int margin = (DASH_W * 25) / 1000;
    const int gap = (DASH_W * 15) / 1000;
    const int card_w = (DASH_W - 2 * margin - 2 * gap) / 3;
    const int card_h = DASH_TILE_H - 8;
    static const uint32_t fill[3] = { 0x00CFEFu, 0x9A6B3Cu, 0x9A9A9Au };
    const char *name[3] = {
        vango_tx("Fresh Water", "Temiz Su", "Frischwasser"),
        vango_tx("Waste Water", "Atık Su", "Abwasser"),
        vango_tx("Grey Water", "Gri Su", "Grauwasser")
    };
    const int levels[3] = { s_w_clean, s_w_waste, s_w_grey };
    int i;

    lv_obj_set_style_bg_color(parent, lv_color_hex(COL_BG), 0);
    lv_obj_set_style_bg_opa(parent, LV_OPA_COVER, 0);
    for (i = 0; i < 3; i++) {
        s_water_fill[i] = NULL;
        s_water_neck[i] = NULL;
        s_water_wave[i] = NULL;
        s_water_bub[i][0] = s_water_bub[i][1] = s_water_bub[i][2] = s_water_bub[i][3] = NULL;
        s_water_pct_lbl[i] = NULL;
        s_water_ltr_lbl[i] = NULL;
        s_water_max[i] = 0;
        s_water_px[i] = 0;
        s_wave_amp[i] = 0;
        add_tank_card(parent, margin + i * (card_w + gap), 4, card_w, card_h, name[i], fill[i], i);
        water_apply_tank(i, levels[i]);
    }
}

extern int motorData;

static bool s_motor_link;

typedef struct {
    lv_obj_t *gear;
    lv_obj_t *arc;
    lv_obj_t *status;
    lv_obj_t *pill;
    lv_obj_t *pill_bg;
    lv_obj_t *dot;
    lv_obj_t *up;
    lv_obj_t *down;
    lv_obj_t *left_lbl;
    lv_obj_t *right_lbl;
    lv_obj_t *arrow;
    lv_obj_t *sw;
    lv_obj_t *lock_btn;
    lv_obj_t *lock_ico;
    lv_obj_t *lock_lbl;
    uint32_t accent;
    int held;
    int spin_dir;
    int gear_live;
    bool drive;
    bool unlocked;
    bool locked;
} motor_ui_t;

/* Survives the motor page rebuild. Tablet: armed on, padlock locked. */
static bool s_motor_armed[2] = { true, true };
static bool s_motor_locked[2] = { true, true };

#define MLOCK 40
static LV_ATTRIBUTE_MEM_ALIGN uint8_t s_mlock_buf[2][MLOCK * MLOCK * 3];

/* Tablet MotorGearDial.drawMetallicGear: 12 diş, tuval 104 px. */
#define MGEAR 104
static LV_ATTRIBUTE_MEM_ALIGN uint8_t s_mgear_buf[2][MGEAR * MGEAR * 3];

static int s_mgear_n = MGEAR;

static void mgear_px(uint8_t *buf, int x, int y, lv_color_t c, uint8_t a)
{
    uint8_t *p;
    const int n = s_mgear_n;
    if (x < 0 || y < 0 || x >= n || y >= n) {
        return;
    }
    p = buf + ((y * n + x) * 3);
    memcpy(p, &c, 2);
    p[2] = a;
}

static void mgear_path(int *xs, int *ys, int *n, int cx, int cy, int outer)
{
    const int teeth = 12;
    const float root = (float)outer * 0.80f;
    int i;
    *n = 0;
    for (i = 0; i < teeth; i++) {
        const float base = i * (360.0f / teeth) - 90.0f;
        const float valley = 360.0f / teeth / 2.0f;
        const float tip_h = valley * 0.42f;
        const float root_h = valley * 0.78f;
        const float next = (i + 1) * (360.0f / teeth) - 90.0f;
        const float ang[4] = { base - root_h, base - tip_h, base + tip_h, base + root_h };
        const float rads[4] = { root, (float)outer, (float)outer, root };
        int k;
        float a1;
        float a2;
        float amid;
        float x0;
        float y0;
        float xc;
        float yc;
        float x1;
        float y1;
        int s;
        for (k = (i == 0) ? 0 : 1; k < 4; k++) {
            const float rad = ang[k] * 0.0174532925f;
            xs[*n] = cx + (int)(cosf(rad) * rads[k]);
            ys[*n] = cy + (int)(sinf(rad) * rads[k]);
            (*n)++;
        }
        a1 = (base + root_h) * 0.0174532925f;
        a2 = (next - root_h) * 0.0174532925f;
        amid = (a1 + a2) * 0.5f;
        x0 = cx + cosf(a1) * root;
        y0 = cy + sinf(a1) * root;
        xc = cx + cosf(amid) * root * 0.98f;
        yc = cy + sinf(amid) * root * 0.98f;
        x1 = cx + cosf(a2) * root;
        y1 = cy + sinf(a2) * root;
        for (s = 1; s <= 4; s++) {
            const float t = s / 4.0f;
            const float u = 1.0f - t;
            xs[*n] = (int)(u * u * x0 + 2.0f * u * t * xc + t * t * x1);
            ys[*n] = (int)(u * u * y0 + 2.0f * u * t * yc + t * t * y1);
            (*n)++;
        }
    }
}

static lv_color_t mgear_body(int x, int y, int cx, int cy, int outer, bool bright)
{
    int u = (((x - cx) + (y - cy)) * 127) / (outer > 0 ? outer : 1) + 128;
    lv_color_t hi;
    lv_color_t lo;
    if (u < 0) {
        u = 0;
    }
    if (u > 255) {
        u = 255;
    }
    if (!bright) {
        hi = lv_color_hex(0x6B7380u);
        lo = lv_color_hex(0x3A4250u);
        return lv_color_mix(hi, lo, (lv_opa_t)(255 - u));
    }
    if (u < 128) {
        hi = lv_color_hex(0xE8EEF5u);
        lo = lv_color_hex(0x8A96A8u);
        return lv_color_mix(hi, lo, (lv_opa_t)(255 - u * 2));
    }
    hi = lv_color_hex(0xD5DCE6u);
    lo = lv_color_hex(0x8A96A8u);
    return lv_color_mix(hi, lo, (lv_opa_t)((u - 128) * 2));
}

static void mgear_disc(uint8_t *buf, int cx, int cy, int r, lv_color_t c, uint8_t a)
{
    int y;
    int x;
    for (y = cy - r; y <= cy + r; y++) {
        for (x = cx - r; x <= cx + r; x++) {
            int dx = x - cx;
            int dy = y - cy;
            if (dx * dx + dy * dy <= r * r) {
                mgear_px(buf, x, y, c, a);
            }
        }
    }
}

static void mgear_ring(uint8_t *buf, int cx, int cy, int r, int w, lv_color_t c, uint8_t a)
{
    int y;
    int x;
    int r0 = r - w;
    if (r0 < 0) {
        r0 = 0;
    }
    for (y = cy - r - 1; y <= cy + r + 1; y++) {
        for (x = cx - r - 1; x <= cx + r + 1; x++) {
            int d = (x - cx) * (x - cx) + (y - cy) * (y - cy);
            if (d <= r * r && d >= r0 * r0) {
                mgear_px(buf, x, y, c, a);
            }
        }
    }
}

static void mgear_render_n(uint8_t *buf, int size, bool bright)
{
    int xs[160];
    int ys[160];
    int n = 0;
    int y;
    int i;
    const int saved = s_mgear_n;
    const int cx = size / 2;
    const int cy = size / 2;
    const int outer = (46 * size) / MGEAR;
    const int root = (outer * 80) / 100;
    const int face = (root * 78) / 100;
    const int hub = (outer * 30) / 100;
    const int bore = (outer * 11) / 100;
    const uint8_t opa = bright ? 255 : 150;
    s_mgear_n = size;
    mgear_path(xs, ys, &n, cx, cy, outer);
    memset(buf, 0, (size_t)size * (size_t)size * 3);
    for (y = 0; y < size; y++) {
        int hit[24];
        int hn = 0;
        int e;
        for (e = 0; e < n; e++) {
            int j = (e + 1) % n;
            int x0 = xs[e];
            int y0 = ys[e];
            int x1 = xs[j];
            int y1 = ys[j];
            if (y0 == y1) {
                continue;
            }
            if (y0 > y1) {
                int tx = x0;
                int ty = y0;
                x0 = x1;
                y0 = y1;
                x1 = tx;
                y1 = ty;
            }
            if (y < y0 || y >= y1 || hn >= 24) {
                continue;
            }
            hit[hn++] = x0 + (int)(((int64_t)(x1 - x0) * (y - y0)) / (y1 - y0));
        }
        for (e = 1; e < hn; e++) {
            int v = hit[e];
            int k = e;
            while (k > 0 && hit[k - 1] > v) {
                hit[k] = hit[k - 1];
                k--;
            }
            hit[k] = v;
        }
        for (e = 0; e + 1 < hn; e += 2) {
            int x;
            for (x = hit[e]; x <= hit[e + 1]; x++) {
                mgear_px(buf, x, y, mgear_body(x, y, cx, cy, outer, bright), opa);
            }
        }
    }
    for (i = 0; i < n; i++) {
        int j = (i + 1) % n;
        int x0 = xs[i];
        int y0 = ys[i];
        int x1 = xs[j];
        int y1 = ys[j];
        int steps = abs(x1 - x0);
        int s;
        if (abs(y1 - y0) > steps) {
            steps = abs(y1 - y0);
        }
        if (steps < 1) {
            steps = 1;
        }
        for (s = 0; s <= steps; s++) {
            mgear_px(buf, x0 + (x1 - x0) * s / steps, y0 + (y1 - y0) * s / steps,
                     lv_color_hex(bright ? 0x5C6777u : 0x3A4250u), opa);
        }
    }
    mgear_disc(buf, cx, cy, face, lv_color_hex(bright ? 0x1A222Eu : 0x121820u), opa);
    mgear_ring(buf, cx, cy, face, 2, lv_color_hex(bright ? 0x6B7788u : 0x4A5564u), opa);
    mgear_disc(buf, cx, cy, hub, lv_color_hex(bright ? 0xF2F5F9u : 0x7A8492u), opa);
    mgear_ring(buf, cx, cy, hub, 2, lv_color_hex(bright ? 0x4A5564u : 0x3A4250u), opa);
    mgear_disc(buf, cx, cy, bore, lv_color_hex(0x0E141Du), opa);
    if (bright) {
        mgear_ring(buf, cx, cy, bore, 1, lv_color_hex(0x7E8A99u), 180);
    }
    s_mgear_n = saved;
}

static void mgear_render(uint8_t *buf, bool bright)
{
    mgear_render_n(buf, MGEAR, bright);
}

#define MGEAR_NAV 40
static uint8_t *s_gear_nav_mem;
static lv_img_dsc_t s_gear_nav_dsc;
static bool s_gear_nav_ready;

static void mgear_fit(const uint8_t *src, int sw, uint8_t *dst, int dw)
{
    int y;
    int x;
    for (y = 0; y < dw; y++) {
        int y0 = (y * sw) / dw;
        int y1 = ((y + 1) * sw) / dw;
        if (y1 <= y0) {
            y1 = y0 + 1;
        }
        for (x = 0; x < dw; x++) {
            int x0 = (x * sw) / dw;
            int x1 = ((x + 1) * sw) / dw;
            unsigned sr = 0;
            unsigned sg = 0;
            unsigned sb = 0;
            unsigned sa = 0;
            unsigned cnt = 0;
            int yy;
            int xx;
            uint8_t *o;
            uint16_t px;
            if (x1 <= x0) {
                x1 = x0 + 1;
            }
            for (yy = y0; yy < y1 && yy < sw; yy++) {
                for (xx = x0; xx < x1 && xx < sw; xx++) {
                    const uint8_t *p = src + ((yy * sw + xx) * 3);
                    uint16_t v = (uint16_t)(p[0] | (p[1] << 8));
                    sr += ((v >> 11) & 31u) * 255u / 31u;
                    sg += ((v >> 5) & 63u) * 255u / 63u;
                    sb += (v & 31u) * 255u / 31u;
                    sa += p[2];
                    cnt++;
                }
            }
            if (cnt == 0) {
                cnt = 1;
            }
            sr /= cnt;
            sg /= cnt;
            sb /= cnt;
            sa /= cnt;
            px = (uint16_t)(((sr >> 3) << 11) | ((sg >> 2) << 5) | (sb >> 3));
            o = dst + ((y * dw + x) * 3);
            o[0] = (uint8_t)(px & 0xFF);
            o[1] = (uint8_t)(px >> 8);
            o[2] = (uint8_t)sa;
        }
    }
}

const lv_img_dsc_t *vango_dash_motor_icon(bool bright)
{
    (void)bright;
    if (!s_gear_nav_ready) {
        uint8_t *big = heap_caps_malloc((size_t)MGEAR * MGEAR * 3, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
        s_gear_nav_mem = heap_caps_malloc((size_t)MGEAR_NAV * MGEAR_NAV * 3, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
        if (!s_gear_nav_mem) {
            heap_caps_free(big);
            return &vango_nav_motor;
        }
        if (big) {
            mgear_render_n(big, MGEAR, true);
            mgear_fit(big, MGEAR, s_gear_nav_mem, MGEAR_NAV);
            heap_caps_free(big);
        } else {
            mgear_render_n(s_gear_nav_mem, MGEAR_NAV, true);
        }
        s_gear_nav_dsc.header.always_zero = 0;
        s_gear_nav_dsc.header.w = MGEAR_NAV;
        s_gear_nav_dsc.header.h = MGEAR_NAV;
        s_gear_nav_dsc.header.cf = LV_IMG_CF_TRUE_COLOR_ALPHA;
        s_gear_nav_dsc.data_size = (uint32_t)(MGEAR_NAV * MGEAR_NAV * 3);
        s_gear_nav_dsc.data = s_gear_nav_mem;
        s_gear_nav_ready = true;
    }
    return &s_gear_nav_dsc;
}

static motor_ui_t s_mot[2];

#define MARROW 16
static LV_ATTRIBUTE_MEM_ALIGN uint8_t s_marrow_buf[2][MARROW * MARROW * 3];

static void marrow_px(uint8_t *buf, int x, int y, lv_color_t c)
{
    uint8_t *p;
    if (x < 0 || y < 0 || x >= MARROW || y >= MARROW) {
        return;
    }
    p = buf + ((y * MARROW + x) * 3);
    memcpy(p, &c, 2);
    p[2] = 255;
}

static int marrow_side(int x, int y, int x0, int y0, int x1, int y1)
{
    return (x - x0) * (y1 - y0) - (y - y0) * (x1 - x0);
}

static void marrow_render(uint8_t *buf, uint32_t accent)
{
    /* Right-pointing head, centroid at the canvas center so rotation stays on the ring. */
    const int ax = 5;
    const int ay = 3;
    const int bx = 5;
    const int by = 13;
    const int cx = 14;
    const int cy = 8;
    int y;
    int x;
    lv_color_t col = lv_color_hex(accent);
    memset(buf, 0, MARROW * MARROW * 3);
    for (y = 0; y < MARROW; y++) {
        for (x = 0; x < MARROW; x++) {
            int ab = marrow_side(x, y, ax, ay, bx, by);
            int bc = marrow_side(x, y, bx, by, cx, cy);
            int ca = marrow_side(x, y, cx, cy, ax, ay);
            if ((ab >= 0 && bc >= 0 && ca >= 0) || (ab <= 0 && bc <= 0 && ca <= 0)) {
                marrow_px(buf, x, y, col);
            }
        }
    }
}

static bool motor_peer_busy(const motor_ui_t *m)
{
    const motor_ui_t *other = (m == &s_mot[0]) ? &s_mot[1] : &s_mot[0];
    return other && other->held != 0;
}

static void motor_motion_cb(void *var, int32_t ang)
{
    motor_ui_t *m = var;
    lv_coord_t cx;
    lv_coord_t cy;
    int r;
    const int orbit = (int)ang * 115 / 100;
    const float rad = ((float)orbit / 10.0f) * 0.0174532925f;
    if (!m || !m->gear) {
        return;
    }
    lv_img_set_angle(m->gear, (int16_t)ang);
    if (!m->arrow || !m->arc) {
        return;
    }
    /* Ring stroke is drawn inside the arc box. Midline is radius minus half the width. */
    r = lv_obj_get_width(m->arc) / 2 - 2;
    cx = lv_obj_get_x(m->arc) + lv_obj_get_width(m->arc) / 2;
    cy = lv_obj_get_y(m->arc) + lv_obj_get_height(m->arc) / 2;
    lv_obj_set_pos(m->arrow,
                   cx + (lv_coord_t)(r * cosf(rad)) - MARROW / 2,
                   cy + (lv_coord_t)(r * sinf(rad)) - MARROW / 2);
    lv_img_set_angle(m->arrow, (int16_t)(orbit + (m->spin_dir == 2 ? -900 : 900)));
}

static bool motor_card_live(const motor_ui_t *m)
{
    if (!m || !m->up) {
        return false;
    }
    if (s_demo) {
        return true;
    }
    return m->drive && s_motor_link;
}

static void motor_spin_one(motor_ui_t *m, int dir)
{
    int start;
    if (!m || !m->gear) {
        if (m) {
            m->spin_dir = 0;
        }
        return;
    }
    if (dir == m->spin_dir) {
        return;
    }
    m->spin_dir = dir;
    lv_anim_del(m, motor_motion_cb);
    if (m->arrow) {
        if (dir == 0) {
            lv_obj_add_flag(m->arrow, LV_OBJ_FLAG_HIDDEN);
        } else {
            lv_obj_clear_flag(m->arrow, LV_OBJ_FLAG_HIDDEN);
        }
    }
    if (dir == 0) {
        return;
    }
    start = (int)lv_img_get_angle(m->gear);
    lv_anim_t a;
    lv_anim_init(&a);
    lv_anim_set_var(&a, m);
    lv_anim_set_exec_cb(&a, motor_motion_cb);
    /* dir 1 clockwise (Yukarı/Aç). dir 2 counter-clockwise (Aşağı/Kapat).
     * Tablet default speedPct 70: 35+70*2.4 = 203 deg/s. Command angle, not a position. */
    lv_anim_set_values(&a, start, dir == 1 ? start + 3600 : start - 3600);
    lv_anim_set_time(&a, 1770);
    lv_anim_set_repeat_count(&a, LV_ANIM_REPEAT_INFINITE);
    lv_anim_start(&a);
}

/** CAN 0x750: 1 ileri, 2 geri, 0 dur. Bırakınca mutlaka 0 bas (periyodik timer 0 göndermez). */
static void motor_can_send(int cmd)
{
    uint8_t data[8] = {0};
    data[0] = (uint8_t)cmd;
    motorData = cmd;
    send_can_frame(0x750, data);
}

static void motor_halt(void)
{
    s_mot[0].held = 0;
    s_mot[1].held = 0;
    motor_can_send(0);
    motor_spin_one(&s_mot[0], 0);
    motor_spin_one(&s_mot[1], 0);
}

static void motor_set_btn(lv_obj_t *btn, bool enable, bool active, uint32_t accent)
{
    lv_obj_t *lbl;
    if (!btn) {
        return;
    }
    lbl = lv_obj_get_child(btn, 0);
    if (enable) {
        lv_obj_add_flag(btn, LV_OBJ_FLAG_CLICKABLE);
        lv_obj_set_style_bg_opa(btn, LV_OPA_COVER, 0);
        lv_obj_set_style_bg_color(btn, lv_color_hex(active ? accent : 0x0A1C2Au), 0);
        lv_obj_set_style_border_color(btn, lv_color_hex(active ? accent : 0x8BA9BAu), 0);
        lv_obj_set_style_border_opa(btn, LV_OPA_COVER, 0);
        if (lbl) {
            lv_obj_set_style_text_color(lbl, lv_color_hex(0xFFFFFFu), 0);
        }
    } else {
        lv_obj_clear_flag(btn, LV_OBJ_FLAG_CLICKABLE);
        lv_obj_clear_state(btn, LV_STATE_PRESSED);
        lv_obj_set_style_bg_opa(btn, LV_OPA_COVER, 0);
        lv_obj_set_style_bg_color(btn, lv_color_hex(0x0A1C2Au), 0);
        lv_obj_set_style_border_color(btn, lv_color_hex(0x3A4A58u), 0);
        if (lbl) {
            lv_obj_set_style_text_color(lbl, lv_color_hex(0x8BA9BAu), 0);
        }
    }
}

static void mlock_px(uint8_t *buf, int x, int y, lv_color_t c, uint8_t a)
{
    uint8_t *p;
    if (x < 0 || y < 0 || x >= MLOCK || y >= MLOCK) {
        return;
    }
    p = buf + ((y * MLOCK + x) * 3);
    memcpy(p, &c, 2);
    p[2] = a;
}

static void mlock_fill(uint8_t *buf, int x0, int y0, int x1, int y1, lv_color_t c)
{
    int y;
    int x;
    for (y = y0; y <= y1; y++) {
        for (x = x0; x <= x1; x++) {
            mlock_px(buf, x, y, c, 255);
        }
    }
}

/** Closed/open padlock for the under-gear lock button. */
static void mlock_render(uint8_t *buf, bool locked, uint32_t accent)
{
    const lv_color_t col = lv_color_hex(accent);
    const int cx = MLOCK / 2;
    const int s = MLOCK;
    int x;
    int y;
    memset(buf, 0, MLOCK * MLOCK * 3);
    /* Body */
    mlock_fill(buf, (8 * s) / 32, (16 * s) / 32, (23 * s) / 32, (28 * s) / 32, col);
    /* Keyhole */
    for (y = (19 * s) / 32; y <= (22 * s) / 32; y++) {
        for (x = cx - 2; x <= cx + 2; x++) {
            mlock_px(buf, x, y, lv_color_hex(0x0A1C2Au), 255);
        }
    }
    mlock_fill(buf, cx - 2, (22 * s) / 32, cx + 2, (25 * s) / 32, lv_color_hex(0x0A1C2Au));
    /* Shackle */
    for (y = (6 * s) / 32; y <= (16 * s) / 32; y++) {
        for (x = 0; x < MLOCK; x++) {
            int dx = x - cx;
            int dy = y - (12 * s) / 32;
            int r_out = (9 * s) / 32;
            int r_in = (6 * s) / 32;
            int d2 = dx * dx + dy * dy;
            if (d2 <= r_out * r_out && d2 >= r_in * r_in) {
                if (locked || x <= cx + 1) {
                    mlock_px(buf, x, y, col, 255);
                }
            }
        }
    }
    mlock_fill(buf, (7 * s) / 32, (12 * s) / 32, (10 * s) / 32, (16 * s) / 32, col);
    if (locked) {
        mlock_fill(buf, (21 * s) / 32, (12 * s) / 32, (24 * s) / 32, (16 * s) / 32, col);
    } else {
        mlock_fill(buf, (21 * s) / 32, (7 * s) / 32, (24 * s) / 32, (12 * s) / 32, col);
    }
}

static void motor_paint_one(motor_ui_t *m)
{
    const bool peer = motor_peer_busy(m);
    const bool linked = motor_card_live(m);
    const bool armed = m->unlocked;
    const bool padlock = m->locked;
    const bool live = linked && !peer && armed && !padlock;
    const bool run = live && m->held != 0;
    const bool gear_on = linked && !peer && armed;
    const char *txt = "--";
    const char *pill = "--";
    uint32_t col = 0x8BA9BAu;
    int slot = (int)(m - s_mot);
    motor_set_btn(m->up, live && m->held != 2, live && m->held == 1, m->accent);
    motor_set_btn(m->down, live && m->held != 1, live && m->held == 2, m->accent);
    if (!linked && !m->drive) {
        txt = vango_tx("No drive", "Sürücü yok", "Kein Antrieb");
        pill = vango_tx("Off", "Yok", "Aus");
    } else if (!linked) {
        txt = vango_tx("No link", "Bağlantı yok", "Keine Verbindung");
        pill = vango_tx("Off", "Yok", "Aus");
    } else if (padlock) {
        txt = vango_tx("Locked", "Kilitli", "Gesperrt");
        pill = vango_tx("Locked", "Kilitli", "Gesperrt");
    } else if (!armed) {
        txt = vango_tx("Passive", "Pasif", "Passiv");
        pill = txt;
    } else if (peer) {
        txt = vango_tx("Other motor running", "Diğer motor çalışıyor", "Anderer Motor laeuft");
        pill = vango_tx("Idle", "Pasif", "Leerlauf");
    } else if (m->held == 1) {
        txt = m->drive ? vango_tx("Moving up", "Yukarı Çalışıyor", "Faehrt hoch")
                       : vango_tx("Opening", "Aç Çalışıyor", "Oeffnet");
        pill = m->drive ? vango_tx("Up", "Yukarı", "Hoch") : vango_tx("Open", "Aç", "Auf");
        col = m->accent;
    } else if (m->held == 2) {
        txt = m->drive ? vango_tx("Moving down", "Aşağı Çalışıyor", "Faehrt runter")
                       : vango_tx("Closing", "Kapat Çalışıyor", "Schliesst");
        pill = m->drive ? vango_tx("Down", "Aşağı", "Runter") : vango_tx("Close", "Kapat", "Zu");
        col = m->accent;
    } else {
        txt = vango_tx("Ready", "Hazır", "Bereit");
        pill = txt;
        col = 0x65D95Bu;
    }
    if (m->status) {
        lv_label_set_text(m->status, txt);
        lv_obj_set_style_text_color(m->status, lv_color_hex(col), 0);
    }
    if (m->pill) {
        lv_label_set_text(m->pill, pill);
        lv_obj_set_style_text_color(m->pill, lv_color_hex(col), 0);
    }
    if (m->pill_bg) {
        lv_obj_set_style_bg_color(m->pill_bg, lv_color_hex(col), 0);
    }
    if (m->dot) {
        lv_obj_set_style_bg_color(m->dot, lv_color_hex(col), 0);
    }
    if (m->left_lbl) {
        lv_obj_set_style_text_color(m->left_lbl,
                                    lv_color_hex(live && m->held == 1 ? m->accent : (live ? 0xFFFFFFu : 0x8BA9BAu)), 0);
    }
    if (m->right_lbl) {
        lv_obj_set_style_text_color(m->right_lbl,
                                    lv_color_hex(live && m->held == 2 ? m->accent : (live ? 0xFFFFFFu : 0x8BA9BAu)), 0);
    }
    if (m->arc) {
        if (!armed) {
            lv_obj_set_style_arc_color(m->arc, lv_color_hex(0x3A4A58u), LV_PART_MAIN);
            lv_obj_set_style_arc_opa(m->arc, LV_OPA_40, LV_PART_MAIN);
        } else {
            lv_obj_set_style_arc_color(m->arc, lv_color_hex(m->accent), LV_PART_MAIN);
            lv_obj_set_style_arc_opa(m->arc, run ? LV_OPA_80 : LV_OPA_70, LV_PART_MAIN);
        }
    }
    if (m->gear && m->gear_live != (int)gear_on) {
        m->gear_live = (int)gear_on;
        if (slot >= 0 && slot < 2) {
            mgear_render(s_mgear_buf[slot], gear_on);
            lv_obj_invalidate(m->gear);
        }
    }
    if (m->lock_btn) {
        lv_obj_set_style_border_color(m->lock_btn, lv_color_hex(m->accent), 0);
    }
    if (m->lock_ico && slot >= 0 && slot < 2) {
        mlock_render(s_mlock_buf[slot], padlock, m->accent);
        lv_obj_invalidate(m->lock_ico);
    }
    if (m->lock_lbl) {
        lv_label_set_text(m->lock_lbl,
                          padlock ? vango_tx("LOCKED", "KİLİTLİ", "GESPERRT") : vango_tx("UNLOCKED", "KİLİT AÇIK", "ENTSPERRT"));
        lv_obj_set_style_text_color(m->lock_lbl, lv_color_hex(m->accent), 0);
    }
    motor_spin_one(m, run ? m->held : 0);
}

static void motor_paint(void)
{
    motor_paint_one(&s_mot[0]);
    motor_paint_one(&s_mot[1]);
}

static void motor_dir_cb(lv_event_t *e)
{
    const intptr_t pack = (intptr_t)lv_event_get_user_data(e);
    const int slot = (int)((pack >> 8) & 1);
    const int dir = (int)(pack & 0xFF);
    motor_ui_t *m = &s_mot[slot];
    const lv_event_code_t code = lv_event_get_code(e);
    if (code == LV_EVENT_PRESSED) {
        if (!m->unlocked || m->locked || !motor_card_live(m) || motor_peer_busy(m)) {
            lv_obj_clear_state(lv_event_get_target(e), LV_STATE_PRESSED);
            return;
        }
        if (m->held != 0 && m->held != dir) {
            lv_obj_clear_state(lv_event_get_target(e), LV_STATE_PRESSED);
            return;
        }
        m->held = dir;
        if (m->drive && s_motor_link) {
            motor_can_send(dir);
        }
        motor_paint();
        return;
    }
    if (code == LV_EVENT_RELEASED || code == LV_EVENT_PRESS_LOST) {
        if (m->held == dir) {
            m->held = 0;
            if (m->drive) {
                motor_can_send(0);
            }
            motor_paint();
        }
    }
}

static void motor_apply_armed(int slot, bool armed)
{
    motor_ui_t *m;
    if (slot < 0 || slot > 1) {
        return;
    }
    s_motor_armed[slot] = armed;
    m = &s_mot[slot];
    m->unlocked = armed;
    if (!m->up) {
        return;
    }
    if (!armed && m->held != 0) {
        m->held = 0;
        if (m->drive) {
            motor_can_send(0);
        }
    }
    motor_paint();
}

static void motor_apply_locked(int slot, bool locked)
{
    motor_ui_t *m;
    if (slot < 0 || slot > 1) {
        return;
    }
    m = &s_mot[slot];
    if (m->held != 0) {
        return;
    }
    s_motor_locked[slot] = locked;
    m->locked = locked;
    if (!m->up) {
        return;
    }
    motor_paint();
}

void vango_dash_motor_set_armed(int slot, bool armed)
{
    motor_ui_t *m;
    if (slot < 0 || slot > 1) {
        return;
    }
    m = &s_mot[slot];
    if (m->sw) {
        if (armed) {
            lv_obj_add_state(m->sw, LV_STATE_CHECKED);
        } else {
            lv_obj_clear_state(m->sw, LV_STATE_CHECKED);
        }
    }
    motor_apply_armed(slot, armed);
}

void vango_dash_motor_set_locked(int slot, bool locked)
{
    motor_apply_locked(slot, locked);
}

static void motor_pad_cb(lv_event_t *e)
{
    const int slot = (int)(intptr_t)lv_event_get_user_data(e);
    motor_ui_t *m;
    if (slot < 0 || slot > 1) {
        return;
    }
    m = &s_mot[slot];
    if (m->held != 0) {
        return;
    }
    motor_apply_locked(slot, !m->locked);
}

static lv_obj_t *motor_round_btn(lv_obj_t *parent, const char *symbol, int slot, int dir)
{
    lv_obj_t *btn = lv_btn_create(parent);
    const intptr_t pack = ((intptr_t)slot << 8) | (dir & 0xFF);
    lv_obj_set_size(btn, 64, 64);
    lv_obj_set_style_radius(btn, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_color(btn, lv_color_hex(0x0A1C2Au), 0);
    lv_obj_set_style_shadow_width(btn, 0, 0);
    lv_obj_set_style_border_width(btn, 2, 0);
    lv_obj_set_style_border_color(btn, lv_color_hex(0x8BA9BAu), 0);
    lv_obj_t *lbl = make_label(btn, symbol, &lv_font_montserrat_24, 0xFFFFFFu);
    lv_obj_center(lbl);
    lv_obj_add_event_cb(btn, motor_dir_cb, LV_EVENT_PRESSED, (void *)pack);
    lv_obj_add_event_cb(btn, motor_dir_cb, LV_EVENT_RELEASED, (void *)pack);
    lv_obj_add_event_cb(btn, motor_dir_cb, LV_EVENT_PRESS_LOST, (void *)pack);
    return btn;
}

static void add_motor_card(lv_obj_t *parent, int x, int y, int w, int h, int slot)
{
    motor_ui_t *m = &s_mot[slot];
    const bool drive = (slot == 0);
    const int head = 56;
    const int dial = 118;
    const int btn = 64;
    const int lock_sz = 48;
    const int lock_lbl_h = 18;
    /* Kilit + etiketi icin daha fazla alt bosluk */
    const int lock_y = h - 36 - lock_lbl_h - lock_sz;
    const int foot = h - lock_y + 4;
    const int cx = w / 2;
    /* Dial biraz yukari — yan buton/etiket icin yer */
    const int cy = head + (h - head - foot) / 2 - 10;
    lv_obj_t *card;
    lv_obj_t *ico;
    lv_obj_t *nm;
    lv_obj_t *dot;
    lv_obj_t *st;
    lv_obj_t *pill;
    lv_obj_t *pl;
    lv_obj_t *rule;
    lv_obj_t *arc;
    lv_obj_t *gear;
    lv_obj_t *arrow;
    lv_obj_t *ll;
    lv_obj_t *rl;
    lv_obj_t *up;
    lv_obj_t *dn;
    lv_obj_t *lock_btn;
    lv_obj_t *lock_ico;
    lv_obj_t *lock_lbl;
    const char *left = drive ? vango_tx("Up", "Yukarı", "Hoch") : vango_tx("Open", "Aç", "Auf");
    const char *right = drive ? vango_tx("Down", "Aşağı", "Runter") : vango_tx("Close", "Kapat", "Zu");

    memset(m, 0, sizeof(*m));
    m->drive = drive;
    /* M1 follows theme accent; M2 stays purple for pair contrast. */
    m->accent = (slot == 0) ? COL_ACCENT : 0xB794F6u;
    m->gear_live = -1;
    m->unlocked = true;
    m->locked = s_motor_locked[slot];
    m->sw = NULL;

    card = lv_obj_create(parent);
    lv_obj_set_size(card, w, h);
    lv_obj_set_pos(card, x, y);
    style_card(card);
    lv_obj_set_style_bg_color(card, lv_color_hex(COL_ACCENT_CARD), 0);
    lv_obj_set_style_radius(card, 16, 0);
    lv_obj_set_style_border_width(card, 1, 0);
    lv_obj_set_style_border_color(card, lv_color_hex(m->accent), 0);

    ico = lv_img_create(card);
    lv_img_set_src(ico, vango_dash_motor_icon(true));
    lv_obj_set_pos(ico, 6, 2);
    lv_obj_clear_flag(ico, LV_OBJ_FLAG_CLICKABLE);

    nm = make_label(card, slot == 0 ? "MOTOR 1" : "MOTOR 2", &vango_font_20, 0xFFFFFFu);
    lv_obj_set_pos(nm, 52, 8);

    dot = lv_obj_create(card);
    lv_obj_set_size(dot, 8, 8);
    lv_obj_set_pos(dot, 52, 36);
    lv_obj_set_style_radius(dot, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_color(dot, lv_color_hex(0x8BA9BAu), 0);
    lv_obj_set_style_border_width(dot, 0, 0);
    lv_obj_clear_flag(dot, LV_OBJ_FLAG_CLICKABLE);

    st = make_label(card, "--", &vango_font_14, 0xB8C6D2u);
    lv_obj_set_pos(st, 66, 32);

    pill = lv_obj_create(card);
    lv_obj_set_size(pill, 108, 28);
    lv_obj_align(pill, LV_ALIGN_TOP_RIGHT, -12, 12);
    lv_obj_set_style_radius(pill, 14, 0);
    lv_obj_set_style_bg_color(pill, lv_color_hex(0x8BA9BAu), 0);
    lv_obj_set_style_bg_opa(pill, LV_OPA_30, 0);
    lv_obj_set_style_border_width(pill, 0, 0);
    lv_obj_set_style_pad_all(pill, 0, 0);
    lv_obj_clear_flag(pill, LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_CLICKABLE);
    pl = make_label(pill, "--", &vango_font_14, 0xB8C6D2u);
    lv_obj_center(pl);

    rule = lv_obj_create(card);
    lv_obj_set_size(rule, w - 28, 1);
    lv_obj_set_pos(rule, 14, head - 2);
    lv_obj_set_style_bg_color(rule, lv_color_hex(m->accent), 0);
    lv_obj_set_style_bg_opa(rule, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(rule, 0, 0);
    lv_obj_set_style_radius(rule, 0, 0);
    lv_obj_clear_flag(rule, LV_OBJ_FLAG_CLICKABLE);

    arc = lv_arc_create(card);
    lv_obj_set_size(arc, dial, dial);
    lv_obj_set_pos(arc, cx - dial / 2, cy - dial / 2);
    lv_arc_set_bg_angles(arc, 0, 359);
    lv_arc_set_angles(arc, 0, 0);
    lv_obj_clear_flag(arc, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_style_arc_width(arc, 4, LV_PART_MAIN);
    lv_obj_set_style_arc_color(arc, lv_color_hex(m->accent), LV_PART_MAIN);
    lv_obj_set_style_arc_opa(arc, LV_OPA_70, LV_PART_MAIN);
    lv_obj_set_style_arc_opa(arc, LV_OPA_TRANSP, LV_PART_INDICATOR);
    lv_obj_set_style_bg_opa(arc, LV_OPA_TRANSP, 0);
    lv_obj_set_style_bg_opa(arc, LV_OPA_TRANSP, LV_PART_KNOB);
    lv_obj_set_style_pad_all(arc, 0, LV_PART_KNOB);
    lv_obj_set_style_border_width(arc, 0, 0);

    gear = lv_canvas_create(card);
    lv_canvas_set_buffer(gear, s_mgear_buf[slot], MGEAR, MGEAR, LV_IMG_CF_TRUE_COLOR_ALPHA);
    lv_obj_set_pos(gear, cx - MGEAR / 2, cy - MGEAR / 2);
    lv_img_set_pivot(gear, MGEAR / 2, MGEAR / 2);
    lv_obj_clear_flag(gear, LV_OBJ_FLAG_CLICKABLE);
    mgear_render(s_mgear_buf[slot], false);

    arrow = lv_canvas_create(card);
    lv_canvas_set_buffer(arrow, s_marrow_buf[slot], MARROW, MARROW, LV_IMG_CF_TRUE_COLOR_ALPHA);
    marrow_render(s_marrow_buf[slot], m->accent);
    lv_img_set_pivot(arrow, MARROW / 2, MARROW / 2);
    lv_obj_add_flag(arrow, LV_OBJ_FLAG_HIDDEN);
    lv_obj_clear_flag(arrow, LV_OBJ_FLAG_CLICKABLE);

    ll = make_label(card, left, &vango_font_16, 0xFFFFFFu);
    rl = make_label(card, right, &vango_font_16, 0xFFFFFFu);
    {
        /* Sabit Y: etiket ile yuvarlak buton arasinda ~10px net bosluk */
        const int btn_y = cy - btn / 2 + 34;
        const int lbl_y = btn_y - 30;
        up = motor_round_btn(card, LV_SYMBOL_UP, slot, 1);
        lv_obj_set_pos(up, 16, btn_y);
        dn = motor_round_btn(card, LV_SYMBOL_DOWN, slot, 2);
        lv_obj_set_pos(dn, w - 16 - btn, btn_y);
        lv_obj_update_layout(ll);
        lv_obj_update_layout(rl);
        lv_obj_set_pos(ll, 16 + (btn - (int)lv_obj_get_width(ll)) / 2, lbl_y);
        lv_obj_set_pos(rl, w - 16 - btn + (btn - (int)lv_obj_get_width(rl)) / 2, lbl_y);
    }

    lock_btn = lv_btn_create(card);
    lv_obj_set_size(lock_btn, lock_sz, lock_sz);
    lv_obj_set_pos(lock_btn, cx - lock_sz / 2, lock_y);
    lv_obj_set_style_radius(lock_btn, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_color(lock_btn, lv_color_hex(0x0A1C2Au), 0);
    lv_obj_set_style_bg_opa(lock_btn, LV_OPA_COVER, 0);
    lv_obj_set_style_shadow_width(lock_btn, 0, 0);
    lv_obj_set_style_border_width(lock_btn, 2, 0);
    lv_obj_set_style_border_color(lock_btn, lv_color_hex(m->accent), 0);
    lv_obj_set_style_pad_all(lock_btn, 0, 0);
    lv_obj_add_event_cb(lock_btn, motor_pad_cb, LV_EVENT_CLICKED, (void *)(intptr_t)slot);

    lock_ico = lv_canvas_create(lock_btn);
    lv_canvas_set_buffer(lock_ico, s_mlock_buf[slot], MLOCK, MLOCK, LV_IMG_CF_TRUE_COLOR_ALPHA);
    mlock_render(s_mlock_buf[slot], m->locked, m->accent);
    lv_obj_center(lock_ico);
    lv_obj_clear_flag(lock_ico, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_flag(lock_ico, LV_OBJ_FLAG_EVENT_BUBBLE);

    lock_lbl = make_label(card,
                          m->locked ? vango_tx("LOCKED", "KİLİTLİ", "GESPERRT") : vango_tx("UNLOCKED", "KİLİT AÇIK", "ENTSPERRT"),
                          &vango_font_14, m->accent);
    lv_obj_set_width(lock_lbl, w - 24);
    lv_obj_set_style_text_align(lock_lbl, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_pos(lock_lbl, 12, lock_y + lock_sz + 6);

    lv_obj_move_foreground(arrow);

    m->gear = gear;
    m->arc = arc;
    m->status = st;
    m->pill = pl;
    m->pill_bg = pill;
    m->dot = dot;
    m->up = up;
    m->down = dn;
    m->left_lbl = ll;
    m->right_lbl = rl;
    m->arrow = arrow;
    m->lock_btn = lock_btn;
    m->lock_ico = lock_ico;
    m->lock_lbl = lock_lbl;
}

static void build_motors_page(lv_obj_t *parent)
{
    memset(s_mot, 0, sizeof(s_mot));
    lv_obj_set_style_bg_color(parent, lv_color_hex(COL_BG), 0);
    lv_obj_set_style_bg_opa(parent, LV_OPA_COVER, 0);
    const hexnet_io_profile_t *p = hexnet_io_profile_get();
    int slots[2];
    int n = 0;
    const int head_h = 8;
    if (p && (p->motor_enabled & 0x01u)) {
        slots[n++] = 0;
    }
    if (p && (p->motor_enabled & 0x02u)) {
        slots[n++] = 1;
    }
    if (n == 0) {
        lv_obj_t *msg = make_label(parent,
                                   vango_tx("No active motor", "Aktif motor bulunmuyor", "Kein aktiver Motor"),
                                   &vango_font_18, COL_MUTED);
        lv_obj_align(msg, LV_ALIGN_CENTER, 0, head_h / 2);
        return;
    }
    const int margin = 16;
    const int gap = 16;
    const int card_y = 8;
    const int card_h = DASH_TILE_H - card_y - 8;
    if (n == 1) {
        const int card_w = 460;
        add_motor_card(parent, (DASH_W - card_w) / 2, card_y, card_w, card_h, slots[0]);
    } else {
        const int card_w = (DASH_W - 2 * margin - gap) / 2;
        add_motor_card(parent, margin, card_y, card_w, card_h, slots[0]);
        add_motor_card(parent, margin + card_w + gap, card_y, card_w, card_h, slots[1]);
    }
    motor_paint();
}

typedef struct {
    lv_obj_t *temp;
    lv_obj_t *hum;
    lv_obj_t *status;
    lv_obj_t *warn;
    lv_obj_t *dot;
    int slot;
    bool has_channel;
} sens_card_t;

static sens_card_t s_sens[2];
static int s_sens_n;
static int s_th_t = -1000;
static int s_th_h = -1;
static bool s_th_link;

/* Tablet TempHumidityCard: 40×96 tuval, cıva (t/45), 1.8 sn parlama. */
#define THERMO_W 40
#define THERMO_H 96
static LV_ATTRIBUTE_MEM_ALIGN uint8_t s_thermo_buf[2][THERMO_W * THERMO_H * 3];
static lv_obj_t *s_thermo_cv[2];
static int s_thermo_now[2] = { 80, 80 };
static bool s_thermo_ok[2];
static int s_thermo_c[2];
static uint8_t s_thermo_glow;
static int s_thermo_glow_dir = 1;

/* Tablet LiveHumidityDropIcon: 64×96 damla, doluluk nem/100, AA kenar. */
#define DROP_W 64
#define DROP_H 96
static LV_ATTRIBUTE_MEM_ALIGN uint8_t s_drop_buf[2][DROP_W * DROP_H * 3];
static lv_obj_t *s_drop_cv[2];
static int s_drop_now[2] = { 120, 120 };
static bool s_drop_ok[2];
static int s_drop_h[2];
static uint8_t s_drop_glow;
static int s_drop_glow_dir = 1;
static void drop_draw(lv_obj_t *cv, int mq, bool ok, uint8_t glow);
static void drop_anim_cb(void *var, int32_t v);

static uint32_t thermo_tint(bool ok, int temp_c)
{
    if (!ok) {
        return 0x8B9BB2u;
    }
    if (temp_c <= 5) {
        return 0x60A5FAu;
    }
    if (temp_c <= 15) {
        return 0x38BDF8u;
    }
    if (temp_c <= 22) {
        return 0x2EC5FFu;
    }
    if (temp_c <= 28) {
        return 0x4ADE80u;
    }
    if (temp_c <= 34) {
        return 0xFBBF24u;
    }
    return 0xF87171u;
}

static int thermo_target(bool ok, int temp_c)
{
    int m;
    if (!ok) {
        return 80;
    }
    if (temp_c < 0) {
        temp_c = 0;
    }
    if (temp_c > 45) {
        temp_c = 45;
    }
    m = (temp_c * 1000) / 45;
    if (m < 120) {
        m = 120;
    }
    if (m > 1000) {
        m = 1000;
    }
    return m;
}

static void thermo_draw(lv_obj_t *cv, int mq, bool ok, int temp_c, uint8_t glow)
{
    const int w = THERMO_W;
    const int h = THERMO_H;
    const int tube_w = (w * 22) / 100;
    const int bulb_r = (w * 30) / 100;
    const int cx = w / 2;
    const int tube_left = cx - tube_w / 2;
    const int tube_top = (h * 6) / 100;
    const int bulb_cy = h - bulb_r - (h * 4) / 100;
    const int tube_bottom = bulb_cy - (bulb_r * 42) / 100;
    int stroke = (w * 85) / 1000;
    int span;
    int fill_h;
    int fill_top;
    int inner_x;
    int inner_w;
    int stroke_opa;
    uint32_t tint;
    lv_draw_rect_dsc_t rect;
    lv_draw_line_dsc_t line;
    int i;
    if (!cv) {
        return;
    }
    if (stroke < 2) {
        stroke = 2;
    }
    if (stroke > 3) {
        stroke = 3;
    }
    if (mq < 0) {
        mq = 0;
    }
    if (mq > 1000) {
        mq = 1000;
    }
    tint = thermo_tint(ok, temp_c);
    stroke_opa = 236 + ((19 * (int)glow) / 255);
    span = tube_bottom - tube_top;
    if (span < 1) {
        span = 1;
    }
    fill_h = (span * mq) / 1000;
    fill_top = tube_bottom - fill_h;
    inner_x = tube_left + (stroke * 6) / 10;
    inner_w = tube_w - (stroke * 12) / 10;
    if (inner_w < 2) {
        inner_w = 2;
    }

    lv_canvas_fill_bg(cv, lv_color_hex(0), LV_OPA_TRANSP);

    lv_draw_rect_dsc_init(&rect);
    rect.radius = inner_w;
    rect.bg_color = lv_color_hex(tint);
    rect.bg_opa = 217;
    rect.border_width = 0;
    if (fill_h > 0) {
        lv_canvas_draw_rect(cv, inner_x, fill_top, inner_w, fill_h + 2, &rect);
    }
    rect.bg_opa = 230;
    rect.radius = LV_RADIUS_CIRCLE;
    lv_canvas_draw_rect(cv, cx - (bulb_r - stroke), bulb_cy - (bulb_r - stroke),
                        (bulb_r - stroke) * 2, (bulb_r - stroke) * 2, &rect);

    lv_draw_rect_dsc_init(&rect);
    rect.bg_opa = LV_OPA_TRANSP;
    rect.border_width = stroke;
    rect.border_color = lv_color_hex(tint);
    rect.border_opa = (lv_opa_t)stroke_opa;
    rect.radius = tube_w;
    lv_canvas_draw_rect(cv, tube_left, tube_top, tube_w,
                        tube_bottom - tube_top + (bulb_r * 35) / 100, &rect);
    rect.radius = LV_RADIUS_CIRCLE;
    lv_canvas_draw_rect(cv, cx - bulb_r, bulb_cy - bulb_r, bulb_r * 2, bulb_r * 2, &rect);

    lv_draw_line_dsc_init(&line);
    line.color = lv_color_hex(tint);
    line.opa = (lv_opa_t)((stroke_opa * 55) / 100);
    line.width = 1;
    line.round_start = 1;
    line.round_end = 1;
    for (i = 1; i <= 4; i++) {
        int ty = tube_top + ((tube_bottom - tube_top) * i) / 5;
        lv_point_t pts[2] = {
            { (lv_coord_t)(tube_left + tube_w + 2), (lv_coord_t)ty },
            { (lv_coord_t)(tube_left + tube_w + (w * 18) / 100), (lv_coord_t)ty },
        };
        lv_canvas_draw_line(cv, pts, 2, &line);
    }
}

static void thermo_anim_cb(void *var, int32_t v)
{
    int idx = (int)(intptr_t)var;
    if (idx < 0 || idx > 1 || !s_thermo_cv[idx]) {
        return;
    }
    s_thermo_now[idx] = (int)v;
    thermo_draw(s_thermo_cv[idx], s_thermo_now[idx], s_thermo_ok[idx], s_thermo_c[idx], s_thermo_glow);
}

static void thermo_apply(int idx, bool ok, int temp_c)
{
    int target;
    lv_anim_t a;
    if (idx < 0 || idx > 1 || !s_thermo_cv[idx]) {
        return;
    }
    s_thermo_ok[idx] = ok;
    s_thermo_c[idx] = temp_c;
    target = thermo_target(ok, temp_c);
    lv_anim_del((void *)(intptr_t)idx, thermo_anim_cb);
    if (s_thermo_now[idx] == target) {
        thermo_draw(s_thermo_cv[idx], target, ok, temp_c, s_thermo_glow);
        return;
    }
    lv_anim_init(&a);
    lv_anim_set_var(&a, (void *)(intptr_t)idx);
    lv_anim_set_exec_cb(&a, thermo_anim_cb);
    lv_anim_set_values(&a, s_thermo_now[idx], target);
    lv_anim_set_time(&a, 450);
    lv_anim_set_path_cb(&a, lv_anim_path_ease_out);
    lv_anim_start(&a);
}

static void thermo_glow_tick(lv_timer_t *timer)
{
    int i;
    int g;
    (void)timer;
    g = (int)s_thermo_glow + s_thermo_glow_dir * 14;
    if (g >= 255) {
        g = 255;
        s_thermo_glow_dir = -1;
    } else if (g <= 0) {
        g = 0;
        s_thermo_glow_dir = 1;
    }
    s_thermo_glow = (uint8_t)g;
    g = (int)s_drop_glow + s_drop_glow_dir * 16;
    if (g >= 255) {
        g = 255;
        s_drop_glow_dir = -1;
    } else if (g <= 0) {
        g = 0;
        s_drop_glow_dir = 1;
    }
    s_drop_glow = (uint8_t)g;
    for (i = 0; i < 2; i++) {
        if (s_thermo_cv[i]) {
            thermo_draw(s_thermo_cv[i], s_thermo_now[i], s_thermo_ok[i], s_thermo_c[i], s_thermo_glow);
        }
        if (s_drop_cv[i]) {
            drop_draw(s_drop_cv[i], s_drop_now[i], s_drop_ok[i], s_drop_glow);
        }
    }
}

static void thermo_release(void)
{
    int i;
    for (i = 0; i < 2; i++) {
        lv_anim_del((void *)(intptr_t)i, thermo_anim_cb);
        lv_anim_del((void *)(intptr_t)i, drop_anim_cb);
        s_thermo_cv[i] = NULL;
        s_drop_cv[i] = NULL;
    }
}

static void drop_px(uint8_t *buf, int x, int y, lv_color_t c, uint8_t a)
{
    uint8_t *p;
    if (x < 0 || y < 0 || x >= DROP_W || y >= DROP_H || a == 0) {
        return;
    }
    p = buf + ((y * DROP_W + x) * 3);
    if (p[2] == 0) {
        memcpy(p, &c, 2);
        p[2] = a;
        return;
    }
    /* Simple over for soft edges / stroke. */
    {
        lv_color_t dst;
        memcpy(&dst, p, 2);
        dst = lv_color_mix(c, dst, a);
        memcpy(p, &dst, 2);
        if (a > p[2]) {
            p[2] = a;
        }
    }
}

static int drop_target(bool ok, int hum)
{
    int m;
    if (!ok) {
        return 120;
    }
    if (hum < 0) {
        hum = 0;
    }
    if (hum > 100) {
        hum = 100;
    }
    m = hum * 10;
    if (m < 120) {
        m = 120;
    }
    if (m > 1000) {
        m = 1000;
    }
    return m;
}

static void drop_draw(lv_obj_t *cv, int mq, bool ok, uint8_t glow)
{
    /* Tip lines + circle border — same LVGL AA path as the thermometer. */
    const int w = DROP_W;
    const int h = DROP_H;
    const int cx = w / 2;
    const int tip_y = (h * 3) / 100;
    const int bulb_r = (w * 34) / 100;
    const int bulb_cy = h - bulb_r - (h * 4) / 100;
    /* Join circle at ~225° / 315° (LVGL: 0°=east, clockwise). */
    const int join_dx = (bulb_r * 71) / 100; /* ~cos/sin 45° */
    const int join_dy = (bulb_r * 71) / 100;
    const int lx = cx - join_dx;
    const int ly = bulb_cy - join_dy;
    const int rx = cx + join_dx;
    const int ry = bulb_cy - join_dy;
    const int tip_join = ly;
    int stroke = 3;
    int fill_top;
    int stroke_opa;
    int hl_opa;
    int hx;
    int hy;
    int hr;
    int y;
    int dx;
    int dy;
    int dist2;
    int lim2;
    uint32_t tint_hex = ok ? COL_ACCENT : 0x8B9BB2u;
    lv_color_t tint = lv_color_hex(tint_hex);
    lv_draw_rect_dsc_t rect;
    lv_draw_line_dsc_t line;
    lv_point_t pts[2];
    uint8_t *buf;
    if (!cv) {
        return;
    }
    if (mq < 0) {
        mq = 0;
    }
    if (mq > 1000) {
        mq = 1000;
    }

    lv_canvas_fill_bg(cv, lv_color_hex(0), LV_OPA_TRANSP);
    buf = (uint8_t *)lv_canvas_get_img(cv)->data;

    fill_top = tip_y + ((bulb_cy + bulb_r - tip_y) * (1000 - mq)) / 1000;
    stroke_opa = 236 + ((19 * (int)glow) / 255);

    /* Soft fill: circle body + triangular tip, clipped by fill_top. */
    lim2 = (bulb_r - 1) * (bulb_r - 1);
    for (y = fill_top; y <= bulb_cy + bulb_r; y++) {
        int x;
        int x0;
        int x1;
        if (y < 0 || y >= h) {
            continue;
        }
        if (y >= tip_join) {
            dy = y - bulb_cy;
            dist2 = lim2 - dy * dy;
            if (dist2 < 0) {
                continue;
            }
            dx = 0;
            while ((dx + 1) * (dx + 1) <= dist2) {
                dx++;
            }
            x0 = cx - dx;
            x1 = cx + dx;
        } else {
            int t = tip_join - tip_y;
            int u = y - tip_y;
            int half;
            if (t < 1) {
                t = 1;
            }
            if (u < 0) {
                continue;
            }
            half = join_dx * u / t;
            x0 = cx - half;
            x1 = cx + half;
        }
        for (x = x0; x <= x1; x++) {
            int edge = x - x0;
            int er = x1 - x;
            int e = edge < er ? edge : er;
            int dt = y - fill_top;
            uint8_t a = 212;
            if (e <= 0) {
                a = 95;
            } else if (e == 1) {
                a = 155;
            } else if (e == 2) {
                a = 192;
            }
            if (dt == 0) {
                a = (uint8_t)((a * 60) / 100);
            } else if (dt == 1) {
                a = (uint8_t)((a * 82) / 100);
            }
            drop_px(buf, x, y, tint, a);
        }
    }

    hx = cx - (bulb_r * 28) / 100;
    hy = fill_top + ((bulb_cy + bulb_r - fill_top) * 38) / 100;
    hr = (bulb_r * 28) / 100;
    if (hr < 3) {
        hr = 3;
    }
    if (bulb_cy + bulb_r > fill_top + 6) {
        hl_opa = 42 + ((14 * (int)glow) / 255);
        lv_draw_rect_dsc_init(&rect);
        rect.bg_color = lv_color_mix(lv_color_hex(0xFFFFFFu), tint, (lv_opa_t)hl_opa);
        rect.bg_opa = 175;
        rect.border_width = 0;
        rect.radius = LV_RADIUS_CIRCLE;
        lv_canvas_draw_rect(cv, hx - hr, hy - hr, hr * 2, hr * 2, &rect);
    }

    /* Bulb outline — same circle border as thermometer, then clip top. */
    lv_draw_rect_dsc_init(&rect);
    rect.bg_opa = LV_OPA_TRANSP;
    rect.border_width = stroke;
    rect.border_color = tint;
    rect.border_opa = (lv_opa_t)stroke_opa;
    rect.radius = LV_RADIUS_CIRCLE;
    rect.outline_width = 0;
    rect.shadow_width = 0;
    lv_canvas_draw_rect(cv, cx - bulb_r, bulb_cy - bulb_r, bulb_r * 2, bulb_r * 2, &rect);

    /* Remove the upper circle chord so tip lines own the silhouette. */
    {
        int yy;
        int r2 = (bulb_r + stroke) * (bulb_r + stroke);
        int r2_in = (bulb_r - stroke - 1) * (bulb_r - stroke - 1);
        if (r2_in < 0) {
            r2_in = 0;
        }
        for (yy = bulb_cy - bulb_r - stroke; yy < tip_join; yy++) {
            int xx;
            if (yy < 0 || yy >= h) {
                continue;
            }
            for (xx = cx - bulb_r - stroke; xx <= cx + bulb_r + stroke; xx++) {
                int ddx = xx - cx;
                int ddy = yy - bulb_cy;
                int d2 = ddx * ddx + ddy * ddy;
                if (d2 <= r2 && d2 >= r2_in && xx >= 0 && yy >= 0 && xx < DROP_W && yy < DROP_H) {
                    uint8_t *p = buf + ((yy * DROP_W + xx) * 3);
                    p[0] = p[1] = p[2] = 0;
                }
            }
        }
    }

    /* Tip sides down to circle join points. */
    lv_draw_line_dsc_init(&line);
    line.color = tint;
    line.opa = (lv_opa_t)stroke_opa;
    line.width = stroke;
    line.round_start = 1;
    line.round_end = 1;
    pts[0].x = (lv_coord_t)cx;
    pts[0].y = (lv_coord_t)tip_y;
    pts[1].x = (lv_coord_t)lx;
    pts[1].y = (lv_coord_t)ly;
    lv_canvas_draw_line(cv, pts, 2, &line);
    pts[1].x = (lv_coord_t)rx;
    pts[1].y = (lv_coord_t)ry;
    lv_canvas_draw_line(cv, pts, 2, &line);
}

static void drop_anim_cb(void *var, int32_t v)
{
    int idx = (int)(intptr_t)var;
    if (idx < 0 || idx > 1 || !s_drop_cv[idx]) {
        return;
    }
    s_drop_now[idx] = (int)v;
    drop_draw(s_drop_cv[idx], s_drop_now[idx], s_drop_ok[idx], s_drop_glow);
}

static void drop_apply(int idx, bool ok, int hum)
{
    int target;
    lv_anim_t a;
    if (idx < 0 || idx > 1 || !s_drop_cv[idx]) {
        return;
    }
    s_drop_ok[idx] = ok;
    s_drop_h[idx] = hum;
    target = drop_target(ok, hum);
    lv_anim_del((void *)(intptr_t)idx, drop_anim_cb);
    if (s_drop_now[idx] == target) {
        drop_draw(s_drop_cv[idx], target, ok, s_drop_glow);
        return;
    }
    lv_anim_init(&a);
    lv_anim_set_var(&a, (void *)(intptr_t)idx);
    lv_anim_set_exec_cb(&a, drop_anim_cb);
    lv_anim_set_values(&a, s_drop_now[idx], target);
    lv_anim_set_time(&a, 450);
    lv_anim_set_path_cb(&a, lv_anim_path_ease_out);
    lv_anim_start(&a);
}

static void sens_paint(sens_card_t *c)
{
    /* hum 0..100; 255 = IO "ornek yok" sentinel (CAN 0x300 data[5]). */
    const bool ok = c && c->has_channel && s_th_link &&
                    s_th_t >= -40 && s_th_t <= 125 && s_th_h >= 0 && s_th_h <= 100;
    char buf[20];
    if (!c || !c->temp || !c->hum) {
        return;
    }
    if (ok) {
        snprintf(buf, sizeof(buf), "%d °C", s_th_t);
        lv_label_set_text(c->temp, buf);
        snprintf(buf, sizeof(buf), "%d %%", s_th_h);
        lv_label_set_text(c->hum, buf);
    } else {
        lv_label_set_text(c->temp, "--.- °C");
        lv_label_set_text(c->hum, "-- %");
    }
    if (c->status) {
        if (ok) {
            lv_label_set_text(c->status, vango_tx("Linked", "Bağlı", "Verbunden"));
            lv_obj_set_style_text_color(c->status, lv_color_hex(0x65D95Bu), 0);
        } else {
            lv_label_set_text(c->status, vango_tx("No link", "Bağlantı Yok", "Keine Verbindung"));
            lv_obj_set_style_text_color(c->status, lv_color_hex(0xE8B35Du), 0);
        }
        lv_obj_align(c->status, LV_ALIGN_TOP_RIGHT, -14, 16);
    }
    if (c->warn) {
        if (ok) {
            lv_obj_add_flag(c->warn, LV_OBJ_FLAG_HIDDEN);
        } else {
            lv_obj_clear_flag(c->warn, LV_OBJ_FLAG_HIDDEN);
            if (c->status) {
                lv_obj_align_to(c->warn, c->status, LV_ALIGN_OUT_LEFT_MID, -4, 0);
            }
        }
    }
    if (c->dot) {
        if (ok) {
            lv_obj_clear_flag(c->dot, LV_OBJ_FLAG_HIDDEN);
            if (c->status) {
                lv_obj_align_to(c->dot, c->status, LV_ALIGN_OUT_LEFT_MID, -8, 0);
            }
        } else {
            lv_obj_add_flag(c->dot, LV_OBJ_FLAG_HIDDEN);
        }
    }
    thermo_apply(c->slot, ok, s_th_t);
    drop_apply(c->slot, ok, s_th_h);
}

static void sens_metric(lv_obj_t *host, int x, int y, int w, int h,
                        const lv_img_dsc_t *icon, uint32_t icon_col, uint16_t zoom,
                        const char *caption, lv_obj_t **value, int thermo_slot, int drop_slot)
{
    lv_obj_t *pane = lv_obj_create(host);
    lv_obj_t *slot;
    lv_obj_t *cap;
    const int block = 156;
    const int top = (h > block) ? (h - block) / 2 : 0;
    lv_obj_set_pos(pane, x, y);
    lv_obj_set_size(pane, w, h);
    lv_obj_set_style_bg_opa(pane, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(pane, 0, 0);
    lv_obj_set_style_pad_all(pane, 0, 0);
    lv_obj_clear_flag(pane, LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_CLICKABLE);

    slot = lv_obj_create(pane);
    lv_obj_set_size(slot, w, 96);
    lv_obj_set_pos(slot, 0, top);
    lv_obj_set_style_bg_opa(slot, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(slot, 0, 0);
    lv_obj_set_style_pad_all(slot, 0, 0);
    lv_obj_clear_flag(slot, LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_CLICKABLE);

    if (thermo_slot >= 0 && thermo_slot < 2) {
        lv_obj_t *cv = lv_canvas_create(slot);
        lv_canvas_set_buffer(cv, s_thermo_buf[thermo_slot], THERMO_W, THERMO_H, LV_IMG_CF_TRUE_COLOR_ALPHA);
        lv_obj_center(cv);
        lv_obj_clear_flag(cv, LV_OBJ_FLAG_CLICKABLE);
        s_thermo_cv[thermo_slot] = cv;
        s_thermo_now[thermo_slot] = 80;
        thermo_draw(cv, 80, false, 0, s_thermo_glow);
    } else if (drop_slot >= 0 && drop_slot < 2) {
        lv_obj_t *cv = lv_canvas_create(slot);
        lv_canvas_set_buffer(cv, s_drop_buf[drop_slot], DROP_W, DROP_H, LV_IMG_CF_TRUE_COLOR_ALPHA);
        lv_obj_center(cv);
        lv_obj_clear_flag(cv, LV_OBJ_FLAG_CLICKABLE);
        s_drop_cv[drop_slot] = cv;
        s_drop_now[drop_slot] = 120;
        s_drop_ok[drop_slot] = false;
        drop_draw(cv, 120, false, s_drop_glow);
    } else if (icon) {
        lv_obj_t *ico = lv_img_create(slot);
        lv_img_set_src(ico, icon);
        lv_img_set_zoom(ico, zoom);
        lv_obj_center(ico);
        lv_obj_set_style_img_recolor(ico, lv_color_hex(icon_col), 0);
        lv_obj_set_style_img_recolor_opa(ico, LV_OPA_COVER, 0);
        lv_obj_clear_flag(ico, LV_OBJ_FLAG_CLICKABLE);
    }

    cap = make_label(pane, caption, &vango_font_16, 0xB8C6D2u);
    lv_obj_set_width(cap, w);
    lv_obj_set_style_text_align(cap, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_pos(cap, 0, top + 100);

    *value = make_label(pane, "", &vango_font_26, 0xFFFFFFu);
    lv_obj_set_width(*value, w);
    lv_obj_set_style_text_align(*value, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_pos(*value, 0, top + 124);
}

static void add_sens_card(lv_obj_t *parent, int x, int y, int w, int h, int slot)
{
    sens_card_t *c = &s_sens[s_sens_n];
    const char *title = (slot == 0) ? vango_tx("Temp - Humidity 1", "Sıcaklık - Nem 1", "Temp. - Feuchte 1")
                                     : vango_tx("Temp - Humidity 2", "Sıcaklık - Nem 2", "Temp. - Feuchte 2");
    const int head = 52;
    const int half = w / 2;
    const int body_y = head + 8;
    const int body_h = h - body_y - 10;
    lv_obj_t *card;
    lv_obj_t *ico;
    lv_obj_t *hd;
    lv_obj_t *rule;
    lv_obj_t *split;

    memset(c, 0, sizeof(*c));
    c->slot = slot;
    c->has_channel = (slot == 0);

    card = lv_obj_create(parent);
    lv_obj_set_size(card, w, h);
    lv_obj_set_pos(card, x, y);
    style_card(card);
    lv_obj_set_style_bg_color(card, lv_color_hex(COL_ACCENT_CARD), 0);
    lv_obj_set_style_bg_grad_color(card, lv_color_hex(COL_BG), 0);
    lv_obj_set_style_bg_grad_dir(card, LV_GRAD_DIR_VER, 0);
    lv_obj_set_style_radius(card, 16, 0);
    lv_obj_set_style_border_width(card, 1, 0);
    lv_obj_set_style_border_color(card, lv_color_hex(COL_ACCENT_DIM), 0);
    lv_obj_set_style_border_opa(card, LV_OPA_COVER, 0);

    ico = lv_img_create(card);
    lv_img_set_src(ico, &vango_nav_sensor);
    lv_img_set_zoom(ico, 192);
    lv_obj_set_pos(ico, 12, 8);
    lv_obj_set_style_img_recolor(ico, lv_color_hex(COL_ACCENT), 0);
    lv_obj_set_style_img_recolor_opa(ico, LV_OPA_COVER, 0);
    lv_obj_clear_flag(ico, LV_OBJ_FLAG_CLICKABLE);

    hd = make_label(card, title, &vango_font_20, 0xFFFFFFu);
    lv_obj_set_pos(hd, 50, 14);

    c->status = make_label(card, "", &vango_font_14, 0xE8B35Du);
    c->warn = make_label(card, LV_SYMBOL_WARNING, &lv_font_montserrat_14, 0xE8B35Du);
    c->dot = lv_obj_create(card);
    lv_obj_set_size(c->dot, 8, 8);
    lv_obj_set_style_radius(c->dot, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_color(c->dot, lv_color_hex(0x65D95Bu), 0);
    lv_obj_set_style_bg_opa(c->dot, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(c->dot, 0, 0);
    lv_obj_clear_flag(c->dot, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_flag(c->dot, LV_OBJ_FLAG_HIDDEN);

    rule = lv_obj_create(card);
    lv_obj_set_size(rule, w - 28, 1);
    lv_obj_set_pos(rule, 14, head);
    lv_obj_set_style_bg_color(rule, lv_color_hex(COL_ACCENT_DIM), 0);
    lv_obj_set_style_bg_opa(rule, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(rule, 0, 0);
    lv_obj_set_style_radius(rule, 0, 0);
    lv_obj_clear_flag(rule, LV_OBJ_FLAG_CLICKABLE);

    split = lv_obj_create(card);
    lv_obj_set_size(split, 1, 150);
    lv_obj_set_pos(split, half, body_y + 18 + ((body_h - 24) > 156 ? ((body_h - 24) - 156) / 2 : 0));
    lv_obj_set_style_bg_color(split, lv_color_hex(COL_ACCENT_DIM), 0);
    lv_obj_set_style_bg_opa(split, LV_OPA_80, 0);
    lv_obj_set_style_border_width(split, 0, 0);
    lv_obj_clear_flag(split, LV_OBJ_FLAG_CLICKABLE);

    sens_metric(card, 0, body_y + 18, half, body_h - 24,
                NULL, 0, 0,
                vango_tx("Temperature", "Sıcaklık", "Temperatur"), &c->temp, slot, -1);
    sens_metric(card, half, body_y + 18, half, body_h - 24,
                NULL, 0, 0,
                vango_tx("Humidity", "Nem", "Feuchte"), &c->hum, -1, slot);

    s_sens_n++;
    sens_paint(c);
}

static void build_sensors_page(lv_obj_t *parent)
{
    const hexnet_io_profile_t *p = hexnet_io_profile_get();
    int slots[2];
    int n = 0;
    const int margin = 16;
    const int gap = 16;
    const int card_y = 8;
    int i;
    lv_obj_set_style_bg_color(parent, lv_color_hex(COL_BG), 0);
    lv_obj_set_style_bg_opa(parent, LV_OPA_COVER, 0);
    thermo_release();
    s_sens_n = 0;
    memset(s_sens, 0, sizeof(s_sens));
    if (p && (p->sensor_enable_mask & 0x01u)) {
        slots[n++] = 0;
    }
    if (p && (p->sensor_enable_mask & 0x02u)) {
        slots[n++] = 1;
    }
    if (n == 0) {
        lv_obj_t *msg = make_label(parent,
                                   vango_tx("No active sensor", "Aktif sensör bulunmuyor", "Kein aktiver Sensor"),
                                   &vango_font_18, 0xB8C6D2u);
        lv_obj_align(msg, LV_ALIGN_CENTER, 0, 0);
        return;
    }
    {
        const int card_h = DASH_TILE_H - card_y - 8;
        const int card_w = (DASH_W - 2 * margin - gap) / 2;
        const int x0 = (n == 1) ? (DASH_W - card_w) / 2 : margin;
        for (i = 0; i < n; i++) {
            add_sens_card(parent, x0 + i * (card_w + gap), card_y, card_w, card_h, slots[i]);
        }
    }
}

static void build_placeholder_page(lv_obj_t *parent, const char *msg)
{
    lv_obj_t *card = lv_obj_create(parent);
    lv_obj_set_size(card, 760, 260);
    lv_obj_align(card, LV_ALIGN_TOP_MID, 0, 20);
    style_card(card);

    lv_obj_t *l = make_label(card, msg, &vango_font_18, COL_MUTED);
    lv_obj_center(l);
}

static void back_cb(lv_event_t *e)
{
    (void)e;
    vango_dash_show(VANGO_PAGE_HOME);
}

static void nav_cb(lv_event_t *e)
{
    vango_page_t page = (vango_page_t)(intptr_t)lv_event_get_user_data(e);
    vango_dash_show(page);
}

static void tileview_changed_cb(lv_event_t *e)
{
    if (s_syncing_tile) {
        return;
    }
    lv_obj_t *tv = lv_event_get_target(e);
    lv_obj_t *tile = lv_tileview_get_tile_act(tv);
    if (!tile) {
        return;
    }
    vango_page_t page = (vango_page_t)(intptr_t)lv_obj_get_user_data(tile);
    if (page == s_page) {
        return;
    }
    s_page = page;
    apply_page_chrome(page);
    ensure_page_built(page);
}

/* LVGL default scroll snap is 200–400 ms — keep page flips snappy */
#define DASH_SWIPE_ANIM_MS  90

static void tileview_scroll_begin_cb(lv_event_t *e)
{
    lv_anim_t *a = lv_event_get_param(e);
    if (a && a->time > DASH_SWIPE_ANIM_MS) {
        a->time = DASH_SWIPE_ANIM_MS;
    }
}

static void status_add_vdiv(lv_obj_t *bar, int x)
{
    /* Alt menü ile ayni dil: 1 px, ust/alt ~8 px boşluk, cyan kuralın üstünde */
    const int inset = 8;
    lv_obj_t *div = lv_obj_create(bar);
    lv_obj_set_size(div, 1, DASH_STATUS_H - 2 - 2 * inset);
    lv_obj_set_pos(div, x, inset);
    lv_obj_set_style_bg_color(div, lv_color_hex(DASH_NAV_RULE), 0);
    lv_obj_set_style_bg_opa(div, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(div, 0, 0);
    lv_obj_set_style_border_width(div, 0, 0);
    lv_obj_set_style_pad_all(div, 0, 0);
    lv_obj_set_style_shadow_width(div, 0, 0);
    lv_obj_clear_flag(div, LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_CLICKABLE);
}

static void build_status_bar(lv_obj_t *parent)
{
    lv_obj_t *bar = lv_obj_create(parent);
    s_status_bar = bar;
    lv_obj_set_size(bar, DASH_W, DASH_STATUS_H);
    lv_obj_set_pos(bar, 0, 0);
    lv_obj_set_style_bg_color(bar, lv_color_hex(COL_BG), 0);
    lv_obj_set_style_bg_opa(bar, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(bar, 0, 0);
    lv_obj_set_style_pad_all(bar, 0, 0);
    lv_obj_set_style_clip_corner(bar, true, 0);
    lv_obj_clear_flag(bar, LV_OBJ_FLAG_SCROLLABLE);

    s_btn_back = lv_btn_create(bar);
    lv_obj_set_size(s_btn_back, 36, 36);
    lv_obj_set_pos(s_btn_back, 8, 4);
    lv_obj_set_style_bg_opa(s_btn_back, LV_OPA_TRANSP, 0);
    lv_obj_set_style_shadow_width(s_btn_back, 0, 0);
    lv_obj_t *back_lbl = make_label(s_btn_back, LV_SYMBOL_LEFT, &lv_font_montserrat_18, COL_TEXT);
    lv_obj_center(back_lbl);
    lv_obj_add_event_cb(s_btn_back, back_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_add_flag(s_btn_back, LV_OBJ_FLAG_HIDDEN);

    /* Hexnet_logo_0.png → 34×34 native (oran korunur, zoom yok) */
    s_img_logo = lv_img_create(bar);
    lv_img_set_src(s_img_logo, &vango_icon_logo);
    lv_img_set_zoom(s_img_logo, 256);
    lv_obj_set_size(s_img_logo, 34, 34);
    /* 36×36 alan içinde ortala: x=8, y=(46-34)/2=6 */
    lv_obj_set_pos(s_img_logo, 8, 6);
    lv_obj_clear_flag(s_img_logo, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);

    s_title = make_label(bar, "VanGO", &vango_font_20, COL_GREEN);
    /* logo sağ kenar 8+34=42, +6 px boşluk → 48 */
    lv_obj_set_pos(s_title, 48, 12);

    s_demo_badge = make_label(bar, "DEMO", &vango_font_12, COL_ACCENT);
    lv_obj_set_pos(s_demo_badge, 128, 14);
    lv_obj_add_flag(s_demo_badge, LV_OBJ_FLAG_HIDDEN);
    demo_badge_refresh();

    /* Marka | batarya | tanklar | sicaklik | baglanti */
    status_add_vdiv(bar, 176);

    lv_obj_t *bat_ico = lv_img_create(bar);
    lv_img_set_src(bat_ico, &vango_icon_battery);
    lv_obj_set_pos(bat_ico, 190, 10);
    s_lbl_bat = make_label(bar, "12.0 V", &vango_font_14, COL_GREEN);
    lv_obj_set_pos(s_lbl_bat, 258, 13);

    status_add_vdiv(bar, 316);

    lv_obj_t *d1 = lv_img_create(bar);
    lv_img_set_src(d1, &vango_icon_drop_clean);
    lv_img_set_zoom(d1, 180);
    lv_obj_set_pos(d1, 330, 2);
    s_lbl_w_clean = make_label(bar, "--%", &vango_font_12, COL_WATER_C);
    lv_obj_set_pos(s_lbl_w_clean, 360, 8);

    lv_obj_t *d2 = lv_img_create(bar);
    lv_img_set_src(d2, &vango_icon_drop_waste);
    lv_img_set_zoom(d2, 180);
    lv_obj_set_style_img_recolor(d2, lv_color_hex(COL_WATER_W), 0);
    lv_obj_set_style_img_recolor_opa(d2, LV_OPA_COVER, 0);
    lv_obj_set_pos(d2, 410, 2);
    s_lbl_w_waste = make_label(bar, "--%", &vango_font_12, COL_WATER_W);
    lv_obj_set_pos(s_lbl_w_waste, 440, 8);

    lv_obj_t *d3 = lv_img_create(bar);
    lv_img_set_src(d3, &vango_icon_drop_grey);
    lv_img_set_zoom(d3, 180);
    lv_obj_set_pos(d3, 490, 2);
    s_lbl_w_grey = make_label(bar, "--%", &vango_font_12, COL_WATER_G);
    lv_obj_set_pos(s_lbl_w_grey, 520, 8);

    status_add_vdiv(bar, 568);

    s_lbl_temp = make_label(bar, "0 °C", &vango_font_14, COL_TEXT);
    lv_obj_set_pos(s_lbl_temp, 584, 13);

    status_add_vdiv(bar, 656);

    /* Sag ust: IO mikroçip · BLE · WiFi — konumlar ayni, sadece IO ikon */
    s_io_ico = lv_img_create(bar);
    lv_img_set_src(s_io_ico, &vango_icon_io_chip);
    lv_img_set_zoom(s_io_ico, 256); /* ALPHA_8BIT: zoom sabit */
    lv_obj_set_size(s_io_ico, 24, 24);
    lv_obj_set_pos(s_io_ico, 670, 11);
    lv_obj_set_style_img_recolor(s_io_ico, lv_color_hex(COL_IO_WAIT), 0);
    lv_obj_set_style_img_recolor_opa(s_io_ico, LV_OPA_COVER, 0);
    lv_obj_clear_flag(s_io_ico, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);
    s_ble_lbl = make_label(bar, LV_SYMBOL_BLUETOOTH, &lv_font_montserrat_16, COL_MUTED);
    lv_obj_set_pos(s_ble_lbl, 712, 11);
    s_wifi_lbl = make_label(bar, LV_SYMBOL_WIFI, &lv_font_montserrat_16, COL_MUTED);
    lv_obj_set_pos(s_wifi_lbl, 748, 11);
    s_link_io = s_link_ble = s_link_wifi = false;
    s_io_wait_until_us = esp_timer_get_time() + 8000000LL; /* ~8 sn baglaniyor */

    /* Ust ayirac: 2 px cyan, L/R 12 px inset — ikonlarla cakismasin */
    lv_obj_t *rule = lv_obj_create(bar);
    lv_obj_set_size(rule, DASH_W - 24, 2);
    lv_obj_set_pos(rule, 12, DASH_STATUS_H - 2);
    lv_obj_set_style_bg_color(rule, lv_color_hex(DASH_STATUS_RULE), 0);
    lv_obj_set_style_bg_opa(rule, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(rule, 0, 0);
    lv_obj_set_style_border_width(rule, 0, 0);
    lv_obj_set_style_pad_all(rule, 0, 0);
    lv_obj_set_style_shadow_width(rule, 0, 0);
    lv_obj_clear_flag(rule, LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_CLICKABLE);
    lv_obj_move_foreground(rule);
    s_status_rule = rule;
}

static void build_bottom_nav(lv_obj_t *parent)
{
    static const lv_img_dsc_t *icons[] = {
        &vango_nav_home,
        &vango_nav_light,
        &vango_nav_rgb,
        &vango_nav_water,
        &vango_nav_sensor,
        &vango_nav_motor,
        &vango_nav_settings
    };
    /* ~40 px kaynak → ~27 px; 7 slot (~114 px) */
    const uint16_t ico_zoom = 172;
    const int slot = DASH_W / VANGO_PAGE_COUNT;
    const int div_h = DASH_NAV_H - 20; /* ust/alt 10 px */
    int i;

    s_nav_bar = lv_obj_create(parent);
    lv_obj_set_size(s_nav_bar, DASH_W, DASH_NAV_H);
    lv_obj_align(s_nav_bar, LV_ALIGN_BOTTOM_MID, 0, -DASH_SAFE_BOTTOM);
    lv_obj_set_style_bg_color(s_nav_bar, lv_color_hex(vango_theme_dark() ? DASH_NAV_BG : COL_NAV), 0);
    lv_obj_set_style_bg_opa(s_nav_bar, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(s_nav_bar, 1, 0);
    lv_obj_set_style_border_side(s_nav_bar, LV_BORDER_SIDE_TOP, 0);
    lv_obj_set_style_border_color(s_nav_bar,
                                  lv_color_hex(vango_theme_dark() ? DASH_NAV_RULE : COL_NAV_BORDER), 0);
    lv_obj_set_style_border_opa(s_nav_bar, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(s_nav_bar, 0, 0);
    lv_obj_set_style_pad_all(s_nav_bar, 0, 0);
    lv_obj_set_style_shadow_width(s_nav_bar, 0, 0);
    lv_obj_clear_flag(s_nav_bar, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_move_foreground(s_nav_bar);

    /* Oge arasi dikey ayirac — secili buton ustune binince bolunmez */
    for (i = 1; i < VANGO_PAGE_COUNT; i++) {
        lv_obj_t *div = lv_obj_create(s_nav_bar);
        lv_obj_set_size(div, 1, div_h);
        lv_obj_set_pos(div, i * slot, 10);
        lv_obj_set_style_bg_color(div, lv_color_hex(DASH_NAV_RULE), 0);
        lv_obj_set_style_bg_opa(div, LV_OPA_COVER, 0);
        lv_obj_set_style_radius(div, 0, 0);
        lv_obj_set_style_border_width(div, 0, 0);
        lv_obj_set_style_pad_all(div, 0, 0);
        lv_obj_set_style_shadow_width(div, 0, 0);
        lv_obj_clear_flag(div, LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_CLICKABLE);
    }

    for (i = 0; i < VANGO_PAGE_COUNT; i++) {
        lv_obj_t *btn = lv_btn_create(s_nav_bar);
        lv_obj_t *clip;
        lv_obj_set_size(btn, slot, DASH_NAV_H);
        lv_obj_set_pos(btn, i * slot, 0);
        lv_obj_set_style_radius(btn, 0, 0);
        lv_obj_set_style_bg_color(btn, lv_color_hex(COL_NAV_SEL), 0);
        lv_obj_set_style_bg_opa(btn, LV_OPA_TRANSP, 0);
        lv_obj_set_style_shadow_width(btn, 0, 0);
        lv_obj_set_style_pad_all(btn, 0, 0);
        lv_obj_set_style_border_width(btn, 0, 0);
        lv_obj_clear_flag(btn, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_add_event_cb(btn, nav_cb, LV_EVENT_CLICKED, (void *)(intptr_t)i);
        s_nav_btns[i] = btn;

        clip = lv_obj_create(btn);
        lv_obj_set_size(clip, 34, 30);
        lv_obj_align(clip, LV_ALIGN_TOP_MID, 0, 2);
        lv_obj_set_style_bg_opa(clip, LV_OPA_TRANSP, 0);
        lv_obj_set_style_border_width(clip, 0, 0);
        lv_obj_set_style_outline_width(clip, 0, 0);
        lv_obj_set_style_pad_all(clip, 0, 0);
        lv_obj_set_style_radius(clip, 0, 0);
        lv_obj_set_style_shadow_width(clip, 0, 0);
        lv_obj_clear_flag(clip, LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_CLICKABLE);
        lv_obj_set_style_clip_corner(clip, false, 0);

        s_nav_icos[i] = lv_img_create(clip);
        if (i == VANGO_PAGE_MOTORS) {
            lv_img_set_src(s_nav_icos[i], vango_dash_motor_icon(true));
            lv_obj_set_style_img_recolor_opa(s_nav_icos[i], LV_OPA_TRANSP, 0);
            lv_obj_set_style_img_opa(s_nav_icos[i], LV_OPA_COVER, 0);
        } else if (i == VANGO_PAGE_RGB) {
            lv_img_set_src(s_nav_icos[i], icons[i]);
            lv_obj_set_style_img_recolor_opa(s_nav_icos[i], LV_OPA_TRANSP, 0);
            lv_obj_set_style_img_opa(s_nav_icos[i], LV_OPA_70, 0);
        } else {
            lv_img_set_src(s_nav_icos[i], icons[i]);
            lv_obj_set_style_img_recolor(s_nav_icos[i], lv_color_hex(COL_MUTED), 0);
            lv_obj_set_style_img_recolor_opa(s_nav_icos[i], LV_OPA_COVER, 0);
        }
        lv_img_set_zoom(s_nav_icos[i], ico_zoom);
        lv_obj_center(s_nav_icos[i]);
        lv_obj_clear_flag(s_nav_icos[i], LV_OBJ_FLAG_CLICKABLE);
        lv_obj_add_flag(s_nav_icos[i], LV_OBJ_FLAG_EVENT_BUBBLE);

        s_nav_lbls[i] = make_label(btn, nav_label(i), &vango_font_12, COL_MUTED);
        lv_obj_set_width(s_nav_lbls[i], slot - 6);
        lv_label_set_long_mode(s_nav_lbls[i], LV_LABEL_LONG_CLIP);
        lv_obj_set_style_text_align(s_nav_lbls[i], LV_TEXT_ALIGN_CENTER, 0);
        lv_obj_align(s_nav_lbls[i], LV_ALIGN_BOTTOM_MID, 0, -4);
        lv_obj_move_foreground(s_nav_lbls[i]);
    }
}

static void build_dots(lv_obj_t *parent)
{
    /* Röle alt sayfa göstergesi (6+6) — ana bölümler arası değil */
    const int total_w = RELAY_SUBPAGE_COUNT * 14;
    const int start_x = (DASH_W - total_w) / 2;
    for (int i = 0; i < RELAY_SUBPAGE_COUNT; i++) {
        s_dots[i] = lv_obj_create(parent);
        lv_obj_set_size(s_dots[i], 8, 8);
        lv_obj_set_pos(s_dots[i], start_x + i * 14, DASH_TILE_H + 6);
        lv_obj_set_style_radius(s_dots[i], LV_RADIUS_CIRCLE, 0);
        lv_obj_set_style_bg_color(s_dots[i], lv_color_hex(i == 0 ? COL_ACCENT : COL_DOT_OFF), 0);
        lv_obj_set_style_bg_opa(s_dots[i], LV_OPA_COVER, 0);
        lv_obj_set_style_border_width(s_dots[i], 0, 0);
        lv_obj_clear_flag(s_dots[i], LV_OBJ_FLAG_CLICKABLE);
    }
}

static void ensure_page_built(vango_page_t page)
{
    if (page < 0 || page >= VANGO_PAGE_COUNT || !s_tiles[page] || s_page_built[page]) {
        return;
    }
    switch (page) {
        case VANGO_PAGE_HOME:
            build_home_page(s_tiles[page]);
            break;
        case VANGO_PAGE_DIMMER:
            build_dimmer_page(s_tiles[page]);
            break;
        case VANGO_PAGE_RGB:
            vango_rgb_build(s_tiles[page]);
            break;
        case VANGO_PAGE_WATER:
            build_water_page(s_tiles[page]);
            break;
        case VANGO_PAGE_SENSORS:
            build_sensors_page(s_tiles[page]);
            break;
        case VANGO_PAGE_MOTORS:
            build_motors_page(s_tiles[page]);
            break;
        case VANGO_PAGE_SETTINGS:
            vango_settings_build(s_tiles[page]);
            break;
        default:
            break;
    }
    s_page_built[page] = true;
}

static void build_pages(void)
{
    s_tileview = lv_tileview_create(s_content);
    lv_obj_set_size(s_tileview, DASH_W, DASH_TILE_H);
    lv_obj_set_pos(s_tileview, 0, 0);
    lv_obj_set_style_bg_color(s_tileview, lv_color_hex(COL_BG), 0);
    lv_obj_set_style_bg_opa(s_tileview, LV_OPA_COVER, 0);
    lv_obj_set_style_pad_all(s_tileview, 0, 0);
    /* Ana bölümler arası yana kaydırma kapalı — sadece alt menü ile geçiş */
    lv_obj_clear_flag(s_tileview, LV_OBJ_FLAG_SCROLLABLE);

    for (int i = 0; i < VANGO_PAGE_COUNT; i++) {
        s_tiles[i] = lv_tileview_add_tile(s_tileview, i, 0, LV_DIR_NONE);
        lv_obj_set_user_data(s_tiles[i], (void *)(intptr_t)i);
        lv_obj_set_style_bg_color(s_tiles[i], lv_color_hex(COL_BG), 0);
        lv_obj_set_style_bg_opa(s_tiles[i], LV_OPA_COVER, 0);
        lv_obj_set_style_pad_all(s_tiles[i], 0, 0);
        lv_obj_clear_flag(s_tiles[i], LV_OBJ_FLAG_SCROLLABLE);
        s_page_built[i] = false;
    }

    ensure_page_built(VANGO_PAGE_HOME);
    ensure_page_built(VANGO_PAGE_DIMMER);
    ensure_page_built(VANGO_PAGE_SETTINGS);
    build_dots(s_content);
}

void vango_dash_init(void)
{
    s_scr = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(s_scr, lv_color_hex(COL_BG), 0);
    lv_obj_set_style_bg_opa(s_scr, LV_OPA_COVER, 0);
    lv_obj_clear_flag(s_scr, LV_OBJ_FLAG_SCROLLABLE);

    build_status_bar(s_scr);

    s_content = lv_obj_create(s_scr);
    lv_obj_set_pos(s_content, 0, DASH_CONTENT_Y);
    lv_obj_set_size(s_content, DASH_W, DASH_CONTENT_H);
    lv_obj_set_style_bg_color(s_content, lv_color_hex(COL_BG), 0);
    lv_obj_set_style_bg_opa(s_content, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(s_content, 0, 0);
    lv_obj_set_style_pad_all(s_content, 0, 0);
    lv_obj_set_style_radius(s_content, 0, 0);
    lv_obj_clear_flag(s_content, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_clear_flag(s_content, LV_OBJ_FLAG_OVERFLOW_VISIBLE);

    build_pages();
    build_bottom_nav(s_scr);
    lv_obj_move_foreground(s_nav_bar);
    brightness_load();
    build_dim_veil(s_scr);
    s_auto_dark_cached = vango_theme_dark();
    lv_timer_create(auto_theme_tick_cb, 60000, NULL); /* reserved (system follows IO) */
    lv_timer_create(water_demo_tick, 800, NULL);
    tank_cap_load();
    /* 60 ms canvas paint overloaded LVGL+RGB flush → CPU0 IWDT under CAN/console load */
    lv_timer_create(water_live_tick, 150, NULL);
    lv_timer_create(thermo_glow_tick, 150, NULL);
    water_demo_apply();
    vango_dash_show(VANGO_PAGE_HOME);
    ESP_LOGI(TAG, "VANGO dash ready swipe tiles=%d content_h=%d", VANGO_PAGE_COUNT, DASH_CONTENT_H);
}

void vango_dash_show(vango_page_t page)
{
    if (!s_scr || !s_tileview) {
        return;
    }
    hexnet_wallpaper_note_activity();
    if (page < 0 || page >= VANGO_PAGE_COUNT) {
        page = VANGO_PAGE_HOME;
    }

    if (s_page == VANGO_PAGE_MOTORS && page != VANGO_PAGE_MOTORS) {
        motor_halt();
    }
    if (page == VANGO_PAGE_MOTORS && s_page != VANGO_PAGE_MOTORS && s_tiles[page] && s_page_built[page]) {
        motor_halt();
        lv_obj_clean(s_tiles[page]);
        memset(s_mot, 0, sizeof(s_mot));
        s_page_built[page] = false;
    }
    if (page == VANGO_PAGE_SENSORS && s_page != VANGO_PAGE_SENSORS && s_tiles[page] && s_page_built[page]) {
        thermo_release();
        lv_obj_clean(s_tiles[page]);
        s_sens_n = 0;
        memset(s_sens, 0, sizeof(s_sens));
        s_page_built[page] = false;
    }
    if (page == VANGO_PAGE_DIMMER && s_page != VANGO_PAGE_DIMMER && s_tiles[page] && s_page_built[page]) {
        lv_obj_clean(s_tiles[page]);
        s_dim_count = 0;
        memset(s_dims, 0, sizeof(s_dims));
        s_page_built[page] = false;
    }

    s_page = page;
    apply_page_chrome(page);
    ensure_page_built(page);
    if (page == VANGO_PAGE_RGB) {
        vango_rgb_refresh();
    }

    /* Prefetch neighbor for smoother section switch */
    if (page + 1 < VANGO_PAGE_COUNT) {
        ensure_page_built((vango_page_t)(page + 1));
    }
    if (page > 0) {
        ensure_page_built((vango_page_t)(page - 1));
    }

    s_syncing_tile = true;
    lv_obj_set_tile_id(s_tileview, (uint32_t)page, 0, LV_ANIM_OFF);
    s_syncing_tile = false;

    update_relay_dots(s_relay_subpage);
    if (page == VANGO_PAGE_HOME) {
        vango_dash_refresh_channel_meta();
    }

    if (s_nav_bar) {
        lv_obj_move_foreground(s_nav_bar);
    }
    if (s_dim_veil) {
        lv_obj_move_foreground(s_dim_veil);
    }
    lv_scr_load(s_scr);
}

lv_obj_t *vango_dash_get_screen(void)
{
    return s_scr;
}

static void brightness_apply(void)
{
    lv_opa_t opa;
    int pct = s_brightness;
    if (pct < 10) {
        pct = 10;
    }
    if (pct > 100) {
        pct = 100;
    }
    /* Soft dim: 100% clear; mid steps gentle; 10% ~half veil (still readable). */
    {
        int drop = 100 - pct; /* 0..90 */
        opa = (lv_opa_t)((drop * drop * 140) / (90 * 90));
    }
    if (!s_dim_veil || !s_scr) {
        return;
    }
    lv_obj_set_style_bg_opa(s_dim_veil, opa, 0);
    lv_obj_move_foreground(s_dim_veil);
}

static void brightness_load(void)
{
    nvs_handle_t h;
    int32_t v = 100;
    if (nvs_open("hexnet", NVS_READONLY, &h) != ESP_OK) {
        s_brightness = 100;
        return;
    }
    if (nvs_get_i32(h, "brPct", &v) == ESP_OK) {
        if (v < 10) {
            v = 10;
        }
        if (v > 100) {
            v = 100;
        }
        s_brightness = (int)v;
    }
    nvs_close(h);
}

static void brightness_save(void)
{
    nvs_handle_t h;
    if (nvs_open("hexnet", NVS_READWRITE, &h) != ESP_OK) {
        return;
    }
    nvs_set_i32(h, "brPct", (int32_t)s_brightness);
    nvs_commit(h);
    nvs_close(h);
}

/* LVGL/PSRAM stack'ten NVS yasak — slider VALUE_CHANGED cok sik tetikler. */
static TaskHandle_t s_br_nvs_task;

static void brightness_save_task(void *arg)
{
    (void)arg;
    for (;;) {
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
        /* Suruklerken biriken notify'lari birlestir. */
        vTaskDelay(pdMS_TO_TICKS(250));
        while (ulTaskNotifyTake(pdTRUE, 0) > 0) {
        }
        brightness_save();
    }
}

static void brightness_schedule_save(void)
{
    if (!s_br_nvs_task) {
        BaseType_t ok = xTaskCreate(brightness_save_task, "br_nvs", 3072, NULL, 2, &s_br_nvs_task);
        if (ok != pdPASS) {
            s_br_nvs_task = NULL;
            ESP_LOGW(TAG, "br_nvs task fail — skip flash");
            return;
        }
    }
    xTaskNotifyGive(s_br_nvs_task);
}

void vango_dash_set_brightness(int pct)
{
    if (pct < 10) {
        pct = 10;
    }
    if (pct > 100) {
        pct = 100;
    }
    s_brightness = pct;
    brightness_apply();
    brightness_schedule_save();
}

int vango_dash_get_brightness(void)
{
    return s_brightness;
}

static void build_dim_veil(lv_obj_t *parent)
{
    s_dim_veil = lv_obj_create(parent);
    lv_obj_set_size(s_dim_veil, DASH_W, DASH_H);
    lv_obj_set_pos(s_dim_veil, 0, 0);
    lv_obj_set_style_bg_color(s_dim_veil, lv_color_hex(0x000000u), 0);
    lv_obj_set_style_bg_opa(s_dim_veil, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(s_dim_veil, 0, 0);
    lv_obj_set_style_radius(s_dim_veil, 0, 0);
    lv_obj_set_style_pad_all(s_dim_veil, 0, 0);
    lv_obj_clear_flag(s_dim_veil, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_clear_flag(s_dim_veil, LV_OBJ_FLAG_CLICKABLE);
    brightness_apply();
}

void vango_dash_set_battery_v(float volts)
{
    s_bat_v = volts;
    if (s_lbl_bat) {
        char buf[24];
        snprintf(buf, sizeof(buf), "%.1f V", (double)volts);
        lv_label_set_text(s_lbl_bat, buf);
    }
    if (s_home_bat) {
        char buf[24];
        snprintf(buf, sizeof(buf), "%.1f V", (double)volts);
        lv_label_set_text(s_home_bat, buf);
    }
}

void vango_dash_set_th(int temp_c, int hum_pct, bool linked)
{
    int i;
    if (temp_c == s_th_t && hum_pct == s_th_h && linked == s_th_link) {
        return;
    }
    s_th_t = temp_c;
    s_th_h = hum_pct;
    s_th_link = linked;
    for (i = 0; i < s_sens_n; i++) {
        sens_paint(&s_sens[i]);
    }
}

void vango_dash_set_temp_c(int temp_c)
{
    s_temp_c = temp_c;
    if (s_lbl_temp) {
        char buf[24];
        snprintf(buf, sizeof(buf), "%d °C", temp_c);
        lv_label_set_text(s_lbl_temp, buf);
    }
    if (s_home_temp) {
        char buf[24];
        snprintf(buf, sizeof(buf), "%d °C", temp_c);
        lv_label_set_text(s_home_temp, buf);
    }
}

static void water_status_text(lv_obj_t *lbl, int pct)
{
    if (!lbl) {
        return;
    }
    char buf[16];
    if (pct >= 0 && pct <= 100) {
        snprintf(buf, sizeof(buf), "%d%%", pct);
    } else {
        snprintf(buf, sizeof(buf), "--%%");
    }
    lv_label_set_text(lbl, buf);
}

static void water_show(int clean, int waste, int grey)
{
    s_w_clean = water_normalize(clean);
    s_w_waste = water_normalize(waste);
    s_w_grey = water_normalize(grey);

    water_status_text(s_lbl_w_clean, s_w_clean);
    water_status_text(s_lbl_w_waste, s_w_waste);
    water_status_text(s_lbl_w_grey, s_w_grey);
    water_status_text(s_home_w[0], s_w_clean);
    water_status_text(s_home_w[1], s_w_waste);
    water_status_text(s_home_w[2], s_w_grey);

    water_apply_tank(0, s_w_clean);
    water_apply_tank(1, s_w_waste);
    water_apply_tank(2, s_w_grey);
}

int vango_dash_tank_capacity(int idx)
{
    if (idx < 0 || idx > 2) {
        return 0;
    }
    return s_tank_cap[idx];
}

void vango_dash_set_tank_capacity(int idx, int liters)
{
    int pct;
    if (idx < 0 || idx > 2) {
        return;
    }
    if (liters < 0) {
        liters = 0;
    }
    if (liters > 10000) {
        liters = 10000;
    }
    s_tank_cap[idx] = liters;
    tank_cap_schedule_save();
    pct = (idx == 0) ? s_w_clean : (idx == 1) ? s_w_waste : s_w_grey;
    water_apply_tank(idx, pct);
}

void vango_dash_set_water(int clean_pct, int waste_pct, int grey_pct)
{
    if (s_demo && !get_canbus_connection_status()) {
        return;
    }
    water_show(clean_pct, waste_pct, grey_pct);
}

static void link_paint_one(lv_obj_t *lbl, bool on)
{
    if (!lbl) {
        return;
    }
    lv_obj_set_style_text_color(lbl, lv_color_hex(on ? COL_GREEN : COL_MUTED), 0);
    lv_obj_set_style_text_opa(lbl, on ? LV_OPA_COVER : LV_OPA_60, 0);
}

static void io_chip_paint(bool on)
{
    uint32_t col;
    if (!s_io_ico) {
        return;
    }
    if (on) {
        col = COL_IO_UP;
        s_io_wait_until_us = 0;
    } else if (s_demo) {
        col = COL_IO_OFF; /* pasif / kapali */
    } else if (s_io_wait_until_us && esp_timer_get_time() < s_io_wait_until_us) {
        col = COL_IO_WAIT; /* baglaniyor */
    } else {
        col = COL_IO_DOWN; /* baglanti yok */
    }
    lv_obj_set_style_img_recolor(s_io_ico, lv_color_hex(col), 0);
    lv_obj_set_style_img_recolor_opa(s_io_ico, LV_OPA_COVER, 0);
}

void vango_dash_set_link_status(bool io_can, bool ble, bool wifi)
{
    if (s_link_io == io_can && s_link_ble == ble && s_link_wifi == wifi) {
        /* Baglaniyor → kirmizi gecisi icin sure dolunca yeniden boya */
        if (!io_can && s_io_wait_until_us && esp_timer_get_time() >= s_io_wait_until_us) {
            s_io_wait_until_us = 0;
            io_chip_paint(false);
        }
        return;
    }
    s_link_io = io_can;
    s_link_ble = ble;
    s_link_wifi = wifi;
    io_chip_paint(io_can);
    link_paint_one(s_ble_lbl, ble);
    link_paint_one(s_wifi_lbl, wifi);
}

void vango_dash_set_wifi(bool connected)
{
    vango_dash_set_link_status(s_link_io, s_link_ble, connected);
}

void vango_dash_set_motor_link(bool connected)
{
    if (connected) {
        vango_dash_set_demo(false);
    }
    if (s_motor_link == connected) {
        return;
    }
    s_motor_link = connected;
    if (!connected) {
        motor_halt();
    }
    motor_paint();
}

void vango_dash_set_dimmer_link(bool connected)
{
    int i;
    if (s_dim_link == connected) {
        return;
    }
    s_dim_link = connected;
    for (i = 0; i < s_dim_count; i++) {
        dim_apply(&s_dims[i]);
    }
}

void vango_dash_sync_relays(void)
{
    static bool link_drawn;
    static int64_t s_meta_paint_at;
    static int64_t s_settings_rebuild_at;
    bool link = get_canbus_connection_status() != 0;
    uint16_t bits = link ? get_outputs() : 0;
    int i;
    const int64_t now = esp_timer_get_time();
    /* CAN 0x118 burst bitmeden LVGL paint/NVS yapma — frame kaybi oluyor */
    if (hexnet_io_labels_peek_ui_dirty() || hexnet_io_profile_consume_ui_dirty()) {
        s_meta_paint_at = now + 700000LL;
        s_settings_rebuild_at = now + 1000000LL;
    }
    if (s_meta_paint_at && now >= s_meta_paint_at) {
        s_meta_paint_at = 0;
        (void)hexnet_io_labels_consume_ui_dirty(); /* NVS burada */
        vango_dash_refresh_channel_meta();
        ESP_LOGW(TAG, "channel meta refresh (labels/profile dirty)");
    }
    if (s_settings_rebuild_at && now >= s_settings_rebuild_at) {
        s_settings_rebuild_at = 0;
        vango_settings_on_prefs_changed();
    }
    if (link && s_demo) {
        vango_dash_set_demo(false);
    }
    if (s_demo) {
        return;
    }
    for (i = 0; i < HOME_RELAYS; i++) {
        bool on = link && (bits & (1u << i)) != 0;
        if (relay_pending_active(i)) {
            /* Optimistic UI stays; clear pending once IO reports the wanted state. */
            if (link && on == s_relay_pending_want[i]) {
                relay_clear_pending(i);
                if (on != s_relay_on[i]) {
                    s_relay_on[i] = on;
                    relay_paint_one(i);
                }
            }
            continue;
        }
        if (link && on != s_relay_on[i]) {
            s_relay_on[i] = on;
            relay_paint_one(i);
        }
    }
    if (link != link_drawn) {
        link_drawn = link;
        for (i = 0; i < HOME_RELAYS; i++) {
            relay_paint_one(i);
        }
    }
}

void vango_dash_sync_dimmers(void)
{
    int i;
    if (!s_dim_link) {
        return;
    }
    for (i = 0; i < s_dim_count; i++) {
        dim_card_t *d = &s_dims[i];
        uint8_t v;
        if (!d->drive || !d->slider) {
            continue;
        }
        if (lv_obj_has_state(d->slider, LV_STATE_PRESSED)) {
            continue;
        }
        if (lv_tick_elaps(s_dim_touch_ms) < 1000) {
            continue;
        }
        v = get_dimmable_output((uint8_t)d->slot);
        if (v > 100) {
            v = 100;
        }
        if (v > 0) {
            if (d->brightness == v && d->enabled) {
                continue;
            }
            d->brightness = v;
            d->enabled = true;
        } else if (d->enabled && d->brightness > 0) {
            /* Output is off. Keep the setpoint and show the switch off. */
            d->enabled = false;
        } else {
            continue;
        }
        dim_apply(d);
    }
}

static bool s_return_from_legacy;

void vango_dash_set_return_from_legacy(bool enable)
{
    s_return_from_legacy = enable;
}

bool vango_dash_consume_return_from_legacy(void)
{
    bool v = s_return_from_legacy;
    s_return_from_legacy = false;
    return v;
}

static void dash_clear_page_state(int i)
{
    if (i == VANGO_PAGE_DIMMER) {
        s_dim_count = 0;
        memset(s_dims, 0, sizeof(s_dims));
    }
    if (i == VANGO_PAGE_HOME) {
        int r;
        s_home_bat = s_home_temp = NULL;
        s_home_w[0] = s_home_w[1] = s_home_w[2] = NULL;
        for (r = 0; r < HOME_RELAYS; r++) {
            s_relay_card[r] = s_relay_ico[r] = s_relay_st[r] = s_relay_nm[r] = NULL;
        }
    }
    if (i == VANGO_PAGE_SENSORS) {
        thermo_release();
        s_sens_n = 0;
        memset(s_sens, 0, sizeof(s_sens));
    }
    if (i == VANGO_PAGE_MOTORS) {
        motor_halt();
        memset(s_mot, 0, sizeof(s_mot));
    }
    if (i == VANGO_PAGE_WATER) {
        s_water_fill[0] = s_water_fill[1] = s_water_fill[2] = NULL;
        s_water_neck[0] = s_water_neck[1] = s_water_neck[2] = NULL;
        s_water_wave[0] = s_water_wave[1] = s_water_wave[2] = NULL;
        memset(s_water_bub, 0, sizeof(s_water_bub));
        s_water_pct_lbl[0] = s_water_pct_lbl[1] = s_water_pct_lbl[2] = NULL;
        s_water_ltr_lbl[0] = s_water_ltr_lbl[1] = s_water_ltr_lbl[2] = NULL;
        s_water_max[0] = s_water_max[1] = s_water_max[2] = 0;
        s_water_px[0] = s_water_px[1] = s_water_px[2] = 0;
        s_wave_amp[0] = s_wave_amp[1] = s_wave_amp[2] = 0;
    }
}

static void dash_restyle_chrome(void)
{
    if (s_scr) {
        lv_obj_set_style_bg_color(s_scr, lv_color_hex(COL_BG), 0);
    }
    if (s_content) {
        lv_obj_set_style_bg_color(s_content, lv_color_hex(COL_BG), 0);
    }
    if (s_status_bar) {
        lv_obj_set_style_bg_color(s_status_bar, lv_color_hex(COL_BG), 0);
    }
    if (s_status_rule) {
        lv_obj_set_style_bg_color(s_status_rule, lv_color_hex(COL_ACCENT), 0);
    }
    if (s_title) {
        lv_obj_set_style_text_color(s_title, lv_color_hex(COL_GREEN), 0);
    }
    if (s_lbl_temp) {
        lv_obj_set_style_text_color(s_lbl_temp, lv_color_hex(COL_TEXT), 0);
    }
    io_chip_paint(s_link_io);
    link_paint_one(s_ble_lbl, s_link_ble);
    link_paint_one(s_wifi_lbl, s_link_wifi);
    if (s_btn_back) {
        lv_obj_t *bl = lv_obj_get_child(s_btn_back, 0);
        if (bl) {
            lv_obj_set_style_text_color(bl, lv_color_hex(COL_TEXT), 0);
        }
    }
    if (s_nav_bar) {
        lv_obj_set_style_bg_color(s_nav_bar, lv_color_hex(COL_NAV), 0);
        lv_obj_set_style_border_color(s_nav_bar, lv_color_hex(COL_NAV_BORDER), 0);
    }
    if (s_tileview) {
        lv_obj_set_style_bg_color(s_tileview, lv_color_hex(COL_BG), 0);
    }
    for (int i = 0; i < VANGO_PAGE_COUNT; i++) {
        if (s_tiles[i]) {
            lv_obj_set_style_bg_color(s_tiles[i], lv_color_hex(COL_BG), 0);
        }
    }
    apply_page_chrome(s_page);
}

void vango_dash_on_accent_changed(void)
{
    if (!s_scr) {
        return;
    }
    /*
     * Sadece chrome renkleri — sayfa destroy/rebuild YOK.
     * (Onceki yol home+neighbors temizleyip IWDT: Interrupt wdt timeout on CPU0
     *  tetikliyordu; BT controller flash cache ile carpisiyordu.)
     * Kart accent'leri sonraki prefs/navigasyon rebuild'inde gelir.
     */
    dash_restyle_chrome();
}

void vango_dash_on_prefs_changed(void)
{
    if (!s_scr) {
        return;
    }

    dash_restyle_chrome();

    /* Rebuild page contents so cards/labels pick new palette.
     * Settings keeps static s_content pointers — always rebuild it if it
     * was live, otherwise a later show_section() hits freed objects. */
    const bool was_settings = s_page_built[VANGO_PAGE_SETTINGS];
    for (int i = 0; i < VANGO_PAGE_COUNT; i++) {
        if (!s_page_built[i] || !s_tiles[i]) {
            continue;
        }
        dash_clear_page_state(i);
        lv_obj_clean(s_tiles[i]);
        s_page_built[i] = false;
    }
    ensure_page_built(s_page);
    if (s_page + 1 < VANGO_PAGE_COUNT) {
        ensure_page_built((vango_page_t)(s_page + 1));
    }
    if (s_page > 0) {
        ensure_page_built((vango_page_t)(s_page - 1));
    }
    if (was_settings) {
        ensure_page_built(VANGO_PAGE_SETTINGS);
    }

    for (int i = 0; i < VANGO_PAGE_COUNT; i++) {
        if (s_nav_lbls[i]) {
            lv_label_set_text(s_nav_lbls[i], nav_label(i));
        }
    }

    apply_page_chrome(s_page);

    /* Re-apply live telemetry after rebuild */
    vango_dash_set_battery_v(s_bat_v);
    vango_dash_set_temp_c(s_temp_c);
    vango_dash_set_water(s_w_clean, s_w_waste, s_w_grey);
    /* Theme rebuild kartlari sifirdan kurar — IO isim/ikonlari tekrar bas */
    vango_dash_refresh_channel_meta();
}
