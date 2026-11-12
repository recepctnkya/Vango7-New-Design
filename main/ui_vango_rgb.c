#include "ui_vango_rgb.h"
#include "ui_vango_theme.h"
#include "ui_vango_i18n.h"
#include "ui_vango_dash.h"
#include "hexnet_canbus.h"
#include "hexnet_io_profile.h"
#include "fonts/vango_fonts.h"
#include "icons/vango_panel_icons.h"

#include "esp_log.h"
#include <stdio.h>
#include <string.h>

#define TAG "VANGO_RGB"

/* Yerel durum — kapaninca parlaklik sifirlanmaz */
static uint8_t s_r = 0;
static uint8_t s_g = 120;
static uint8_t s_b = 255;
static uint8_t s_bri = 78; /* 0..100 */
static bool s_on;
static int s_effect; /* 0=sabit — firmware baska efekt desteklemiyor */

static lv_obj_t *s_root;
static lv_obj_t *s_wheel;
static lv_obj_t *s_pwr;
static lv_obj_t *s_pwr_lbl;
static lv_obj_t *s_bri_slider;
static lv_obj_t *s_bri_val;
static lv_obj_t *s_spd_slider;
static lv_obj_t *s_spd_val;
static lv_obj_t *s_effect_lbl;
static lv_obj_t *s_disabled_msg;
static lv_obj_t *s_preset[8];

static const uint32_t s_preset_rgb[8] = {
    0xEF4444u, 0xF97316u, 0xEAB308u, 0x22C55Eu,
    0x06B6D4u, 0x3B82F6u, 0xA855F7u, 0xEC4899u
};

extern uint8_t rgbEna;

static lv_obj_t *make_label(lv_obj_t *parent, const char *txt, const lv_font_t *font, uint32_t color)
{
    lv_obj_t *l = lv_label_create(parent);
    lv_label_set_text(l, txt);
    lv_obj_set_style_text_font(l, font, 0);
    lv_obj_set_style_text_color(l, lv_color_hex(color), 0);
    return l;
}

static void style_card(lv_obj_t *obj)
{
    lv_obj_set_style_bg_color(obj, lv_color_hex(COL_ACCENT_CARD), 0);
    lv_obj_set_style_bg_opa(obj, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(obj, 14, 0);
    lv_obj_set_style_border_width(obj, 1, 0);
    lv_obj_set_style_border_color(obj, lv_color_hex(COL_ACCENT_DIM), 0);
    lv_obj_set_style_pad_all(obj, 0, 0);
    lv_obj_set_style_shadow_width(obj, 0, 0);
    lv_obj_clear_flag(obj, LV_OBJ_FLAG_SCROLLABLE);
}

static bool rgb_channel_enabled(void)
{
    const hexnet_io_profile_t *p = hexnet_io_profile_get();
    return p && p->rgb_enabled;
}

static void rgb_apply_scaled(uint8_t *or_, uint8_t *og, uint8_t *ob)
{
    uint32_t bri = s_bri;
    if (bri > 100) {
        bri = 100;
    }
    *or_ = (uint8_t)((uint32_t)s_r * bri / 100u);
    *og = (uint8_t)((uint32_t)s_g * bri / 100u);
    *ob = (uint8_t)((uint32_t)s_b * bri / 100u);
}

static void rgb_tx(void)
{
    uint8_t data[8] = {0};
    uint8_t rr, gg, bb;
    rgb_apply_scaled(&rr, &gg, &bb);
    data[0] = rr;
    data[1] = gg;
    data[2] = bb;
    data[3] = s_on ? 1u : 0u;
    rgbEna = data[3];
    send_can_frame(0x740, data);
    ESP_LOGI(TAG, "TX 0x740 R=%u G=%u B=%u ena=%u (base %u,%u,%u bri=%u)",
             rr, gg, bb, data[3], s_r, s_g, s_b, s_bri);
}

static void sync_wheel_from_state(void)
{
    if (s_wheel) {
        lv_colorwheel_set_rgb(s_wheel, lv_color_make(s_r, s_g, s_b));
    }
}

static void sync_pwr_ui(void)
{
    if (!s_pwr) {
        return;
    }
    lv_obj_set_style_bg_color(s_pwr, lv_color_hex(s_on ? COL_ACCENT : COL_BTN_IDLE), 0);
    if (s_pwr_lbl) {
        lv_label_set_text(s_pwr_lbl,
                          s_on ? vango_tx("On", "Açık", "An")
                               : vango_tx("Off", "Kapalı", "Aus"));
        lv_obj_set_style_text_color(s_pwr_lbl, lv_color_hex(COL_TEXT), 0);
    }
}

static void sync_bri_ui(void)
{
    char buf[12];
    if (s_bri_slider) {
        lv_slider_set_value(s_bri_slider, s_bri, LV_ANIM_OFF);
    }
    if (s_bri_val) {
        snprintf(buf, sizeof(buf), "%u%%", (unsigned)s_bri);
        lv_label_set_text(s_bri_val, buf);
    }
}

static void set_controls_enabled(bool en)
{
    if (s_wheel) {
        if (en) {
            lv_obj_clear_state(s_wheel, LV_STATE_DISABLED);
        } else {
            lv_obj_add_state(s_wheel, LV_STATE_DISABLED);
        }
    }
    if (s_pwr) {
        if (en) {
            lv_obj_clear_state(s_pwr, LV_STATE_DISABLED);
        } else {
            lv_obj_add_state(s_pwr, LV_STATE_DISABLED);
        }
    }
    if (s_bri_slider) {
        if (en) {
            lv_obj_clear_state(s_bri_slider, LV_STATE_DISABLED);
        } else {
            lv_obj_add_state(s_bri_slider, LV_STATE_DISABLED);
        }
    }
    for (int i = 0; i < 8; i++) {
        if (!s_preset[i]) {
            continue;
        }
        if (en) {
            lv_obj_clear_state(s_preset[i], LV_STATE_DISABLED);
        } else {
            lv_obj_add_state(s_preset[i], LV_STATE_DISABLED);
        }
    }
    if (s_disabled_msg) {
        if (en) {
            lv_obj_add_flag(s_disabled_msg, LV_OBJ_FLAG_HIDDEN);
        } else {
            lv_obj_clear_flag(s_disabled_msg, LV_OBJ_FLAG_HIDDEN);
        }
    }
}

static void wheel_cb(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    if (code != LV_EVENT_VALUE_CHANGED && code != LV_EVENT_RELEASED) {
        return;
    }
    if (!rgb_channel_enabled()) {
        return;
    }
    lv_color_t c = lv_colorwheel_get_rgb(s_wheel);
    lv_color32_t c32;
    c32.full = lv_color_to32(c);
    s_r = c32.ch.red;
    s_g = c32.ch.green;
    s_b = c32.ch.blue;
    if (code == LV_EVENT_RELEASED || s_on) {
        if (!s_on) {
            s_on = true;
            sync_pwr_ui();
        }
        rgb_tx();
    }
}

static void pwr_cb(lv_event_t *e)
{
    if (lv_event_get_code(e) != LV_EVENT_CLICKED) {
        return;
    }
    if (!rgb_channel_enabled()) {
        return;
    }
    s_on = !s_on;
    sync_pwr_ui();
    rgb_tx();
}

static void bri_cb(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    if (code != LV_EVENT_VALUE_CHANGED && code != LV_EVENT_RELEASED) {
        return;
    }
    if (!s_bri_slider || !rgb_channel_enabled()) {
        return;
    }
    s_bri = (uint8_t)lv_slider_get_value(s_bri_slider);
    sync_bri_ui();
    if (code == LV_EVENT_RELEASED || s_on) {
        if (!s_on && code == LV_EVENT_RELEASED) {
            /* parlaklik degisti — acik degilse sadece sakla */
            return;
        }
        if (s_on) {
            rgb_tx();
        }
    }
}

static void preset_cb(lv_event_t *e)
{
    if (lv_event_get_code(e) != LV_EVENT_CLICKED) {
        return;
    }
    if (!rgb_channel_enabled()) {
        return;
    }
    int idx = (int)(intptr_t)lv_event_get_user_data(e);
    if (idx < 0 || idx >= 8) {
        return;
    }
    uint32_t rgb = s_preset_rgb[idx];
    s_r = (uint8_t)((rgb >> 16) & 0xFFu);
    s_g = (uint8_t)((rgb >> 8) & 0xFFu);
    s_b = (uint8_t)(rgb & 0xFFu);
    s_on = true;
    sync_wheel_from_state();
    sync_pwr_ui();
    rgb_tx();
}

static lv_obj_t *add_mid_card(lv_obj_t *parent, int y, int w, int h, const char *title)
{
    lv_obj_t *card = lv_obj_create(parent);
    lv_obj_set_size(card, w, h);
    lv_obj_set_pos(card, 0, y);
    style_card(card);
    make_label(card, title, &vango_font_14, COL_MUTED);
    lv_obj_set_pos(lv_obj_get_child(card, 0), 12, 8);
    return card;
}

void vango_rgb_build(lv_obj_t *parent)
{
    const int W = 800;
    const int H = VANGO_DASH_TILE_H;
    const int pad = 8;
    const int left_w = 270;
    const int mid_w = 250;
    const int right_w = W - left_w - mid_w - pad * 4;
    int i;

    s_root = parent;
    s_wheel = s_pwr = s_pwr_lbl = NULL;
    s_bri_slider = s_bri_val = s_spd_slider = s_spd_val = NULL;
    s_effect_lbl = s_disabled_msg = NULL;
    memset(s_preset, 0, sizeof(s_preset));

    lv_obj_set_style_bg_color(parent, lv_color_hex(COL_BG), 0);
    lv_obj_set_style_bg_opa(parent, LV_OPA_COVER, 0);
    lv_obj_clear_flag(parent, LV_OBJ_FLAG_SCROLLABLE);

    /* ---- Sol: baslik + colorwheel + power ---- */
    lv_obj_t *left = lv_obj_create(parent);
    lv_obj_set_size(left, left_w, H - pad);
    lv_obj_set_pos(left, pad, 4);
    lv_obj_set_style_bg_opa(left, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(left, 0, 0);
    lv_obj_set_style_pad_all(left, 0, 0);
    lv_obj_clear_flag(left, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *hdr_ico = lv_img_create(left);
    lv_img_set_src(hdr_ico, &vango_panel_rgb_aydinlatma);
    lv_img_set_zoom(hdr_ico, 160);
    lv_obj_set_style_img_recolor(hdr_ico, lv_color_hex(COL_ACCENT), 0);
    lv_obj_set_style_img_recolor_opa(hdr_ico, LV_OPA_COVER, 0);
    lv_obj_set_pos(hdr_ico, 4, 0);

    make_label(left, vango_tx("RGB Lighting", "RGB Aydınlatma", "RGB Beleuchtung"),
               &vango_font_16, COL_TEXT);
    lv_obj_set_pos(lv_obj_get_child(left, lv_obj_get_child_cnt(left) - 1), 44, 10);

    const int wheel = 200;
    s_wheel = lv_colorwheel_create(left, true);
    lv_obj_set_size(s_wheel, wheel, wheel);
    lv_obj_set_pos(s_wheel, (left_w - wheel) / 2, 44);
    lv_colorwheel_set_mode(s_wheel, LV_COLORWHEEL_MODE_HUE);
    lv_colorwheel_set_mode_fixed(s_wheel, true);
    lv_obj_add_event_cb(s_wheel, wheel_cb, LV_EVENT_VALUE_CHANGED, NULL);
    lv_obj_add_event_cb(s_wheel, wheel_cb, LV_EVENT_RELEASED, NULL);

    s_pwr = lv_btn_create(left);
    lv_obj_set_size(s_pwr, 70, 70);
    lv_obj_set_pos(s_pwr, (left_w - 70) / 2, 44 + (wheel - 70) / 2);
    lv_obj_set_style_radius(s_pwr, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_shadow_width(s_pwr, 0, 0);
    lv_obj_set_style_border_width(s_pwr, 2, 0);
    lv_obj_set_style_border_color(s_pwr, lv_color_hex(COL_CARD_BORDER), 0);
    lv_obj_add_event_cb(s_pwr, pwr_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_t *pwr_sym = make_label(s_pwr, LV_SYMBOL_POWER, &lv_font_montserrat_20, COL_TEXT);
    lv_obj_align(pwr_sym, LV_ALIGN_CENTER, 0, -8);
    s_pwr_lbl = make_label(s_pwr, "", &vango_font_12, COL_TEXT);
    lv_obj_align(s_pwr_lbl, LV_ALIGN_CENTER, 0, 14);

    /* ---- Orta: parlaklik / hiz / efekt ---- */
    lv_obj_t *mid = lv_obj_create(parent);
    lv_obj_set_size(mid, mid_w, H - pad);
    lv_obj_set_pos(mid, pad * 2 + left_w, 4);
    lv_obj_set_style_bg_opa(mid, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(mid, 0, 0);
    lv_obj_set_style_pad_all(mid, 0, 0);
    lv_obj_clear_flag(mid, LV_OBJ_FLAG_SCROLLABLE);

    const int card_h = 92;
    const int card_gap = 8;
    lv_obj_t *c1 = add_mid_card(mid, 8, mid_w, card_h,
                                vango_tx("Brightness", "Parlaklık", "Helligkeit"));
    s_bri_val = make_label(c1, "78%", &vango_font_20, COL_TEXT);
    lv_obj_align(s_bri_val, LV_ALIGN_TOP_RIGHT, -12, 6);
    s_bri_slider = lv_slider_create(c1);
    lv_obj_set_size(s_bri_slider, mid_w - 28, 18);
    lv_obj_align(s_bri_slider, LV_ALIGN_BOTTOM_MID, 0, -16);
    lv_slider_set_range(s_bri_slider, 0, 100);
    lv_obj_set_style_bg_color(s_bri_slider, lv_color_hex(COL_ACCENT_DIM), LV_PART_MAIN);
    lv_obj_set_style_bg_color(s_bri_slider, lv_color_hex(COL_ACCENT), LV_PART_INDICATOR);
    lv_obj_set_style_bg_color(s_bri_slider, lv_color_hex(COL_TEXT), LV_PART_KNOB);
    lv_obj_add_event_cb(s_bri_slider, bri_cb, LV_EVENT_VALUE_CHANGED, NULL);
    lv_obj_add_event_cb(s_bri_slider, bri_cb, LV_EVENT_RELEASED, NULL);

    lv_obj_t *c2 = add_mid_card(mid, 8 + card_h + card_gap, mid_w, card_h,
                                vango_tx("Speed (Effect)", "Hız (Efekt)", "Tempo (Effekt)"));
    s_spd_val = make_label(c2, vango_tx("Medium", "Orta", "Mittel"), &vango_font_16, COL_MUTED);
    lv_obj_align(s_spd_val, LV_ALIGN_TOP_RIGHT, -12, 8);
    s_spd_slider = lv_slider_create(c2);
    lv_obj_set_size(s_spd_slider, mid_w - 28, 18);
    lv_obj_align(s_spd_slider, LV_ALIGN_BOTTOM_MID, 0, -16);
    lv_slider_set_range(s_spd_slider, 0, 2);
    lv_slider_set_value(s_spd_slider, 1, LV_ANIM_OFF);
    lv_obj_set_style_bg_color(s_spd_slider, lv_color_hex(COL_ACCENT_DIM), LV_PART_MAIN);
    lv_obj_set_style_bg_color(s_spd_slider, lv_color_hex(COL_ACCENT_DIM), LV_PART_INDICATOR);
    lv_obj_set_style_bg_color(s_spd_slider, lv_color_hex(COL_MUTED), LV_PART_KNOB);
    /* Firmware efekt hizi yok — sabit renk modunda pasif */
    lv_obj_add_state(s_spd_slider, LV_STATE_DISABLED);
    lv_obj_set_style_opa(s_spd_slider, LV_OPA_40, 0);
    lv_obj_set_style_opa(s_spd_val, LV_OPA_50, 0);

    lv_obj_t *c3 = add_mid_card(mid, 8 + 2 * (card_h + card_gap), mid_w, card_h,
                                vango_tx("Effect Mode", "Efekt Modu", "Effektmodus"));
    s_effect_lbl = make_label(c3,
                              vango_tx("Solid Color", "Sabit Renk", "Einfarbig"),
                              &vango_font_16, COL_TEXT);
    lv_obj_align(s_effect_lbl, LV_ALIGN_LEFT_MID, 12, 8);
    make_label(c3,
               vango_tx("(only mode supported)", "(desteklenen tek mod)", "(einziger Modus)"),
               &vango_font_12, COL_MUTED);
    lv_obj_align(lv_obj_get_child(c3, lv_obj_get_child_cnt(c3) - 1), LV_ALIGN_BOTTOM_LEFT, 12, -10);
    (void)s_effect;

    /* ---- Sag: hizli renkler ---- */
    lv_obj_t *right = lv_obj_create(parent);
    lv_obj_set_size(right, right_w, H - pad);
    lv_obj_set_pos(right, pad * 3 + left_w + mid_w, 4);
    lv_obj_set_style_bg_opa(right, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(right, 0, 0);
    lv_obj_set_style_pad_all(right, 0, 0);
    lv_obj_clear_flag(right, LV_OBJ_FLAG_SCROLLABLE);

    make_label(right, vango_tx("Quick Colors", "Hızlı Renkler", "Schnellfarben"),
               &vango_font_14, COL_MUTED);
    lv_obj_set_pos(lv_obj_get_child(right, 0), 4, 8);

    const int cell = 48;
    const int gap = 8;
    const int grid_w = 4 * cell + 3 * gap;
    const int ox = (right_w - grid_w) / 2;
    for (i = 0; i < 8; i++) {
        int col = i % 4;
        int row = i / 4;
        lv_obj_t *p = lv_obj_create(right);
        s_preset[i] = p;
        lv_obj_set_size(p, cell, cell);
        lv_obj_set_pos(p, ox + col * (cell + gap), 40 + row * (cell + gap));
        lv_obj_set_style_radius(p, 12, 0);
        lv_obj_set_style_bg_color(p, lv_color_hex(s_preset_rgb[i]), 0);
        lv_obj_set_style_bg_opa(p, LV_OPA_COVER, 0);
        lv_obj_set_style_border_width(p, 2, 0);
        lv_obj_set_style_border_color(p, lv_color_hex(COL_CARD_BORDER), 0);
        lv_obj_set_style_pad_all(p, 0, 0);
        lv_obj_set_style_shadow_width(p, 0, 0);
        lv_obj_clear_flag(p, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_add_flag(p, LV_OBJ_FLAG_CLICKABLE);
        lv_obj_add_event_cb(p, preset_cb, LV_EVENT_CLICKED, (void *)(intptr_t)i);
    }

    /* Desteklenmeyen efekt listesi (bilgi — tiklanmaz) */
    make_label(right,
               vango_tx("Effects on IO: Solid only",
                        "IO efekt: yalnızca Sabit",
                        "IO-Effekt: nur Einfarbig"),
               &vango_font_12, COL_MUTED);
    lv_obj_set_pos(lv_obj_get_child(right, lv_obj_get_child_cnt(right) - 1), 4, 40 + 2 * (cell + gap) + 8);

    s_disabled_msg = make_label(parent,
                                vango_tx("RGB disabled in Panel Settings",
                                         "RGB Panel Ayarlarında kapalı",
                                         "RGB in Paneleinstellungen aus"),
                                &vango_font_14, COL_ORANGE);
    lv_obj_align(s_disabled_msg, LV_ALIGN_BOTTOM_MID, 0, -8);
    lv_obj_add_flag(s_disabled_msg, LV_OBJ_FLAG_HIDDEN);

    /* Baslangic: IO echo veya varsayilan */
    if (get_canbus_connection_status()) {
        uint8_t er = get_rgb_value(0);
        uint8_t eg = get_rgb_value(1);
        uint8_t eb = get_rgb_value(2);
        uint8_t ee = get_rgb_value(3);
        if (er || eg || eb) {
            /* echo parlaklikli olabilir — full olarak sakla */
            s_r = er;
            s_g = eg;
            s_b = eb;
            s_bri = 100;
        }
        s_on = (ee != 0);
    } else {
        s_on = (rgbEna != 0);
    }

    sync_wheel_from_state();
    sync_pwr_ui();
    sync_bri_ui();
    set_controls_enabled(rgb_channel_enabled());
}

void vango_rgb_refresh(void)
{
    if (!s_root) {
        return;
    }
    set_controls_enabled(rgb_channel_enabled());
    sync_wheel_from_state();
    sync_pwr_ui();
    sync_bri_ui();
}
