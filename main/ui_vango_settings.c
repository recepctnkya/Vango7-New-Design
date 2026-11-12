#include "ui_vango_settings.h"
#include "ui_vango_theme.h"
#include "ui_vango_i18n.h"
#include "ui_vango_panel_catalog.h"
#include "fonts/vango_fonts.h"
#include "icons/vango_icons.h"
#include "icons/vango_panel_icons.h"
#include "display_manager.h"
#include "hexnet_io_profile.h"
#include "hexnet_io_labels.h"
#include "hexnet_icon_map.h"
#include "ui.h"
#include "ui_vango_dash.h"

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "esp_log.h"
#include "hexnet_canbus.h"
#include "hexnet_version.h"
#include "panel_pinout.h"

#define TAG "VANGO_SET"

/* Tile is 800 x VANGO_DASH_TILE_H. Sidebar is ~23% of the screen. */
#define SET_W      800
#define SET_H      VANGO_DASH_TILE_H
#define SIDE_W     184
#define CONT_W     (SET_W - SIDE_W)
#define PAGE_H     SET_H
#define SET_TOP    16
#define SET_GAP    16
#define SET_RIGHT  14

#define SET_BG_DARK      0x06121Du
#define SET_SIDE_DARK    0x0C1D2Bu
#define SET_CARD_DARK    0x0D2636u
#define SET_TEXT_DARK    0xFFFFFFu
#define SET_MUTED_DARK   0xB8C6D2u

typedef enum {
    SET_SEC_PANEL = 0,
    SET_SEC_SCREEN,
    SET_SEC_THEME,
    SET_SEC_LANG,
    SET_SEC_DEVICE,
    SET_SEC_DEMO,
    SET_SEC_COUNT
} set_sec_t;

/* Overview 2×3:
 *  Buton | Aydınlatma | Motorlar
 *  Sensörler | Su seviye | RGB Aydınlatma
 * (eski Aydınlatma+Röle+Dimmer ayrimi kaldirildi — dimmer = aydinlatma) */
typedef enum {
    PANEL_CAT_BUTTON = 0, /* on/off role butonlari */
    PANEL_CAT_LIGHT,      /* dimmer kanallari */
    PANEL_CAT_MOTOR,
    PANEL_CAT_SENSOR,
    PANEL_CAT_WATER,
    PANEL_CAT_RGB,
    PANEL_CAT_COUNT
} panel_cat_t;

#define PANEL_BUTTON_SLOTS 12
#define PANEL_LIGHT_SLOTS  HEXNET_IO_DIM_SLOTS
#define PANEL_MOTOR_SLOTS  HEXNET_IO_MOTOR_SLOTS
#define PANEL_SENSOR_SLOTS HEXNET_IO_SENSOR_SLOTS
#define PANEL_WATER_SLOTS  3 /* Temiz / Atık / Gri — yedek slot UI'da yok */

static lv_obj_t *s_root;
static lv_obj_t *s_side_btns[SET_SEC_COUNT];
static lv_obj_t *s_side_lbls[SET_SEC_COUNT];
static lv_obj_t *s_side_glyphs[SET_SEC_COUNT];
static lv_obj_t *s_content;
static lv_obj_t *s_picker;
static lv_obj_t *s_liter_ta;
static int s_liter_idx;
static set_sec_t s_sec = SET_SEC_PANEL;
static panel_cat_t s_panel_detail = PANEL_CAT_COUNT; /* COUNT = overview */

static void show_section(set_sec_t sec);
static void panel_counts(panel_cat_t cat, int *active, int *total);

/* Butonlar sayfasi — switch event icinde destroy etmemek icin yerinde boya */
static lv_obj_t *s_btn_title;
static lv_obj_t *s_btn_card[PANEL_BUTTON_SLOTS];
static lv_obj_t *s_btn_ico[PANEL_BUTTON_SLOTS];
static lv_obj_t *s_btn_nm[PANEL_BUTTON_SLOTS];
static lv_obj_t *s_btn_rl[PANEL_BUTTON_SLOTS];
static lv_obj_t *s_relay_menu; /* secim listesi overlay */
static int s_relay_menu_slot = -1;
/** Ikon secimi sonrasi Kaydet/Iptal onay */
static lv_obj_t *s_icon_confirm;
static int s_icon_pending_slot = -1;
static char s_icon_pending_new_key[HEXNET_LABEL_ICON_MAX + 1];
static char s_icon_pending_old_icon[HEXNET_LABEL_ICON_MAX + 1];
static char s_icon_pending_old_name[HEXNET_LABEL_NAME_MAX + 1];
static int s_icon_pending_old_type = 1;

/* Diger panel detay kartlari */
static lv_obj_t *s_cat_title;
static lv_obj_t *s_light_card[PANEL_LIGHT_SLOTS];
static lv_obj_t *s_light_ico[PANEL_LIGHT_SLOTS];
static lv_obj_t *s_light_nm[PANEL_LIGHT_SLOTS];
static lv_obj_t *s_light_sub[PANEL_LIGHT_SLOTS];
static lv_obj_t *s_motor_card[PANEL_MOTOR_SLOTS];
static lv_obj_t *s_motor_ico[PANEL_MOTOR_SLOTS];
static lv_obj_t *s_motor_nm[PANEL_MOTOR_SLOTS];
static lv_obj_t *s_sensor_card[PANEL_SENSOR_SLOTS];
static lv_obj_t *s_sensor_ico[PANEL_SENSOR_SLOTS];
static lv_obj_t *s_sensor_nm[PANEL_SENSOR_SLOTS];
static lv_obj_t *s_water_card[PANEL_WATER_SLOTS];
static lv_obj_t *s_water_ico[PANEL_WATER_SLOTS];
static lv_obj_t *s_water_nm[PANEL_WATER_SLOTS];
static lv_obj_t *s_water_cap[PANEL_WATER_SLOTS];

static lv_obj_t *s_accent_sw[VANGO_ACCENT_N];
static lv_obj_t *s_theme_mode_card[3];

static void panel_back_cb(lv_event_t *e);
static void relay_menu_close(void);
static void open_relay_type_menu(int slot);
static void panel_button_paint_meta(int slot);
static void icon_confirm_close(bool revert);
static void icon_confirm_show(int slot, const char *new_key);
static void relay_icon_apply_local(int slot, const char *key, bool push_can);
static const char *relay_type_name(int type);
static const void *panel_button_icon(int slot);
static void build_panel_lights_page(void);
static void build_panel_motors_page(void);
static void build_panel_sensors_page(void);
static void build_panel_water_page(void);
static void water_refresh_cap_labels(void);
static lv_obj_t *panel_detail_header(panel_cat_t cat, const char *en, const char *tr, const char *de);
static void panel_card_set_active(lv_obj_t *card, lv_obj_t *ico, lv_obj_t *nm, lv_obj_t *sub, bool on);
static const lv_img_dsc_t *dim_type_icon(int type);
static const lv_img_dsc_t *sensor_slot_icon(int idx);
static const lv_img_dsc_t *water_slot_icon(int idx);

extern int panelThemeType;
extern int panelLanguageType;
extern int panelAccentIdx;
extern int panelWallpaperEnable;
extern int panelWallpaperTime;

static uint32_t set_bg(void)
{
    return COL_BG;
}

static uint32_t set_side_bg(void)
{
    return vango_theme_dark() ? COL_ACCENT_CARD : COL_SIDE;
}

static uint32_t set_card_bg(void)
{
    return vango_theme_dark() ? COL_ACCENT_CARD : COL_CARD;
}

static uint32_t set_card_border(void)
{
    return vango_theme_dark() ? COL_ACCENT_DIM : COL_CARD_BORDER;
}

static uint32_t set_sel_bg(void)
{
    return COL_SIDE_SEL;
}

static uint32_t set_accent(void)
{
    return COL_ACCENT;
}

static uint32_t set_text(void)
{
    return vango_theme_dark() ? SET_TEXT_DARK : COL_TEXT;
}

static uint32_t set_muted(void)
{
    return vango_theme_dark() ? SET_MUTED_DARK : COL_MUTED;
}

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
    lv_obj_set_style_bg_color(obj, lv_color_hex(set_card_bg()), 0);
    lv_obj_set_style_bg_opa(obj, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(obj, 14, 0);
    lv_obj_set_style_border_width(obj, 1, 0);
    lv_obj_set_style_border_color(obj, lv_color_hex(set_card_border()), 0);
    lv_obj_set_style_pad_all(obj, 0, 0);
    lv_obj_set_style_shadow_width(obj, 0, 0);
    lv_obj_clear_flag(obj, LV_OBJ_FLAG_SCROLLABLE);
}

static void glyph_set_color(lv_obj_t *obj, uint32_t color)
{
    lv_color_t c = lv_color_hex(color);
    uint32_t n;
    uint32_t i;

    if (!obj) {
        return;
    }
    if (lv_obj_check_type(obj, &lv_img_class)) {
        lv_obj_set_style_img_recolor(obj, c, 0);
        lv_obj_set_style_img_recolor_opa(obj, LV_OPA_COVER, 0);
        return;
    }
    if (lv_obj_check_type(obj, &lv_label_class)) {
        lv_obj_set_style_text_color(obj, c, 0);
        return;
    }
    if (lv_obj_get_style_border_width(obj, 0) > 0) {
        lv_obj_set_style_border_color(obj, c, 0);
    }
    if (lv_obj_get_style_bg_opa(obj, 0) > LV_OPA_TRANSP) {
        lv_obj_set_style_bg_color(obj, c, 0);
    }
    n = lv_obj_get_child_cnt(obj);
    for (i = 0; i < n; i++) {
        glyph_set_color(lv_obj_get_child(obj, i), color);
    }
}

static void clear_content(void)
{
    s_picker = NULL;
    s_liter_ta = NULL;
    s_btn_title = NULL;
    s_cat_title = NULL;
    s_relay_menu = NULL;
    s_relay_menu_slot = -1;
    if (s_icon_pending_slot >= 0) {
        /* Sayfa yenilenmeden once yerel onizlemeyi geri al */
        icon_confirm_close(true);
    } else {
        s_icon_confirm = NULL;
    }
    memset(s_btn_card, 0, sizeof(s_btn_card));
    memset(s_btn_ico, 0, sizeof(s_btn_ico));
    memset(s_btn_nm, 0, sizeof(s_btn_nm));
    memset(s_btn_rl, 0, sizeof(s_btn_rl));
    memset(s_light_card, 0, sizeof(s_light_card));
    memset(s_light_ico, 0, sizeof(s_light_ico));
    memset(s_light_nm, 0, sizeof(s_light_nm));
    memset(s_light_sub, 0, sizeof(s_light_sub));
    memset(s_motor_card, 0, sizeof(s_motor_card));
    memset(s_motor_ico, 0, sizeof(s_motor_ico));
    memset(s_motor_nm, 0, sizeof(s_motor_nm));
    memset(s_sensor_card, 0, sizeof(s_sensor_card));
    memset(s_sensor_ico, 0, sizeof(s_sensor_ico));
    memset(s_sensor_nm, 0, sizeof(s_sensor_nm));
    memset(s_water_card, 0, sizeof(s_water_card));
    memset(s_water_ico, 0, sizeof(s_water_ico));
    memset(s_water_nm, 0, sizeof(s_water_nm));
    memset(s_water_cap, 0, sizeof(s_water_cap));
    memset(s_accent_sw, 0, sizeof(s_accent_sw));
    memset(s_theme_mode_card, 0, sizeof(s_theme_mode_card));
    if (s_content) {
        lv_obj_clean(s_content);
    }
}

static void panel_section_refresh_async(void *user)
{
    (void)user;
    if (s_content) {
        show_section(SET_SEC_PANEL);
    }
}

/** ALPHA_8BIT: zoom 256 zorunlu (degilse bos cizer). target_px bilgi amacli. */
static void panel_button_apply_icon(lv_obj_t *ico, const lv_img_dsc_t *src, uint16_t target_px)
{
    (void)target_px;
    if (!ico || !src) {
        return;
    }
    lv_img_set_src(ico, src);
    lv_img_set_size_mode(ico, LV_IMG_SIZE_MODE_VIRTUAL);
    lv_img_set_zoom(ico, 256);
    lv_obj_set_style_transform_zoom(ico, LV_IMG_ZOOM_NONE, 0);
}

static lv_obj_t *panel_detail_header(panel_cat_t cat, const char *en, const char *tr, const char *de)
{
    int active = 0;
    int total = 1;
    char hdr[48];
    lv_obj_t *back;
    lv_obj_t *bl;
    lv_obj_t *title;

    back = lv_btn_create(s_content);
    lv_obj_set_size(back, 32, 26);
    lv_obj_set_pos(back, 2, 2);
    lv_obj_set_style_bg_opa(back, LV_OPA_TRANSP, 0);
    lv_obj_set_style_shadow_width(back, 0, 0);
    bl = make_label(back, LV_SYMBOL_LEFT, &lv_font_montserrat_16, set_text());
    lv_obj_center(bl);
    lv_obj_add_event_cb(back, panel_back_cb, LV_EVENT_CLICKED, NULL);

    panel_counts(cat, &active, &total);
    snprintf(hdr, sizeof(hdr), "%s (%d/%d)", vango_tx(en, tr, de), active, total);
    title = make_label(s_content, hdr, &vango_font_16, set_accent());
    lv_obj_set_pos(title, 36, 6);
    s_cat_title = title;
    return title;
}

static void panel_card_set_active(lv_obj_t *card, lv_obj_t *ico, lv_obj_t *nm, lv_obj_t *sub, bool on)
{
    uint32_t fg = on ? set_text() : set_muted();
    uint32_t ico_col = on ? set_accent() : set_muted();
    lv_opa_t ico_opa = on ? LV_OPA_COVER : LV_OPA_50;

    if (card) {
        lv_obj_set_style_border_color(card, lv_color_hex(on ? set_accent() : set_card_bg()), 0);
        lv_obj_set_style_border_width(card, on ? 2 : 0, 0);
        lv_obj_set_style_border_opa(card, on ? LV_OPA_COVER : LV_OPA_TRANSP, 0);
        lv_obj_set_style_opa(card, on ? LV_OPA_COVER : LV_OPA_70, 0);
    }
    if (ico) {
        lv_obj_set_style_img_recolor(ico, lv_color_hex(ico_col), 0);
        lv_obj_set_style_img_recolor_opa(ico, ico_opa, 0);
    }
    if (nm) {
        lv_obj_set_style_text_color(nm, lv_color_hex(fg), 0);
        lv_obj_set_style_opa(nm, on ? LV_OPA_COVER : LV_OPA_60, 0);
    }
    if (sub) {
        lv_obj_set_style_text_color(sub, lv_color_hex(on ? set_text() : set_muted()), 0);
        lv_obj_set_style_opa(sub, on ? LV_OPA_COVER : LV_OPA_60, 0);
    }
}

static void panel_cat_title_refresh(panel_cat_t cat, const char *en, const char *tr, const char *de)
{
    int active = 0;
    int total = 1;
    char hdr[48];
    if (!s_cat_title) {
        return;
    }
    panel_counts(cat, &active, &total);
    snprintf(hdr, sizeof(hdr), "%s (%d/%d)", vango_tx(en, tr, de), active, total);
    lv_label_set_text(s_cat_title, hdr);
}

static const lv_img_dsc_t *dim_type_icon(int type)
{
    static const char *keys[8] = {
        "tavan_aydinlatma", "banyo_aydinlatma", "mutfak_aydinlatma", "yatak_basi",
        "giris_aydinlatma", "merdiven_aydinlatma", "dis_aydinlatma", "spot_aydinlatma"
    };
    const lv_img_dsc_t *img;
    if (type < 1 || type > 8) {
        type = 1;
    }
    img = vango_panel_icon_sm_by_key(keys[type - 1]);
    return img ? img : &vango_panel_tavan_aydinlatma_sm;
}

static const lv_img_dsc_t *sensor_slot_icon(int idx)
{
    static const char *keys[5] = {
        "sicaklik", "nem", "hareket_sensoru", "gaz_dedektoru", "duman_dedektoru"
    };
    const lv_img_dsc_t *img;
    if (idx < 0 || idx >= 5) {
        idx = 0;
    }
    img = vango_panel_icon_sm_by_key(keys[idx]);
    return img ? img : &vango_panel_sicaklik_sm;
}

static const lv_img_dsc_t *water_slot_icon(int idx)
{
    static const char *keys[3] = { "temiz_su_tanki", "pis_su_tanki", "gri_su_tanki" };
    const lv_img_dsc_t *img;
    if (idx < 0 || idx >= 3) {
        idx = 0;
    }
    img = vango_panel_icon_sm_by_key(keys[idx]);
    return img ? img : &vango_panel_temiz_su_tanki_sm;
}

static void panel_button_paint_meta(int slot)
{
    const hexnet_io_labels_t *L = hexnet_io_labels_get();
    const hexnet_io_profile_t *p = hexnet_io_profile_get();
    const lv_img_dsc_t *src;
    char name[32];
    int type = 1;

    if (slot < 0 || slot >= PANEL_BUTTON_SLOTS) {
        return;
    }
    if (p && p->relay_type[slot]) {
        type = (int)p->relay_type[slot];
    }
    if (L && slot < HEXNET_LABEL_RELAYS && L->relays[slot].name[0]) {
        snprintf(name, sizeof(name), "%s", L->relays[slot].name);
    } else {
        snprintf(name, sizeof(name), "%s", relay_type_name(type));
    }
    src = (const lv_img_dsc_t *)panel_button_icon(slot);
    if (s_btn_ico[slot] && src) {
        lv_obj_t *par = lv_obj_get_parent(s_btn_ico[slot]);
        int box = 40;
        if (par) {
            int bw = (int)lv_obj_get_width(par);
            int bh = (int)lv_obj_get_height(par);
            box = bw < bh ? bw : bh;
            box -= 6;
        }
        if (box > 48) {
            box = 48;
        }
        if (box < 28) {
            box = 28;
        }
        panel_button_apply_icon(s_btn_ico[slot], src, (uint16_t)box);
        lv_obj_center(s_btn_ico[slot]);
    }
    if (s_btn_nm[slot]) {
        lv_label_set_text(s_btn_nm[slot], name);
    }
}

static void panel_button_paint_active(int slot, bool on)
{
    uint32_t fg = on ? set_text() : set_muted();
    uint32_t ico_col = on ? set_accent() : set_muted();
    lv_opa_t ico_opa = on ? LV_OPA_COVER : LV_OPA_50;
    /* Aktif = mavi kenar; pasif = kenar yok. Menu acikken secili kart vurgulu. */
    bool menu_on = (s_relay_menu && s_relay_menu_slot == slot);
    uint32_t border = (on || menu_on) ? set_accent() : set_card_bg();
    int bw = menu_on ? 2 : (on ? 2 : 0);

    if (slot < 0 || slot >= PANEL_BUTTON_SLOTS) {
        return;
    }
    if (s_btn_card[slot]) {
        lv_obj_set_style_border_color(s_btn_card[slot], lv_color_hex(border), 0);
        lv_obj_set_style_border_width(s_btn_card[slot], bw, 0);
        lv_obj_set_style_border_opa(s_btn_card[slot],
                                    (on || menu_on) ? LV_OPA_COVER : LV_OPA_TRANSP, 0);
        lv_obj_set_style_opa(s_btn_card[slot], on ? LV_OPA_COVER : LV_OPA_70, 0);
    }
    if (s_btn_ico[slot]) {
        lv_obj_set_style_img_recolor(s_btn_ico[slot], lv_color_hex(ico_col), 0);
        lv_obj_set_style_img_recolor_opa(s_btn_ico[slot], ico_opa, 0);
    }
    if (s_btn_nm[slot]) {
        lv_obj_set_style_text_color(s_btn_nm[slot], lv_color_hex(fg), 0);
        lv_obj_set_style_opa(s_btn_nm[slot], on ? LV_OPA_COVER : LV_OPA_60, 0);
    }
    if (s_btn_rl[slot]) {
        lv_obj_set_style_text_color(s_btn_rl[slot], lv_color_hex(on ? set_text() : set_muted()), 0);
        lv_obj_set_style_opa(s_btn_rl[slot], on ? LV_OPA_COVER : LV_OPA_60, 0);
    }
    if (s_btn_title) {
        int active = 0;
        int total = PANEL_BUTTON_SLOTS;
        char hdr[48];
        panel_counts(PANEL_CAT_BUTTON, &active, &total);
        snprintf(hdr, sizeof(hdr), "%s (%d/%d)",
                 vango_tx("BUTTONS", "BUTONLAR", "TASTER"), active, total);
        lv_label_set_text(s_btn_title, hdr);
    }
}

static void panel_button_sw_cb(lv_event_t *e)
{
    int slot = (int)(intptr_t)lv_event_get_user_data(e);
    lv_obj_t *sw = lv_event_get_target(e);
    const hexnet_io_profile_t *p = hexnet_io_profile_get();
    bool on;
    int type = 1;

    if (slot < 0 || slot >= PANEL_BUTTON_SLOTS || !p || !sw) {
        return;
    }
    /* LVGL switch'i zaten cevirdi — profili ona gore yaz (NVS ertelenir) */
    on = lv_obj_has_state(sw, LV_STATE_CHECKED);
    type = p->relay_type[slot] ? (int)p->relay_type[slot] : 1;
    ESP_LOGI(TAG, "Buton switch slot=%d -> %s", slot, on ? "AKTIF" : "PASIF");
    if (s_relay_menu) {
        relay_menu_close();
    }
    hexnet_io_profile_set_relay(slot, on, type);
    panel_button_paint_active(slot, on);
    /* Ana sayfa kenar rengi de hemen guncellensin */
    vango_dash_refresh_channel_meta();
}

static void side_set_active(set_sec_t sec)
{
    for (int i = 0; i < SET_SEC_COUNT; i++) {
        bool on = (i == (int)sec);
        if (s_side_btns[i]) {
            lv_obj_set_style_border_color(s_side_btns[i], lv_color_hex(set_accent()), 0);
            lv_obj_set_style_border_width(s_side_btns[i], on ? 1 : 0, 0);
            lv_obj_set_style_bg_color(s_side_btns[i], lv_color_hex(set_sel_bg()), 0);
            lv_obj_set_style_bg_opa(s_side_btns[i], on ? LV_OPA_COVER : LV_OPA_TRANSP, 0);
        }
        if (s_side_lbls[i]) {
            lv_obj_set_style_text_color(s_side_lbls[i], lv_color_hex(on ? set_text() : set_muted()), 0);
        }
        glyph_set_color(s_side_glyphs[i], on ? set_accent() : set_muted());
    }
}

static int mask_bits(uint32_t mask, int n)
{
    int count = 0;
    for (int i = 0; i < n; i++) {
        count += (mask & (1u << i)) ? 1 : 0;
    }
    return count;
}

static void panel_counts(panel_cat_t cat, int *active, int *total)
{
    const hexnet_io_profile_t *p = hexnet_io_profile_get();
    *active = 0;
    *total = 1;
    if (!p) {
        return;
    }
    switch (cat) {
        case PANEL_CAT_BUTTON:
            *active = mask_bits(p->relay_enable_mask, PANEL_BUTTON_SLOTS);
            *total = PANEL_BUTTON_SLOTS;
            break;
        case PANEL_CAT_LIGHT:
            *active = mask_bits(p->dim_enable_mask, HEXNET_IO_DIM_SLOTS);
            *total = HEXNET_IO_DIM_SLOTS;
            break;
        case PANEL_CAT_MOTOR:
            *active = ((p->motor_enabled & 0x01u) ? 1 : 0) + ((p->motor_enabled & 0x02u) ? 1 : 0);
            *total = HEXNET_IO_MOTOR_SLOTS;
            break;
        case PANEL_CAT_SENSOR:
            *active = mask_bits(p->sensor_enable_mask, HEXNET_IO_SENSOR_SLOTS);
            *total = HEXNET_IO_SENSOR_SLOTS;
            break;
        case PANEL_CAT_WATER:
            *active = mask_bits(p->tank_enable_mask, HEXNET_IO_TANK_SLOTS);
            *total = HEXNET_IO_TANK_SLOTS;
            break;
        case PANEL_CAT_RGB:
            *active = p->rgb_enabled ? 1 : 0;
            *total = 1;
            break;
        default:
            break;
    }
}

static const char *panel_cat_name(panel_cat_t cat)
{
    switch (cat) {
        case PANEL_CAT_BUTTON: return vango_tx("Button", "Buton", "Taster");
        case PANEL_CAT_LIGHT: return vango_tx("Lighting", "Aydınlatma", "Beleuchtung");
        case PANEL_CAT_MOTOR: return vango_tx("Motors", "Motorlar", "Motoren");
        case PANEL_CAT_SENSOR: return vango_tx("Sensors", "Sensörler", "Sensoren");
        case PANEL_CAT_WATER: return vango_tx("Water Level", "Su seviye", "Füllstand");
        case PANEL_CAT_RGB: return vango_tx("RGB Lighting", "RGB Aydınlatma", "RGB-Beleuchtung");
        default: return "";
    }
}

static uint32_t panel_cat_color(panel_cat_t cat)
{
    switch (cat) {
        case PANEL_CAT_BUTTON: return COL_GLOW_G;
        case PANEL_CAT_LIGHT: return COL_GLOW_Y;
        case PANEL_CAT_MOTOR: return COL_ORANGE;
        case PANEL_CAT_SENSOR: return COL_GLOW_P;
        case PANEL_CAT_WATER: return COL_GLOW_B;
        case PANEL_CAT_RGB: return COL_RED;
        default: return COL_MUTED;
    }
}

static const lv_img_dsc_t *panel_cat_icon(panel_cat_t cat)
{
    switch (cat) {
        case PANEL_CAT_BUTTON: return &vango_nav_power;
        case PANEL_CAT_LIGHT: return &vango_nav_light;
        case PANEL_CAT_MOTOR: return &vango_nav_motor;
        case PANEL_CAT_SENSOR: return &vango_nav_sensor;
        case PANEL_CAT_WATER: return &vango_nav_water;
        case PANEL_CAT_RGB: return NULL;
        default: return &vango_nav_settings;
    }
}


static void panel_card_cb(lv_event_t *e)
{
    panel_cat_t cat = (panel_cat_t)(intptr_t)lv_event_get_user_data(e);
    s_panel_detail = cat;
    show_section(SET_SEC_PANEL);
}

static void panel_back_cb(lv_event_t *e)
{
    (void)e;
    s_panel_detail = PANEL_CAT_COUNT;
    show_section(SET_SEC_PANEL);
}

extern const char *lblBtnNames[18];
extern const char *lblBtnNames_TR[18];
extern const char *lblBtnNames_DE[18];
extern const char *lblDimBtnNames[8];
extern const char *lblDimBtnNames_TR[8];
extern const char *lblDimBtnNames_DE[8];

static const char *relay_type_name(int type)
{
    if (type < 1 || type > 18) {
        type = 1;
    }
    return vango_tx(lblBtnNames[type - 1], lblBtnNames_TR[type - 1], lblBtnNames_DE[type - 1]);
}

static const char *dim_type_name(int type)
{
    if (type < 1 || type > 8) {
        type = 1;
    }
    return vango_tx(lblDimBtnNames[type - 1], lblDimBtnNames_TR[type - 1],
                    lblDimBtnNames_DE[type - 1]);
}

static void dim_pick_close_cb(lv_event_t *e)
{
    (void)e;
    if (s_picker) {
        lv_obj_del(s_picker);
        s_picker = NULL;
    }
    s_liter_ta = NULL;
}

static void dim_pick_cb(lv_event_t *e)
{
    int packed = (int)(intptr_t)lv_event_get_user_data(e);
    int idx = (packed >> 8) & 0xFF;
    int type = packed & 0xFF;
    const hexnet_io_profile_t *p = hexnet_io_profile_get();
    bool on = p && (p->dim_enable_mask & (1u << idx)) != 0;
    const lv_img_dsc_t *isrc;

    hexnet_io_profile_set_dim(idx, on, type);
    if (s_picker) {
        lv_obj_del(s_picker);
        s_picker = NULL;
    }
    if (idx >= 0 && idx < PANEL_LIGHT_SLOTS) {
        if (s_light_nm[idx]) {
            lv_label_set_text(s_light_nm[idx], dim_type_name(type));
        }
        isrc = dim_type_icon(type);
        if (s_light_ico[idx] && isrc) {
            panel_button_apply_icon(s_light_ico[idx], isrc, 40);
            lv_obj_center(s_light_ico[idx]);
        }
        panel_card_set_active(s_light_card[idx], s_light_ico[idx], s_light_nm[idx], s_light_sub[idx], on);
    }
    panel_cat_title_refresh(PANEL_CAT_LIGHT, "LIGHTING", "AYDINLATMA", "BELEUCHTUNG");
    vango_dash_refresh_channel_meta();
}

static void open_dim_picker(int idx)
{
    const hexnet_io_profile_t *p = hexnet_io_profile_get();
    int current = (p && p->dim_type[idx]) ? p->dim_type[idx] : 1;
    int menu_w = 320;
    int menu_h = PAGE_H - 24;
    int row_h = 48;
    int i;
    lv_obj_t *shade;
    lv_obj_t *panel;
    lv_obj_t *list;

    if (s_picker) {
        lv_obj_del(s_picker);
    }
    if (menu_h > PAGE_H - 16) {
        menu_h = PAGE_H - 16;
    }
    if (menu_w > CONT_W - 24) {
        menu_w = CONT_W - 24;
    }

    s_picker = lv_obj_create(s_content);
    lv_obj_set_size(s_picker, CONT_W, PAGE_H);
    lv_obj_set_pos(s_picker, 0, 0);
    lv_obj_set_style_bg_opa(s_picker, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(s_picker, 0, 0);
    lv_obj_set_style_pad_all(s_picker, 0, 0);
    lv_obj_set_style_radius(s_picker, 0, 0);
    lv_obj_clear_flag(s_picker, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(s_picker, LV_OBJ_FLAG_CLICKABLE);

    shade = lv_obj_create(s_picker);
    lv_obj_set_size(shade, CONT_W, PAGE_H);
    lv_obj_set_pos(shade, 0, 0);
    lv_obj_set_style_bg_color(shade, lv_color_hex(0x000000), 0);
    lv_obj_set_style_bg_opa(shade, LV_OPA_40, 0);
    lv_obj_set_style_border_width(shade, 0, 0);
    lv_obj_clear_flag(shade, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_event_cb(shade, dim_pick_close_cb, LV_EVENT_CLICKED, NULL);

    panel = lv_obj_create(s_picker);
    lv_obj_set_size(panel, menu_w, menu_h);
    lv_obj_align(panel, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_bg_color(panel, lv_color_hex(set_card_bg()), 0);
    lv_obj_set_style_bg_opa(panel, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(panel, 1, 0);
    lv_obj_set_style_border_color(panel, lv_color_hex(set_accent()), 0);
    lv_obj_set_style_radius(panel, 10, 0);
    lv_obj_set_style_pad_all(panel, 6, 0);
    lv_obj_clear_flag(panel, LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_EVENT_BUBBLE);
    lv_obj_add_flag(panel, LV_OBJ_FLAG_CLICKABLE);

    list = lv_obj_create(panel);
    lv_obj_set_size(list, menu_w - 12, menu_h - 12);
    lv_obj_align(list, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_bg_opa(list, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(list, 0, 0);
    lv_obj_set_style_pad_all(list, 2, 0);
    lv_obj_set_style_pad_right(list, 10, 0);
    lv_obj_set_style_pad_row(list, 2, 0);
    lv_obj_set_flex_flow(list, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_scroll_dir(list, LV_DIR_VER);
    lv_obj_add_flag(list, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_scrollbar_mode(list, LV_SCROLLBAR_MODE_ON);
    lv_obj_set_style_width(list, 8, LV_PART_SCROLLBAR);
    lv_obj_set_style_bg_color(list, lv_color_hex(set_accent()), LV_PART_SCROLLBAR);
    lv_obj_set_style_bg_opa(list, LV_OPA_COVER, LV_PART_SCROLLBAR);
    lv_obj_set_style_radius(list, 4, LV_PART_SCROLLBAR);

    for (i = 0; i < 8; i++) {
        bool sel = (current == i + 1);
        lv_obj_t *row = lv_obj_create(list);
        lv_obj_t *ibox;
        lv_obj_t *ico;
        lv_obj_t *lbl;
        const lv_img_dsc_t *isrc = dim_type_icon(i + 1);

        lv_obj_set_size(row, menu_w - 28, row_h);
        lv_obj_set_style_radius(row, 8, 0);
        lv_obj_set_style_border_width(row, 0, 0);
        lv_obj_set_style_pad_all(row, 0, 0);
        lv_obj_set_style_bg_color(row, lv_color_hex(sel ? set_accent() : set_card_bg()), 0);
        lv_obj_set_style_bg_opa(row, sel ? LV_OPA_COVER : LV_OPA_TRANSP, 0);
        lv_obj_clear_flag(row, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_add_flag(row, LV_OBJ_FLAG_CLICKABLE);
        lv_obj_add_event_cb(row, dim_pick_cb, LV_EVENT_CLICKED,
                            (void *)(intptr_t)((idx << 8) | (i + 1)));

        ibox = lv_obj_create(row);
        lv_obj_set_size(ibox, 40, 40);
        lv_obj_align(ibox, LV_ALIGN_LEFT_MID, 6, 0);
        lv_obj_set_style_bg_opa(ibox, LV_OPA_TRANSP, 0);
        lv_obj_set_style_border_width(ibox, 0, 0);
        lv_obj_set_style_pad_all(ibox, 0, 0);
        lv_obj_clear_flag(ibox, LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_CLICKABLE);
        ico = lv_img_create(ibox);
        if (isrc) {
            panel_button_apply_icon(ico, isrc, 40);
        }
        lv_obj_center(ico);
        lv_obj_set_style_img_recolor(ico, lv_color_hex(sel ? COL_ON_ACCENT : set_accent()), 0);
        lv_obj_set_style_img_recolor_opa(ico, LV_OPA_COVER, 0);

        lbl = make_label(row, dim_type_name(i + 1), &vango_font_16,
                         sel ? COL_ON_ACCENT : set_text());
        lv_obj_align(lbl, LV_ALIGN_LEFT_MID, 52, 0);
        if (sel) {
            lv_obj_t *chk = make_label(row, LV_SYMBOL_OK, &lv_font_montserrat_14, COL_ON_ACCENT);
            lv_obj_align(chk, LV_ALIGN_RIGHT_MID, -8, 0);
        }
    }
}

static void liter_ok_cb(lv_event_t *e)
{
    int v = 0;
    (void)e;
    if (s_liter_ta) {
        const char *t = lv_textarea_get_text(s_liter_ta);
        if (t && t[0]) {
            v = atoi(t);
        }
    }
    vango_dash_set_tank_capacity(s_liter_idx, v);
    if (s_picker) {
        lv_obj_del(s_picker);
        s_picker = NULL;
    }
    s_liter_ta = NULL;
    water_refresh_cap_labels();
}

static void open_tank_liter(int idx)
{
    static const char *tr[] = {"Temiz Su", "Atık Su", "Gri Su"};
    static const char *en[] = {"Fresh Water", "Waste Water", "Grey Water"};
    static const char *de[] = {"Frischwasser", "Abwasser", "Grauwasser"};
    char head[40];
    char cur[16];
    int cap;
    if (idx < 0 || idx > 2) {
        return;
    }
    s_liter_idx = idx;
    if (s_picker) {
        lv_obj_del(s_picker);
    }
    s_picker = lv_obj_create(s_content);
    lv_obj_set_size(s_picker, CONT_W, PAGE_H);
    lv_obj_set_pos(s_picker, 0, 0);
    lv_obj_set_style_bg_color(s_picker, lv_color_hex(COL_BG), 0);
    lv_obj_set_style_bg_opa(s_picker, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(s_picker, 0, 0);
    lv_obj_set_style_pad_all(s_picker, 0, 0);
    lv_obj_set_style_radius(s_picker, 0, 0);
    lv_obj_clear_flag(s_picker, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *back = lv_btn_create(s_picker);
    lv_obj_set_size(back, 36, 28);
    lv_obj_set_pos(back, 4, 4);
    lv_obj_set_style_bg_opa(back, LV_OPA_TRANSP, 0);
    lv_obj_set_style_shadow_width(back, 0, 0);
    lv_obj_t *bl = make_label(back, LV_SYMBOL_LEFT, &lv_font_montserrat_16, COL_TEXT);
    lv_obj_center(bl);
    lv_obj_add_event_cb(back, dim_pick_close_cb, LV_EVENT_CLICKED, NULL);

    snprintf(head, sizeof(head), "%s", vango_tx(en[idx], tr[idx], de[idx]));
    make_label(s_picker, head, &vango_font_16, COL_TEXT);
    lv_obj_set_pos(lv_obj_get_child(s_picker, lv_obj_get_child_cnt(s_picker) - 1), 48, 8);

    s_liter_ta = lv_textarea_create(s_picker);
    lv_obj_set_size(s_liter_ta, 160, 40);
    lv_obj_set_pos(s_liter_ta, 16, 44);
    lv_textarea_set_one_line(s_liter_ta, true);
    lv_textarea_set_accepted_chars(s_liter_ta, "0123456789");
    lv_textarea_set_max_length(s_liter_ta, 5);
    lv_obj_set_style_text_font(s_liter_ta, &vango_font_16, 0);
    cap = vango_dash_tank_capacity(idx);
    if (cap > 0) {
        snprintf(cur, sizeof(cur), "%d", cap);
        lv_textarea_set_text(s_liter_ta, cur);
    }
    lv_obj_add_event_cb(s_liter_ta, liter_ok_cb, LV_EVENT_READY, NULL);

    make_label(s_picker,
               vango_tx("Total liters. Empty hides the line.", "Toplam litre. Boş bırakılırsa satır gizlenir.", "Gesamtliter. Leer blendet die Zeile aus."),
               &vango_font_12, COL_MUTED);
    lv_obj_set_pos(lv_obj_get_child(s_picker, lv_obj_get_child_cnt(s_picker) - 1), 188, 54);

    lv_obj_t *kb = lv_keyboard_create(s_picker);
    lv_obj_set_size(kb, CONT_W - 8, PAGE_H - 100);
    lv_obj_align(kb, LV_ALIGN_BOTTOM_MID, 0, -4);
    lv_keyboard_set_mode(kb, LV_KEYBOARD_MODE_NUMBER);
    lv_keyboard_set_textarea(kb, s_liter_ta);
}

static void panel_slot_cb(lv_event_t *e)
{
    uintptr_t pack = (uintptr_t)lv_event_get_user_data(e);
    int cat = (int)((pack >> 16) & 0xFFu);
    int idx = (int)((pack >> 8) & 0xFFu);
    int act = (int)(pack & 0xFFu);
    const hexnet_io_profile_t *p = hexnet_io_profile_get();
    if (!p) {
        return;
    }

    if (cat == PANEL_CAT_BUTTON) {
        int slot = idx;
        bool on = (p->relay_enable_mask & (1u << slot)) != 0;
        int type = p->relay_type[slot] ? p->relay_type[slot] : 1;
        if (act == 1) {
            type = (type % 18) + 1;
            on = true;
        } else if (lv_event_get_code(e) == LV_EVENT_VALUE_CHANGED) {
            lv_obj_t *sw = lv_event_get_target(e);
            on = sw && lv_obj_has_state(sw, LV_STATE_CHECKED);
        } else {
            on = !on;
        }
        hexnet_io_profile_set_relay(slot, on, type);
        panel_button_paint_active(slot, on);
        vango_dash_refresh_channel_meta();
    } else if (cat == PANEL_CAT_LIGHT) {
        if (act == 1) {
            open_dim_picker(idx);
            return;
        }
        bool on = (p->dim_enable_mask & (1u << idx)) == 0;
        int type = p->dim_type[idx] ? p->dim_type[idx] : 1;
        hexnet_io_profile_set_dim(idx, on, type);
    } else if (cat == PANEL_CAT_SENSOR) {
        bool on = (p->sensor_enable_mask & (1u << idx)) == 0;
        hexnet_io_profile_set_sensor(idx, on);
    } else if (cat == PANEL_CAT_WATER) {
        if (act == 1) {
            open_tank_liter(idx);
            return;
        }
        bool on = (p->tank_enable_mask & (1u << idx)) == 0;
        hexnet_io_profile_set_tank(idx, on);
    } else if (cat == PANEL_CAT_MOTOR) {
        bool on = (p->motor_enabled & (1u << idx)) == 0;
        hexnet_io_profile_set_motor(idx, on);
    } else if (cat == PANEL_CAT_RGB) {
        hexnet_io_profile_set_rgb(!p->rgb_enabled);
    }
    /* Event icinde widget silmek reset yapar — async yenile */
    lv_async_call(panel_section_refresh_async, NULL);
}

static void make_rgb_mark(lv_obj_t *card, int card_h)
{
    static const uint32_t cols[3] = {0xEF4444u, 0x22C55Eu, 0x3B82F6u};
    static const int dx[3] = {0, 14, 7};
    static const int dy[3] = {10, 10, 0};
    const int y0 = (card_h - 26) / 2;
    for (int i = 0; i < 3; i++) {
        lv_obj_t *dot = lv_obj_create(card);
        lv_obj_set_size(dot, 16, 16);
        lv_obj_set_pos(dot, 12 + dx[i], y0 + dy[i]);
        lv_obj_set_style_radius(dot, LV_RADIUS_CIRCLE, 0);
        lv_obj_set_style_bg_color(dot, lv_color_hex(cols[i]), 0);
        lv_obj_set_style_bg_opa(dot, LV_OPA_COVER, 0);
        lv_obj_set_style_border_width(dot, 0, 0);
        lv_obj_clear_flag(dot, LV_OBJ_FLAG_CLICKABLE);
    }
}

static void build_panel_overview(void)
{
    const int card_w = (CONT_W - SET_GAP - SET_RIGHT - 2 * 14) / 3;
    const int card_h = 92;
    const int gap_x = 14;
    const int gap_y = 14;
    const int ox = SET_GAP;
    const int oy = SET_TOP;

    for (int i = 0; i < PANEL_CAT_COUNT; i++) {
        int col = i % 3;
        int row = i / 3;
        int active = 0;
        int total = 1;
        const lv_img_dsc_t *icon;
        char buf[32];
        lv_obj_t *card;
        lv_obj_t *txt;
        lv_obj_t *title;
        lv_obj_t *st;
        lv_obj_t *arrow;

        panel_counts((panel_cat_t)i, &active, &total);

        card = lv_obj_create(s_content);
        lv_obj_set_size(card, card_w, card_h);
        lv_obj_set_pos(card, ox + col * (card_w + gap_x), oy + row * (card_h + gap_y));
        style_card(card);
        lv_obj_add_flag(card, LV_OBJ_FLAG_CLICKABLE);
        lv_obj_add_event_cb(card, panel_card_cb, LV_EVENT_CLICKED, (void *)(intptr_t)i);

        icon = (i == PANEL_CAT_MOTOR) ? vango_dash_motor_icon(true) : panel_cat_icon((panel_cat_t)i);
        if (icon) {
            lv_obj_t *ico = lv_img_create(card);
            lv_img_set_src(ico, icon);
            lv_obj_align(ico, LV_ALIGN_LEFT_MID, 4, 0);
            if (i != PANEL_CAT_MOTOR) {
                lv_obj_set_style_img_recolor(ico, lv_color_hex(panel_cat_color((panel_cat_t)i)), 0);
                lv_obj_set_style_img_recolor_opa(ico, LV_OPA_COVER, 0);
            }
            lv_obj_clear_flag(ico, LV_OBJ_FLAG_CLICKABLE);
        } else {
            make_rgb_mark(card, card_h);
        }

        txt = lv_obj_create(card);
        lv_obj_set_size(txt, card_w - 92, card_h - 16);
        lv_obj_align(txt, LV_ALIGN_LEFT_MID, 50, 0);
        lv_obj_set_style_bg_opa(txt, LV_OPA_TRANSP, 0);
        lv_obj_set_style_border_width(txt, 0, 0);
        lv_obj_set_style_pad_all(txt, 0, 0);
        lv_obj_set_style_pad_row(txt, 2, 0);
        lv_obj_set_flex_flow(txt, LV_FLEX_FLOW_COLUMN);
        lv_obj_set_flex_align(txt, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);
        lv_obj_clear_flag(txt, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);

        title = make_label(txt, panel_cat_name((panel_cat_t)i), &vango_font_16, set_text());
        lv_obj_set_width(title, card_w - 92);
        lv_label_set_long_mode(title, LV_LABEL_LONG_WRAP);

        snprintf(buf, sizeof(buf), vango_tx("%d/%d Active", "%d/%d Aktif", "%d/%d Aktiv"), active, total);
        st = make_label(txt, buf, &vango_font_14, set_muted());
        lv_obj_set_width(st, card_w - 92);

        arrow = make_label(card, LV_SYMBOL_RIGHT, &lv_font_montserrat_20, set_muted());
        lv_obj_align(arrow, LV_ALIGN_RIGHT_MID, -10, 0);
    }
}

static const void *panel_button_icon(int slot)
{
    const hexnet_io_labels_t *L = hexnet_io_labels_get();
    const hexnet_io_profile_t *p = hexnet_io_profile_get();
    int type = 1;
    const void *img;

    /* Ayarlar: 40px sm set (ALPHA_8BIT zoom=256; 96px tasma yapardi) */
    if (L && slot >= 0 && slot < HEXNET_LABEL_RELAYS && L->relays[slot].icon[0]) {
        const lv_img_dsc_t *panel = vango_panel_icon_sm_by_key(L->relays[slot].icon);
        if (panel) {
            return panel;
        }
        type = (int)hexnet_icon_key_to_preset(L->relays[slot].icon);
    }
    if ((type < 1 || type > 18) && p && slot >= 0 && slot < HEXNET_IO_RELAY_SLOTS) {
        type = p->relay_type[slot] ? (int)p->relay_type[slot] : 1;
    }
    if (type < 1 || type > 18) {
        type = 1;
    }
    img = get_image_for_button(type - 1);
    return img ? img : (const void *)&vango_nav_power;
}

static void relay_menu_close(void)
{
    int prev = s_relay_menu_slot;
    const hexnet_io_profile_t *p;

    if (s_relay_menu) {
        lv_obj_del(s_relay_menu);
        s_relay_menu = NULL;
    }
    s_relay_menu_slot = -1;
    if (prev >= 0 && prev < PANEL_BUTTON_SLOTS) {
        p = hexnet_io_profile_get();
        panel_button_paint_active(prev, p && (p->relay_enable_mask & (1u << prev)) != 0);
    }
}

static void relay_menu_shade_cb(lv_event_t *e)
{
    if (lv_event_get_code(e) == LV_EVENT_CLICKED) {
        relay_menu_close();
    }
}

static void relay_icon_apply_local(int slot, const char *key, bool push_can)
{
    const vango_panel_catalog_item_t *cat = vango_panel_catalog();
    size_t n = vango_panel_catalog_count();
    const hexnet_io_profile_t *p = hexnet_io_profile_get();
    const char *name;
    int type;
    bool on;
    size_t i;

    if (!key || !key[0] || slot < 0 || slot >= PANEL_BUTTON_SLOTS || !p || !cat) {
        return;
    }
    for (i = 0; i < n; ++i) {
        if (strcmp(cat[i].key, key) == 0) {
            break;
        }
    }
    if (i >= n) {
        ESP_LOGW(TAG, "icon key unknown: %s", key);
        return;
    }
    name = vango_tx(cat[i].en, cat[i].tr, cat[i].en);
    type = (int)hexnet_icon_key_to_preset(key);
    on = (p->relay_enable_mask & (1u << slot)) != 0;
    hexnet_io_labels_set_relay(slot, name, key);
    hexnet_io_profile_set_relay(slot, on, type);
    if (push_can) {
        hexnet_can_push_relay_label(slot, name, key);
        /* NVS: labels_set_relay zaten deferred task schedule eder */
    }
    panel_button_paint_meta(slot);
    panel_button_paint_active(slot, on);
    vango_dash_refresh_channel_meta();
    ESP_LOGW(TAG, "set relay icon slot=%d key=%s push=%d", slot, key, push_can ? 1 : 0);
}

static void icon_confirm_close(bool revert)
{
    int slot = s_icon_pending_slot;
    const hexnet_io_profile_t *p;
    bool on;

    if (revert && slot >= 0 && slot < PANEL_BUTTON_SLOTS) {
        p = hexnet_io_profile_get();
        on = p && (p->relay_enable_mask & (1u << slot)) != 0;
        hexnet_io_labels_set_relay(slot, s_icon_pending_old_name, s_icon_pending_old_icon);
        hexnet_io_profile_set_relay(slot, on, s_icon_pending_old_type);
        panel_button_paint_meta(slot);
        panel_button_paint_active(slot, on);
        vango_dash_refresh_channel_meta();
        ESP_LOGI(TAG, "icon change cancelled slot=%d", slot);
    }
    s_icon_pending_slot = -1;
    s_icon_pending_new_key[0] = '\0';
    s_icon_pending_old_icon[0] = '\0';
    s_icon_pending_old_name[0] = '\0';
    s_icon_pending_old_type = 1;
    if (s_icon_confirm) {
        lv_obj_del(s_icon_confirm);
        s_icon_confirm = NULL;
    }
}

static void icon_confirm_save_cb(lv_event_t *e)
{
    int slot;
    const hexnet_io_labels_t *L;

    if (lv_event_get_code(e) != LV_EVENT_CLICKED) {
        return;
    }
    slot = s_icon_pending_slot;
    if (slot < 0 || slot >= PANEL_BUTTON_SLOTS || !s_icon_pending_new_key[0]) {
        icon_confirm_close(false);
        return;
    }
    L = hexnet_io_labels_get();
    /* Onizleme zaten uygulandi — sadece IO'ya gonder ve NVS kaydet */
    if (L && slot < HEXNET_LABEL_RELAYS) {
        hexnet_can_push_relay_label(slot, L->relays[slot].name, L->relays[slot].icon);
        ESP_LOGW(TAG, "icon change saved slot=%d key=%s", slot, L->relays[slot].icon);
    } else {
        hexnet_can_push_relay_label(slot, "", s_icon_pending_new_key);
        ESP_LOGW(TAG, "icon change saved slot=%d key=%s", slot, s_icon_pending_new_key);
    }
    /* NVS onizlemede dirty — Kaydet sadece CAN push; flash ayri task */
    icon_confirm_close(false);
}

static void icon_confirm_cancel_cb(lv_event_t *e)
{
    if (lv_event_get_code(e) == LV_EVENT_CLICKED) {
        icon_confirm_close(true);
    }
}

static void icon_confirm_show(int slot, const char *new_key)
{
    lv_obj_t *shade;
    lv_obj_t *panel;
    lv_obj_t *msg;
    lv_obj_t *row;
    lv_obj_t *btn_save;
    lv_obj_t *btn_cancel;
    lv_obj_t *lbl;

    if (!s_content || slot < 0 || slot >= PANEL_BUTTON_SLOTS || !new_key || !new_key[0]) {
        return;
    }
    if (s_icon_confirm) {
        icon_confirm_close(true);
    }

    s_icon_confirm = lv_obj_create(s_content);
    lv_obj_set_size(s_icon_confirm, CONT_W, PAGE_H);
    lv_obj_set_pos(s_icon_confirm, 0, 0);
    lv_obj_set_style_bg_opa(s_icon_confirm, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(s_icon_confirm, 0, 0);
    lv_obj_set_style_pad_all(s_icon_confirm, 0, 0);
    lv_obj_set_style_radius(s_icon_confirm, 0, 0);
    lv_obj_clear_flag(s_icon_confirm, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(s_icon_confirm, LV_OBJ_FLAG_CLICKABLE);

    shade = lv_obj_create(s_icon_confirm);
    lv_obj_set_size(shade, CONT_W, PAGE_H);
    lv_obj_set_pos(shade, 0, 0);
    lv_obj_set_style_bg_color(shade, lv_color_hex(0x000000), 0);
    lv_obj_set_style_bg_opa(shade, LV_OPA_40, 0);
    lv_obj_set_style_border_width(shade, 0, 0);
    lv_obj_set_style_radius(shade, 0, 0);
    lv_obj_clear_flag(shade, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_event_cb(shade, icon_confirm_cancel_cb, LV_EVENT_CLICKED, NULL);

    panel = lv_obj_create(s_icon_confirm);
    lv_obj_set_size(panel, 300, 150);
    lv_obj_align(panel, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_bg_color(panel, lv_color_hex(set_card_bg()), 0);
    lv_obj_set_style_bg_opa(panel, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(panel, 1, 0);
    lv_obj_set_style_border_color(panel, lv_color_hex(set_accent()), 0);
    lv_obj_set_style_radius(panel, 10, 0);
    lv_obj_set_style_pad_all(panel, 14, 0);
    lv_obj_set_style_shadow_width(panel, 0, 0);
    lv_obj_clear_flag(panel, LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_EVENT_BUBBLE);
    lv_obj_add_flag(panel, LV_OBJ_FLAG_CLICKABLE);

    msg = make_label(panel,
                     vango_tx("Save your changes?",
                              "Yaptığınız değişikliği kaydetmek istiyor musunuz?",
                              "Änderungen speichern?"),
                     &vango_font_16, set_text());
    lv_obj_set_width(msg, 270);
    lv_label_set_long_mode(msg, LV_LABEL_LONG_WRAP);
    lv_obj_set_style_text_align(msg, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_align(msg, LV_ALIGN_TOP_MID, 0, 8);

    row = lv_obj_create(panel);
    lv_obj_set_size(row, 270, 44);
    lv_obj_align(row, LV_ALIGN_BOTTOM_MID, 0, -4);
    lv_obj_set_style_bg_opa(row, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(row, 0, 0);
    lv_obj_set_style_pad_all(row, 0, 0);
    lv_obj_set_style_radius(row, 0, 0);
    lv_obj_clear_flag(row, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_flex_flow(row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(row, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    btn_cancel = lv_obj_create(row);
    lv_obj_set_size(btn_cancel, 120, 40);
    lv_obj_set_style_bg_color(btn_cancel, lv_color_hex(set_card_bg()), 0);
    lv_obj_set_style_bg_opa(btn_cancel, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(btn_cancel, 1, 0);
    lv_obj_set_style_border_color(btn_cancel, lv_color_hex(set_accent()), 0);
    lv_obj_set_style_radius(btn_cancel, 8, 0);
    lv_obj_set_style_pad_all(btn_cancel, 0, 0);
    lv_obj_clear_flag(btn_cancel, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(btn_cancel, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(btn_cancel, icon_confirm_cancel_cb, LV_EVENT_CLICKED, NULL);
    lbl = make_label(btn_cancel, vango_tx("Cancel", "İptal", "Abbrechen"),
                     &vango_font_16, set_text());
    lv_obj_center(lbl);
    lv_obj_clear_flag(lbl, LV_OBJ_FLAG_CLICKABLE);

    btn_save = lv_obj_create(row);
    lv_obj_set_size(btn_save, 120, 40);
    lv_obj_set_style_bg_color(btn_save, lv_color_hex(set_accent()), 0);
    lv_obj_set_style_bg_opa(btn_save, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(btn_save, 0, 0);
    lv_obj_set_style_radius(btn_save, 8, 0);
    lv_obj_set_style_pad_all(btn_save, 0, 0);
    lv_obj_clear_flag(btn_save, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(btn_save, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(btn_save, icon_confirm_save_cb, LV_EVENT_CLICKED, NULL);
    lbl = make_label(btn_save, vango_tx("Save", "Kaydet", "Speichern"),
                     &vango_font_16, COL_ON_ACCENT);
    lv_obj_center(lbl);
    lv_obj_clear_flag(lbl, LV_OBJ_FLAG_CLICKABLE);

    (void)new_key;
}

static void relay_menu_pick_cb(lv_event_t *e)
{
    int idx = (int)(intptr_t)lv_event_get_user_data(e);
    int slot = s_relay_menu_slot;
    const vango_panel_catalog_item_t *cat = vango_panel_catalog();
    size_t n = vango_panel_catalog_count();
    const hexnet_io_labels_t *L = hexnet_io_labels_get();
    const hexnet_io_profile_t *p = hexnet_io_profile_get();
    const char *cur_icon = "";

    if (slot < 0 || slot >= PANEL_BUTTON_SLOTS || idx < 0 || (size_t)idx >= n || !cat) {
        relay_menu_close();
        return;
    }
    if (L && slot < HEXNET_LABEL_RELAYS && L->relays[slot].icon[0]) {
        cur_icon = L->relays[slot].icon;
    }
    if (cur_icon[0] && strcmp(cur_icon, cat[idx].key) == 0) {
        /* Ayni ikon — degisiklik yok */
        relay_menu_close();
        return;
    }

    /* Onceki bekleyen degisikligi geri al */
    if (s_icon_pending_slot >= 0) {
        icon_confirm_close(true);
    }

    s_icon_pending_slot = slot;
    strncpy(s_icon_pending_new_key, cat[idx].key, sizeof(s_icon_pending_new_key) - 1);
    s_icon_pending_new_key[sizeof(s_icon_pending_new_key) - 1] = '\0';
    s_icon_pending_old_icon[0] = '\0';
    s_icon_pending_old_name[0] = '\0';
    s_icon_pending_old_type = (p && p->relay_type[slot]) ? (int)p->relay_type[slot] : 1;
    if (L && slot < HEXNET_LABEL_RELAYS) {
        strncpy(s_icon_pending_old_icon, L->relays[slot].icon, sizeof(s_icon_pending_old_icon) - 1);
        strncpy(s_icon_pending_old_name, L->relays[slot].name, sizeof(s_icon_pending_old_name) - 1);
        s_icon_pending_old_icon[sizeof(s_icon_pending_old_icon) - 1] = '\0';
        s_icon_pending_old_name[sizeof(s_icon_pending_old_name) - 1] = '\0';
    }

    /* Yerel onizleme — CAN/IO ancak Kaydet ile */
    relay_icon_apply_local(slot, cat[idx].key, false);
    relay_menu_close();
    icon_confirm_show(slot, cat[idx].key);
}

static void open_relay_type_menu(int slot)
{
    const hexnet_io_labels_t *L = hexnet_io_labels_get();
    const hexnet_io_profile_t *p = hexnet_io_profile_get();
    const vango_panel_catalog_item_t *cat = vango_panel_catalog();
    size_t n = vango_panel_catalog_count();
    const char *cur_icon = "";
    lv_obj_t *shade;
    lv_obj_t *panel;
    lv_obj_t *list;
    lv_obj_t *sel_row = NULL;
    /* Sabit orta panel; kart konumundan bagimsiz */
    int menu_w = 320;
    int menu_h = PAGE_H - 24;
    int row_h = 48;
    int row_w;
    size_t i;

    if (slot < 0 || slot >= PANEL_BUTTON_SLOTS || !s_content || !s_btn_card[slot]) {
        return;
    }
    relay_menu_close();
    s_relay_menu_slot = slot;

    if (L && slot < HEXNET_LABEL_RELAYS && L->relays[slot].icon[0]) {
        cur_icon = L->relays[slot].icon;
    }

    s_relay_menu = lv_obj_create(s_content);
    lv_obj_set_size(s_relay_menu, CONT_W, PAGE_H);
    lv_obj_set_pos(s_relay_menu, 0, 0);
    lv_obj_set_style_bg_opa(s_relay_menu, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(s_relay_menu, 0, 0);
    lv_obj_set_style_pad_all(s_relay_menu, 0, 0);
    lv_obj_set_style_radius(s_relay_menu, 0, 0);
    lv_obj_clear_flag(s_relay_menu, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(s_relay_menu, LV_OBJ_FLAG_CLICKABLE);

    shade = lv_obj_create(s_relay_menu);
    lv_obj_set_size(shade, CONT_W, PAGE_H);
    lv_obj_set_pos(shade, 0, 0);
    lv_obj_set_style_bg_color(shade, lv_color_hex(0x000000), 0);
    lv_obj_set_style_bg_opa(shade, LV_OPA_40, 0);
    lv_obj_set_style_border_width(shade, 0, 0);
    lv_obj_set_style_radius(shade, 0, 0);
    lv_obj_clear_flag(shade, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_event_cb(shade, relay_menu_shade_cb, LV_EVENT_CLICKED, NULL);

    if (menu_h > PAGE_H - 16) {
        menu_h = PAGE_H - 16;
    }
    if (menu_w > CONT_W - 24) {
        menu_w = CONT_W - 24;
    }

    panel = lv_obj_create(s_relay_menu);
    lv_obj_set_size(panel, menu_w, menu_h);
    lv_obj_align(panel, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_bg_color(panel, lv_color_hex(set_card_bg()), 0);
    lv_obj_set_style_bg_opa(panel, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(panel, 1, 0);
    lv_obj_set_style_border_color(panel, lv_color_hex(set_accent()), 0);
    lv_obj_set_style_radius(panel, 10, 0);
    lv_obj_set_style_pad_all(panel, 6, 0);
    lv_obj_set_style_pad_right(panel, 4, 0);
    lv_obj_set_style_shadow_width(panel, 0, 0);
    lv_obj_clear_flag(panel, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(panel, LV_OBJ_FLAG_CLICKABLE);
    /* Panel tiklamasi shade'e dusmesin */
    lv_obj_clear_flag(panel, LV_OBJ_FLAG_EVENT_BUBBLE);

    list = lv_obj_create(panel);
    lv_obj_set_size(list, menu_w - 12, menu_h - 12);
    lv_obj_align(list, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_bg_opa(list, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(list, 0, 0);
    lv_obj_set_style_pad_all(list, 2, 0);
    lv_obj_set_style_pad_right(list, 10, 0); /* scrollbar icin yer */
    lv_obj_set_style_pad_row(list, 2, 0);
    lv_obj_set_flex_flow(list, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_scroll_dir(list, LV_DIR_VER);
    lv_obj_add_flag(list, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_scrollbar_mode(list, LV_SCROLLBAR_MODE_ON);
    lv_obj_set_style_width(list, 8, LV_PART_SCROLLBAR);
    lv_obj_set_style_pad_right(list, 2, LV_PART_SCROLLBAR);
    lv_obj_set_style_bg_color(list, lv_color_hex(set_accent()), LV_PART_SCROLLBAR);
    lv_obj_set_style_bg_opa(list, LV_OPA_COVER, LV_PART_SCROLLBAR);
    lv_obj_set_style_bg_color(list, lv_color_hex(set_accent()), LV_PART_SCROLLBAR | LV_STATE_SCROLLED);
    lv_obj_set_style_bg_opa(list, LV_OPA_COVER, LV_PART_SCROLLBAR | LV_STATE_SCROLLED);
    lv_obj_set_style_radius(list, 4, LV_PART_SCROLLBAR);

    row_w = menu_w - 28;
    for (i = 0; i < n; i++) {
        const lv_img_dsc_t *isrc = vango_panel_icon_sm_by_key(cat[i].key);
        bool sel = false;
        lv_obj_t *row;
        lv_obj_t *ico;
        lv_obj_t *lbl;

        if (!isrc) {
            continue;
        }
        if (cur_icon[0]) {
            sel = (strcmp(cur_icon, cat[i].key) == 0);
        } else if (p && !sel_row) {
            int type = p->relay_type[slot] ? (int)p->relay_type[slot] : 1;
            sel = ((int)hexnet_icon_key_to_preset(cat[i].key) == type);
        }

        row = lv_obj_create(list);
        lv_obj_set_size(row, row_w, row_h);
        lv_obj_set_style_radius(row, 8, 0);
        lv_obj_set_style_border_width(row, 0, 0);
        lv_obj_set_style_pad_all(row, 0, 0);
        lv_obj_set_style_bg_color(row, lv_color_hex(sel ? set_accent() : set_card_bg()), 0);
        lv_obj_set_style_bg_opa(row, sel ? LV_OPA_COVER : LV_OPA_TRANSP, 0);
        lv_obj_clear_flag(row, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_add_flag(row, LV_OBJ_FLAG_CLICKABLE);
        lv_obj_add_flag(row, LV_OBJ_FLAG_EVENT_BUBBLE);
        lv_obj_add_event_cb(row, relay_menu_pick_cb, LV_EVENT_CLICKED, (void *)(intptr_t)i);

        /* 40px sm + sabit kutu (grid mid ile ayni desen; flex listede dogrudan img bazen cizilmiyor) */
        {
            lv_obj_t *ibox = lv_obj_create(row);
            lv_obj_set_size(ibox, 40, 40);
            lv_obj_align(ibox, LV_ALIGN_LEFT_MID, 6, 0);
            lv_obj_set_style_bg_opa(ibox, LV_OPA_TRANSP, 0);
            lv_obj_set_style_border_width(ibox, 0, 0);
            lv_obj_set_style_pad_all(ibox, 0, 0);
            lv_obj_set_style_radius(ibox, 0, 0);
            lv_obj_set_scrollbar_mode(ibox, LV_SCROLLBAR_MODE_OFF);
            lv_obj_clear_flag(ibox, LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_CLICKABLE);

            ico = lv_img_create(ibox);
            panel_button_apply_icon(ico, isrc, 40);
            lv_obj_center(ico);
            lv_obj_set_style_img_recolor(ico, lv_color_hex(sel ? COL_ON_ACCENT : set_accent()), 0);
            lv_obj_set_style_img_recolor_opa(ico, LV_OPA_COVER, 0);
            lv_obj_clear_flag(ico, LV_OBJ_FLAG_CLICKABLE);
        }

        lbl = make_label(row, vango_tx(cat[i].en, cat[i].tr, cat[i].en),
                         &vango_font_16, sel ? COL_ON_ACCENT : set_text());
        lv_obj_set_width(lbl, row_w - 70);
        lv_label_set_long_mode(lbl, LV_LABEL_LONG_DOT);
        lv_obj_align(lbl, LV_ALIGN_LEFT_MID, 52, 0);
        lv_obj_clear_flag(lbl, LV_OBJ_FLAG_CLICKABLE);

        if (sel) {
            lv_obj_t *chk = make_label(row, LV_SYMBOL_OK, &lv_font_montserrat_14, COL_ON_ACCENT);
            lv_obj_align(chk, LV_ALIGN_RIGHT_MID, -8, 0);
            lv_obj_clear_flag(chk, LV_OBJ_FLAG_CLICKABLE);
            sel_row = row;
        }
    }

    if (sel_row) {
        lv_obj_scroll_to_view(sel_row, LV_ANIM_OFF);
    }
    panel_button_paint_active(slot, p && (p->relay_enable_mask & (1u << slot)) != 0);
}

static void panel_button_open_cb(lv_event_t *e)
{
    int slot = (int)(intptr_t)lv_event_get_user_data(e);
    open_relay_type_menu(slot);
}

/*
 * Butonlar: 4x3 sabit izgara.
 * Kart: ustte Röle N + switch | ortada ikon | altta isim (max 2 satir).
 * Switch = aktif/pasif; ikon/isim = tip secim listesi.
 */
static void build_panel_buttons_page(void)
{
    const hexnet_io_profile_t *p = hexnet_io_profile_get();
    const hexnet_io_labels_t *L = hexnet_io_labels_get();
    int active = 0;
    int total = PANEL_BUTTON_SLOTS;
    char hdr[48];
    const int cols = 4;
    const int rows = 3;
    const int gap = 4;
    const int ox = 6;
    const int oy = 22;
    const int grid_w = CONT_W - 12;
    const int card_w = (grid_w - (cols - 1) * gap) / cols;
    const int card_h = (PAGE_H - oy - 2 - (rows - 1) * gap) / rows;
    const int head_h = 20;
    const int name_h = 30;
    int i;

    lv_obj_t *back = lv_btn_create(s_content);
    lv_obj_set_size(back, 32, 26);
    lv_obj_set_pos(back, 2, 2);
    lv_obj_set_style_bg_opa(back, LV_OPA_TRANSP, 0);
    lv_obj_set_style_shadow_width(back, 0, 0);
    lv_obj_t *bl = make_label(back, LV_SYMBOL_LEFT, &lv_font_montserrat_16, set_text());
    lv_obj_center(bl);
    lv_obj_add_event_cb(back, panel_back_cb, LV_EVENT_CLICKED, NULL);

    panel_counts(PANEL_CAT_BUTTON, &active, &total);
    snprintf(hdr, sizeof(hdr), "%s (%d/%d)",
             vango_tx("BUTTONS", "BUTONLAR", "TASTER"), active, total);
    s_btn_title = make_label(s_content, hdr, &vango_font_16, set_accent());
    lv_obj_set_pos(s_btn_title, 36, 6);

    if (!p) {
        return;
    }

    for (i = 0; i < PANEL_BUTTON_SLOTS; i++) {
        char name[32];
        char sub[24];
        bool on = (p->relay_enable_mask & (1u << i)) != 0;
        int col = i % cols;
        int row = i / cols;
        int mid_h = card_h - head_h - name_h;
        uint32_t fg = on ? set_text() : set_muted();
        uint32_t ico_col = on ? set_accent() : set_muted();
        lv_opa_t ico_opa = on ? LV_OPA_COVER : LV_OPA_50;
        lv_obj_t *card;
        lv_obj_t *mid;
        lv_obj_t *hit;
        lv_obj_t *ico;
        lv_obj_t *nm;
        lv_obj_t *rl;
        lv_obj_t *sw;
        lv_obj_t *chev;
        const lv_img_dsc_t *isrc;

        if (L && i < HEXNET_LABEL_RELAYS && L->relays[i].name[0]) {
            snprintf(name, sizeof(name), "%s", L->relays[i].name);
        } else {
            snprintf(name, sizeof(name), "%s", relay_type_name(
                p->relay_type[i] ? (int)p->relay_type[i] : 1));
        }
        snprintf(sub, sizeof(sub), vango_tx("Relay %d", "Röle %d", "Relais %d"), i + 1);

        card = lv_obj_create(s_content);
        s_btn_card[i] = card;
        lv_obj_set_size(card, card_w, card_h);
        lv_obj_set_pos(card, ox + col * (card_w + gap), oy + row * (card_h + gap));
        style_card(card);
        lv_obj_set_style_radius(card, 8, 0);
        lv_obj_clear_flag(card, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_clear_flag(card, LV_OBJ_FLAG_CLICKABLE);
        lv_obj_set_style_pad_all(card, 0, 0);
        lv_obj_set_style_clip_corner(card, true, 0);

        rl = make_label(card, sub, &vango_font_12, on ? set_text() : set_muted());
        lv_obj_set_width(rl, card_w - 46);
        lv_label_set_long_mode(rl, LV_LABEL_LONG_CLIP);
        lv_obj_set_style_text_align(rl, LV_TEXT_ALIGN_LEFT, 0);
        lv_obj_align(rl, LV_ALIGN_TOP_LEFT, 6, 3);
        s_btn_rl[i] = rl;

        sw = lv_switch_create(card);
        lv_obj_set_size(sw, 34, 19);
        lv_obj_align(sw, LV_ALIGN_TOP_RIGHT, -5, 2);
        if (on) {
            lv_obj_add_state(sw, LV_STATE_CHECKED);
        }
        lv_obj_set_style_bg_color(sw, lv_color_hex(COL_ACCENT), LV_PART_INDICATOR | LV_STATE_CHECKED);
        lv_obj_set_style_bg_color(sw, lv_color_hex(set_muted()), LV_PART_INDICATOR);
        lv_obj_add_event_cb(sw, panel_button_sw_cb, LV_EVENT_VALUE_CHANGED, (void *)(intptr_t)i);

        /* Ikon + isim hit alani — switch haric listeyi acar */
        hit = lv_obj_create(card);
        lv_obj_set_size(hit, card_w, card_h - head_h);
        lv_obj_set_pos(hit, 0, head_h);
        lv_obj_set_style_bg_opa(hit, LV_OPA_TRANSP, 0);
        lv_obj_set_style_border_width(hit, 0, 0);
        lv_obj_set_style_pad_all(hit, 0, 0);
        lv_obj_set_style_radius(hit, 0, 0);
        lv_obj_clear_flag(hit, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_add_flag(hit, LV_OBJ_FLAG_CLICKABLE);
        lv_obj_add_event_cb(hit, panel_button_open_cb, LV_EVENT_CLICKED, (void *)(intptr_t)i);

        mid = lv_obj_create(hit);
        lv_obj_set_size(mid, card_w, mid_h);
        lv_obj_set_pos(mid, 0, 0);
        lv_obj_set_style_bg_opa(mid, LV_OPA_TRANSP, 0);
        lv_obj_set_style_border_width(mid, 0, 0);
        lv_obj_set_style_pad_all(mid, 0, 0);
        lv_obj_set_style_radius(mid, 0, 0);
        lv_obj_set_scrollbar_mode(mid, LV_SCROLLBAR_MODE_OFF);
        lv_obj_set_style_clip_corner(mid, true, 0);
        lv_obj_clear_flag(mid, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_clear_flag(mid, LV_OBJ_FLAG_CLICKABLE);
        lv_obj_clear_flag(mid, LV_OBJ_FLAG_OVERFLOW_VISIBLE);

        {
            int box = mid_h - 6;
            if (box > card_w - 10) {
                box = card_w - 10;
            }
            if (box > 48) {
                box = 48;
            }
            if (box < 28) {
                box = 28;
            }
            isrc = (const lv_img_dsc_t *)panel_button_icon(i);
            ico = lv_img_create(mid);
            if (isrc) {
                panel_button_apply_icon(ico, isrc, (uint16_t)box);
            }
            lv_obj_center(ico);
            lv_obj_set_style_img_recolor(ico, lv_color_hex(ico_col), 0);
            lv_obj_set_style_img_recolor_opa(ico, ico_opa, 0);
            lv_obj_clear_flag(ico, LV_OBJ_FLAG_CLICKABLE);
            s_btn_ico[i] = ico;
        }

        nm = make_label(hit, name, &vango_font_14, fg);
        lv_obj_set_width(nm, card_w - 18);
        lv_obj_set_height(nm, name_h);
        lv_label_set_long_mode(nm, LV_LABEL_LONG_WRAP);
        lv_obj_set_style_text_align(nm, LV_TEXT_ALIGN_CENTER, 0);
        lv_obj_set_style_text_line_space(nm, -1, 0);
        lv_obj_set_style_bg_color(nm, lv_color_hex(set_card_bg()), 0);
        lv_obj_set_style_bg_opa(nm, LV_OPA_COVER, 0);
        lv_obj_set_style_pad_top(nm, 1, 0);
        lv_obj_set_pos(nm, 2, mid_h);
        lv_obj_clear_flag(nm, LV_OBJ_FLAG_CLICKABLE);
        s_btn_nm[i] = nm;

        chev = make_label(hit, LV_SYMBOL_DOWN, &lv_font_montserrat_12, set_muted());
        lv_obj_align(chev, LV_ALIGN_BOTTOM_RIGHT, -4, -2);
        lv_obj_clear_flag(chev, LV_OBJ_FLAG_CLICKABLE);

        lv_obj_move_foreground(rl);
        lv_obj_move_foreground(sw);
        panel_button_paint_active(i, on);
    }
}

static void light_sw_cb(lv_event_t *e)
{
    int idx = (int)(intptr_t)lv_event_get_user_data(e);
    lv_obj_t *sw = lv_event_get_target(e);
    const hexnet_io_profile_t *p = hexnet_io_profile_get();
    bool on;
    int type = 1;

    if (idx < 0 || idx >= PANEL_LIGHT_SLOTS || !p || !sw) {
        return;
    }
    on = lv_obj_has_state(sw, LV_STATE_CHECKED);
    type = p->dim_type[idx] ? (int)p->dim_type[idx] : 1;
    hexnet_io_profile_set_dim(idx, on, type);
    panel_card_set_active(s_light_card[idx], s_light_ico[idx], s_light_nm[idx], s_light_sub[idx], on);
    panel_cat_title_refresh(PANEL_CAT_LIGHT, "LIGHTING", "AYDINLATMA", "BELEUCHTUNG");
    vango_dash_refresh_channel_meta();
}

static void light_open_cb(lv_event_t *e)
{
    int idx = (int)(intptr_t)lv_event_get_user_data(e);
    open_dim_picker(idx);
}

/*
 * Aydınlatma: 4x2 dimmer kartlari.
 * Tip ikonu + isim; switch = kanal aktif; tik = tip secici (orta overlay).
 */
static void build_panel_lights_page(void)
{
    const hexnet_io_profile_t *p = hexnet_io_profile_get();
    const hexnet_io_labels_t *L = hexnet_io_labels_get();
    const int cols = 4;
    const int rows = 2;
    const int gap = 6;
    const int ox = 6;
    const int oy = 28;
    const int grid_w = CONT_W - 12;
    const int card_w = (grid_w - (cols - 1) * gap) / cols;
    const int card_h = (PAGE_H - oy - 6 - (rows - 1) * gap) / rows;
    int i;

    panel_detail_header(PANEL_CAT_LIGHT, "LIGHTING", "AYDINLATMA", "BELEUCHTUNG");
    if (!p) {
        return;
    }

    for (i = 0; i < PANEL_LIGHT_SLOTS; i++) {
        char name[32];
        char sub[24];
        int type = p->dim_type[i] ? (int)p->dim_type[i] : 1;
        bool on = (p->dim_enable_mask & (1u << i)) != 0;
        int col = i % cols;
        int row = i / cols;
        lv_obj_t *card;
        lv_obj_t *hit;
        lv_obj_t *ibox;
        lv_obj_t *ico;
        lv_obj_t *nm;
        lv_obj_t *rl;
        lv_obj_t *sw;
        lv_obj_t *chev;
        const lv_img_dsc_t *isrc = dim_type_icon(type);

        if (L && i < HEXNET_LABEL_DIMS && L->dimmers[i].name[0]) {
            snprintf(name, sizeof(name), "%s", L->dimmers[i].name);
        } else {
            snprintf(name, sizeof(name), "%s", dim_type_name(type));
        }
        snprintf(sub, sizeof(sub), "Dim %d", i + 1);

        card = lv_obj_create(s_content);
        s_light_card[i] = card;
        lv_obj_set_size(card, card_w, card_h);
        lv_obj_set_pos(card, ox + col * (card_w + gap), oy + row * (card_h + gap));
        style_card(card);
        lv_obj_set_style_radius(card, 10, 0);
        lv_obj_set_style_pad_all(card, 0, 0);
        lv_obj_clear_flag(card, LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_CLICKABLE);

        rl = make_label(card, sub, &vango_font_12, on ? set_text() : set_muted());
        lv_obj_align(rl, LV_ALIGN_TOP_LEFT, 8, 4);
        s_light_sub[i] = rl;

        sw = lv_switch_create(card);
        lv_obj_set_size(sw, 34, 19);
        lv_obj_align(sw, LV_ALIGN_TOP_RIGHT, -6, 4);
        if (on) {
            lv_obj_add_state(sw, LV_STATE_CHECKED);
        }
        lv_obj_set_style_bg_color(sw, lv_color_hex(COL_ACCENT), LV_PART_INDICATOR | LV_STATE_CHECKED);
        lv_obj_add_event_cb(sw, light_sw_cb, LV_EVENT_VALUE_CHANGED, (void *)(intptr_t)i);

        hit = lv_obj_create(card);
        lv_obj_set_size(hit, card_w, card_h - 26);
        lv_obj_set_pos(hit, 0, 26);
        lv_obj_set_style_bg_opa(hit, LV_OPA_TRANSP, 0);
        lv_obj_set_style_border_width(hit, 0, 0);
        lv_obj_set_style_pad_all(hit, 0, 0);
        lv_obj_clear_flag(hit, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_add_flag(hit, LV_OBJ_FLAG_CLICKABLE);
        lv_obj_add_event_cb(hit, light_open_cb, LV_EVENT_CLICKED, (void *)(intptr_t)i);

        ibox = lv_obj_create(hit);
        lv_obj_set_size(ibox, 48, 48);
        lv_obj_align(ibox, LV_ALIGN_TOP_MID, 0, 4);
        lv_obj_set_style_bg_opa(ibox, LV_OPA_TRANSP, 0);
        lv_obj_set_style_border_width(ibox, 0, 0);
        lv_obj_set_style_pad_all(ibox, 0, 0);
        lv_obj_clear_flag(ibox, LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_CLICKABLE);
        ico = lv_img_create(ibox);
        if (isrc) {
            panel_button_apply_icon(ico, isrc, 40);
        }
        lv_obj_center(ico);
        s_light_ico[i] = ico;

        nm = make_label(hit, name, &vango_font_14, on ? set_text() : set_muted());
        lv_obj_set_width(nm, card_w - 12);
        lv_label_set_long_mode(nm, LV_LABEL_LONG_DOT);
        lv_obj_set_style_text_align(nm, LV_TEXT_ALIGN_CENTER, 0);
        lv_obj_align(nm, LV_ALIGN_BOTTOM_MID, 0, -14);
        s_light_nm[i] = nm;

        chev = make_label(hit, LV_SYMBOL_DOWN, &lv_font_montserrat_12, set_muted());
        lv_obj_align(chev, LV_ALIGN_BOTTOM_RIGHT, -4, -2);

        lv_obj_move_foreground(sw);
        panel_card_set_active(card, ico, nm, rl, on);
    }
}

/* Soft gear is TRUE_COLOR — keep baked tones; dim via opacity only. */
static void motor_card_paint(int idx, bool on)
{
    if (idx < 0 || idx >= PANEL_MOTOR_SLOTS) {
        return;
    }
    panel_card_set_active(s_motor_card[idx], NULL, s_motor_nm[idx], NULL, on);
    if (s_motor_ico[idx]) {
        lv_obj_set_style_img_recolor_opa(s_motor_ico[idx], LV_OPA_TRANSP, 0);
        lv_obj_set_style_img_opa(s_motor_ico[idx], on ? LV_OPA_COVER : LV_OPA_40, 0);
    }
}

static void motor_sw_cb(lv_event_t *e)
{
    int idx = (int)(intptr_t)lv_event_get_user_data(e);
    lv_obj_t *sw = lv_event_get_target(e);
    bool on;

    if (idx < 0 || idx >= PANEL_MOTOR_SLOTS || !sw) {
        return;
    }
    on = lv_obj_has_state(sw, LV_STATE_CHECKED);
    hexnet_io_profile_set_motor(idx, on);
    motor_card_paint(idx, on);
    panel_cat_title_refresh(PANEL_CAT_MOTOR, "MOTORS", "MOTORLAR", "MOTOREN");
}

/*
 * Motorlar: 2 genis kart — sadece kanal aktif/pasif.
 * Yon/hiz Motor sayfasinda (0x750); burada profil enable.
 * Ikon: Motor sayfasi / panel hub ile ayni soft disli.
 */
static void build_panel_motors_page(void)
{
    const hexnet_io_profile_t *p = hexnet_io_profile_get();
    const int gap = 12;
    const int ox = 10;
    const int oy = 36;
    const int card_w = (CONT_W - 2 * ox - gap) / 2;
    const int card_h = PAGE_H - oy - 18;
    const lv_img_dsc_t *gear = vango_dash_motor_icon(true);
    int i;
    lv_obj_t *hint;

    panel_detail_header(PANEL_CAT_MOTOR, "MOTORS", "MOTORLAR", "MOTOREN");
    hint = make_label(s_content,
                      vango_tx("Enable channels here. Direction is on the Motor page.",
                               "Kanalı burada aç/kapa. Yön kontrolü Motor sayfasında.",
                               "Kanäle hier aktivieren. Richtung auf Motor-Seite."),
                      &vango_font_12, set_muted());
    lv_obj_set_width(hint, CONT_W - 20);
    lv_label_set_long_mode(hint, LV_LABEL_LONG_DOT);
    lv_obj_set_pos(hint, 10, PAGE_H - 16);

    if (!p) {
        return;
    }

    for (i = 0; i < PANEL_MOTOR_SLOTS; i++) {
        bool on = (p->motor_enabled & (1u << i)) != 0;
        char title[24];
        lv_obj_t *card;
        lv_obj_t *ibox;
        lv_obj_t *ico;
        lv_obj_t *nm;
        lv_obj_t *sub;
        lv_obj_t *sw;

        snprintf(title, sizeof(title), vango_tx("Motor %d", "Motor %d", "Motor %d"), i + 1);

        card = lv_obj_create(s_content);
        s_motor_card[i] = card;
        lv_obj_set_size(card, card_w, card_h - 8);
        lv_obj_set_pos(card, ox + i * (card_w + gap), oy);
        style_card(card);
        lv_obj_set_style_radius(card, 12, 0);
        lv_obj_set_style_pad_all(card, 0, 0);
        lv_obj_clear_flag(card, LV_OBJ_FLAG_SCROLLABLE);

        sw = lv_switch_create(card);
        lv_obj_set_size(sw, 42, 24);
        lv_obj_align(sw, LV_ALIGN_TOP_RIGHT, -12, 12);
        if (on) {
            lv_obj_add_state(sw, LV_STATE_CHECKED);
        }
        lv_obj_set_style_bg_color(sw, lv_color_hex(COL_ACCENT), LV_PART_INDICATOR | LV_STATE_CHECKED);
        lv_obj_add_event_cb(sw, motor_sw_cb, LV_EVENT_VALUE_CHANGED, (void *)(intptr_t)i);

        nm = make_label(card, title, &vango_font_18, on ? set_text() : set_muted());
        lv_obj_align(nm, LV_ALIGN_TOP_LEFT, 14, 14);
        s_motor_nm[i] = nm;

        ibox = lv_obj_create(card);
        lv_obj_set_size(ibox, 72, 72);
        lv_obj_align(ibox, LV_ALIGN_CENTER, 0, -4);
        lv_obj_set_style_bg_opa(ibox, LV_OPA_TRANSP, 0);
        lv_obj_set_style_border_width(ibox, 0, 0);
        lv_obj_set_style_pad_all(ibox, 0, 0);
        lv_obj_clear_flag(ibox, LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_CLICKABLE);
        ico = lv_img_create(ibox);
        if (gear) {
            panel_button_apply_icon(ico, gear, 40);
        } else {
            lv_img_set_src(ico, &vango_nav_motor);
        }
        lv_obj_center(ico);
        s_motor_ico[i] = ico;

        sub = make_label(card,
                         vango_tx("Panel channel", "Panel kanalı", "Panelkanal"),
                         &vango_font_12, set_muted());
        lv_obj_align(sub, LV_ALIGN_BOTTOM_MID, 0, -14);

        motor_card_paint(i, on);
    }
}

static void sensor_sw_cb(lv_event_t *e)
{
    int idx = (int)(intptr_t)lv_event_get_user_data(e);
    lv_obj_t *sw = lv_event_get_target(e);
    bool on;

    if (idx < 0 || idx >= PANEL_SENSOR_SLOTS || !sw) {
        return;
    }
    on = lv_obj_has_state(sw, LV_STATE_CHECKED);
    hexnet_io_profile_set_sensor(idx, on);
    panel_card_set_active(s_sensor_card[idx], s_sensor_ico[idx], s_sensor_nm[idx], NULL, on);
    panel_cat_title_refresh(PANEL_CAT_SENSOR, "SENSORS", "SENSÖRLER", "SENSOREN");
}

/*
 * Sensorler: dikey zengin satirlar — ikon + isim + aciklama + enable.
 * Canli deger / kalibrasyon protokolde yok; sadece kanal varligi.
 */
static void build_panel_sensors_page(void)
{
    static const char *sens_tr[] = {"Sıcaklık", "Nem", "Hareket", "Gaz", "Duman"};
    static const char *sens_en[] = {"Temperature", "Humidity", "Motion", "Gas", "Smoke"};
    static const char *sens_de[] = {"Temperatur", "Feuchte", "Bewegung", "Gas", "Rauch"};
    static const char *hint_tr[] = {
        "IO sıcaklık kanalı", "IO nem kanalı", "Hareket algılama", "Gaz dedektörü", "Duman dedektörü"
    };
    static const char *hint_en[] = {
        "IO temperature channel", "IO humidity channel", "Motion detect", "Gas detector", "Smoke detector"
    };
    static const char *hint_de[] = {
        "IO-Temperaturkanal", "IO-Feuchtekanal", "Bewegung", "Gasdetektor", "Rauchmelder"
    };
    const hexnet_io_profile_t *p = hexnet_io_profile_get();
    const hexnet_io_labels_t *L = hexnet_io_labels_get();
    const int row_h = 52;
    const int gap = 4;
    const int oy = 30;
    int i;
    lv_obj_t *list;

    panel_detail_header(PANEL_CAT_SENSOR, "SENSORS", "SENSÖRLER", "SENSOREN");
    if (!p) {
        return;
    }

    list = lv_obj_create(s_content);
    lv_obj_set_size(list, CONT_W - 12, PAGE_H - oy - 4);
    lv_obj_set_pos(list, 6, oy);
    lv_obj_set_style_bg_opa(list, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(list, 0, 0);
    lv_obj_set_style_pad_all(list, 0, 0);
    lv_obj_set_style_pad_row(list, gap, 0);
    lv_obj_set_style_pad_right(list, 10, 0);
    lv_obj_set_flex_flow(list, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_scroll_dir(list, LV_DIR_VER);
    lv_obj_add_flag(list, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_scrollbar_mode(list, LV_SCROLLBAR_MODE_ON);
    lv_obj_set_style_width(list, 8, LV_PART_SCROLLBAR);
    lv_obj_set_style_bg_color(list, lv_color_hex(set_accent()), LV_PART_SCROLLBAR);
    lv_obj_set_style_bg_opa(list, LV_OPA_COVER, LV_PART_SCROLLBAR);

    for (i = 0; i < PANEL_SENSOR_SLOTS; i++) {
        bool on = (p->sensor_enable_mask & (1u << i)) != 0;
        char name[32];
        lv_obj_t *card;
        lv_obj_t *ibox;
        lv_obj_t *ico;
        lv_obj_t *nm;
        lv_obj_t *hint;
        lv_obj_t *sw;
        const lv_img_dsc_t *isrc = sensor_slot_icon(i);

        if (L && i < HEXNET_LABEL_SENSORS && L->sensors[i].name[0]) {
            snprintf(name, sizeof(name), "%s", L->sensors[i].name);
        } else {
            snprintf(name, sizeof(name), "%s", vango_tx(sens_en[i], sens_tr[i], sens_de[i]));
        }

        card = lv_obj_create(list);
        s_sensor_card[i] = card;
        lv_obj_set_size(card, CONT_W - 28, row_h);
        style_card(card);
        lv_obj_set_style_radius(card, 10, 0);
        lv_obj_set_style_pad_all(card, 0, 0);
        lv_obj_clear_flag(card, LV_OBJ_FLAG_SCROLLABLE);

        ibox = lv_obj_create(card);
        lv_obj_set_size(ibox, 40, 40);
        lv_obj_align(ibox, LV_ALIGN_LEFT_MID, 8, 0);
        lv_obj_set_style_bg_opa(ibox, LV_OPA_TRANSP, 0);
        lv_obj_set_style_border_width(ibox, 0, 0);
        lv_obj_set_style_pad_all(ibox, 0, 0);
        lv_obj_clear_flag(ibox, LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_CLICKABLE);
        ico = lv_img_create(ibox);
        if (isrc) {
            panel_button_apply_icon(ico, isrc, 40);
        }
        lv_obj_center(ico);
        s_sensor_ico[i] = ico;

        nm = make_label(card, name, &vango_font_16, on ? set_text() : set_muted());
        lv_obj_align(nm, LV_ALIGN_LEFT_MID, 56, -8);
        s_sensor_nm[i] = nm;

        hint = make_label(card, vango_tx(hint_en[i], hint_tr[i], hint_de[i]),
                          &vango_font_12, set_muted());
        lv_obj_set_width(hint, CONT_W - 160);
        lv_label_set_long_mode(hint, LV_LABEL_LONG_DOT);
        lv_obj_align(hint, LV_ALIGN_LEFT_MID, 56, 10);

        sw = lv_switch_create(card);
        lv_obj_set_size(sw, 42, 24);
        lv_obj_align(sw, LV_ALIGN_RIGHT_MID, -12, 0);
        if (on) {
            lv_obj_add_state(sw, LV_STATE_CHECKED);
        }
        lv_obj_set_style_bg_color(sw, lv_color_hex(COL_ACCENT), LV_PART_INDICATOR | LV_STATE_CHECKED);
        lv_obj_add_event_cb(sw, sensor_sw_cb, LV_EVENT_VALUE_CHANGED, (void *)(intptr_t)i);

        panel_card_set_active(card, ico, nm, NULL, on);
    }
}

static void water_sw_cb(lv_event_t *e)
{
    int idx = (int)(intptr_t)lv_event_get_user_data(e);
    lv_obj_t *sw = lv_event_get_target(e);
    bool on;

    if (idx < 0 || idx >= PANEL_WATER_SLOTS || !sw) {
        return;
    }
    on = lv_obj_has_state(sw, LV_STATE_CHECKED);
    hexnet_io_profile_set_tank(idx, on);
    panel_card_set_active(s_water_card[idx], s_water_ico[idx], s_water_nm[idx], s_water_cap[idx], on);
    panel_cat_title_refresh(PANEL_CAT_WATER, "WATER LEVEL", "SU SEVİYE", "WASSERSTAND");
}

static void water_cap_cb(lv_event_t *e)
{
    int idx = (int)(intptr_t)lv_event_get_user_data(e);
    open_tank_liter(idx);
}

static void water_refresh_cap_labels(void)
{
    int i;
    for (i = 0; i < PANEL_WATER_SLOTS; i++) {
        char cap_txt[24];
        int cap;
        if (!s_water_cap[i]) {
            continue;
        }
        cap = vango_dash_tank_capacity(i);
        if (cap > 0) {
            snprintf(cap_txt, sizeof(cap_txt), "%d L", cap);
        } else {
            snprintf(cap_txt, sizeof(cap_txt), "%s",
                     vango_tx("Set liters", "Litre gir", "Liter"));
        }
        lv_label_set_text(s_water_cap[i], cap_txt);
    }
}

/*
 * Su seviye: 3 tank karti (Temiz / Atik / Gri).
 * Enable + yerel kapasite (litre NVS); seviye gosterimi Ana Sayfa/Su.
 */
static void build_panel_water_page(void)
{
    static const char *tank_tr[] = {"Temiz Su", "Atık Su", "Gri Su"};
    static const char *tank_en[] = {"Fresh Water", "Waste Water", "Grey Water"};
    static const char *tank_de[] = {"Frischwasser", "Abwasser", "Grauwasser"};
    const hexnet_io_profile_t *p = hexnet_io_profile_get();
    const hexnet_io_labels_t *L = hexnet_io_labels_get();
    const int gap = 8;
    const int ox = 8;
    const int oy = 30;
    const int card_w = (CONT_W - 2 * ox - 2 * gap) / 3;
    const int card_h = PAGE_H - oy - 20;
    int i;
    lv_obj_t *hint;

    panel_detail_header(PANEL_CAT_WATER, "WATER LEVEL", "SU SEVİYE", "WASSERSTAND");
    hint = make_label(s_content,
                      vango_tx("Capacity is local. Live levels are on Water page.",
                               "Kapasite yerel. Canlı seviye Su sayfasında.",
                               "Kapazität lokal. Füllstände auf Wasser-Seite."),
                      &vango_font_12, set_muted());
    lv_obj_set_width(hint, CONT_W - 16);
    lv_label_set_long_mode(hint, LV_LABEL_LONG_DOT);
    lv_obj_set_pos(hint, 8, PAGE_H - 16);

    if (!p) {
        return;
    }

    for (i = 0; i < PANEL_WATER_SLOTS; i++) {
        bool on = (p->tank_enable_mask & (1u << i)) != 0;
        char name[32];
        char cap_txt[24];
        int cap = vango_dash_tank_capacity(i);
        lv_obj_t *card;
        lv_obj_t *ibox;
        lv_obj_t *ico;
        lv_obj_t *nm;
        lv_obj_t *cap_btn;
        lv_obj_t *cap_lbl;
        lv_obj_t *sw;
        const lv_img_dsc_t *isrc = water_slot_icon(i);

        if (L && i < HEXNET_LABEL_WATER && L->water[i].name[0]) {
            snprintf(name, sizeof(name), "%s", L->water[i].name);
        } else {
            snprintf(name, sizeof(name), "%s", vango_tx(tank_en[i], tank_tr[i], tank_de[i]));
        }
        if (cap > 0) {
            snprintf(cap_txt, sizeof(cap_txt), "%d L", cap);
        } else {
            snprintf(cap_txt, sizeof(cap_txt), "%s",
                     vango_tx("Set liters", "Litre gir", "Liter"));
        }

        card = lv_obj_create(s_content);
        s_water_card[i] = card;
        lv_obj_set_size(card, card_w, card_h - 4);
        lv_obj_set_pos(card, ox + i * (card_w + gap), oy);
        style_card(card);
        lv_obj_set_style_radius(card, 12, 0);
        lv_obj_set_style_pad_all(card, 0, 0);
        lv_obj_clear_flag(card, LV_OBJ_FLAG_SCROLLABLE);

        sw = lv_switch_create(card);
        lv_obj_set_size(sw, 36, 20);
        lv_obj_align(sw, LV_ALIGN_TOP_RIGHT, -8, 8);
        if (on) {
            lv_obj_add_state(sw, LV_STATE_CHECKED);
        }
        lv_obj_set_style_bg_color(sw, lv_color_hex(COL_ACCENT), LV_PART_INDICATOR | LV_STATE_CHECKED);
        lv_obj_add_event_cb(sw, water_sw_cb, LV_EVENT_VALUE_CHANGED, (void *)(intptr_t)i);

        nm = make_label(card, name, &vango_font_14, on ? set_text() : set_muted());
        lv_obj_set_width(nm, card_w - 50);
        lv_label_set_long_mode(nm, LV_LABEL_LONG_DOT);
        lv_obj_align(nm, LV_ALIGN_TOP_LEFT, 10, 10);
        s_water_nm[i] = nm;

        ibox = lv_obj_create(card);
        lv_obj_set_size(ibox, 56, 56);
        lv_obj_align(ibox, LV_ALIGN_CENTER, 0, -10);
        lv_obj_set_style_bg_opa(ibox, LV_OPA_TRANSP, 0);
        lv_obj_set_style_border_width(ibox, 0, 0);
        lv_obj_set_style_pad_all(ibox, 0, 0);
        lv_obj_clear_flag(ibox, LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_CLICKABLE);
        ico = lv_img_create(ibox);
        if (isrc) {
            panel_button_apply_icon(ico, isrc, 40);
        }
        lv_obj_center(ico);
        s_water_ico[i] = ico;

        cap_btn = lv_btn_create(card);
        lv_obj_set_size(cap_btn, card_w - 20, 32);
        lv_obj_align(cap_btn, LV_ALIGN_BOTTOM_MID, 0, -12);
        lv_obj_set_style_radius(cap_btn, 8, 0);
        lv_obj_set_style_shadow_width(cap_btn, 0, 0);
        lv_obj_set_style_bg_color(cap_btn, lv_color_hex(COL_BTN_IDLE), 0);
        lv_obj_add_event_cb(cap_btn, water_cap_cb, LV_EVENT_CLICKED, (void *)(intptr_t)i);
        cap_lbl = make_label(cap_btn, cap_txt, &vango_font_14, set_text());
        lv_obj_center(cap_lbl);
        s_water_cap[i] = cap_lbl;

        panel_card_set_active(card, ico, nm, NULL, on);
    }
}

static void build_panel_rgb_page(void)
{
    const hexnet_io_profile_t *p = hexnet_io_profile_get();
    bool on;
    lv_obj_t *card;
    lv_obj_t *sw;
    uintptr_t pack_sw;

    panel_detail_header(PANEL_CAT_RGB, "RGB LIGHTING", "RGB AYDINLATMA", "RGB-LICHT");
    if (!p) {
        return;
    }
    on = p->rgb_enabled != 0;
    pack_sw = ((uintptr_t)PANEL_CAT_RGB << 16);

    card = lv_obj_create(s_content);
    lv_obj_set_size(card, CONT_W - 24, 120);
    lv_obj_set_pos(card, 12, 40);
    style_card(card);
    lv_obj_set_style_radius(card, 12, 0);

    make_label(card, vango_tx("RGB strip", "RGB şerit", "RGB-Streifen"),
               &vango_font_16, set_text());
    lv_obj_align(lv_obj_get_child(card, 0), LV_ALIGN_LEFT_MID, 16, -12);
    make_label(card,
               vango_tx("Color control is on the RGB page.",
                        "Renk kontrolü RGB sayfasında.",
                        "Farbe auf der RGB-Seite."),
               &vango_font_12, set_muted());
    lv_obj_align(lv_obj_get_child(card, 1), LV_ALIGN_LEFT_MID, 16, 12);

    make_rgb_mark(card, 120);

    sw = lv_switch_create(card);
    lv_obj_set_size(sw, 46, 26);
    lv_obj_align(sw, LV_ALIGN_RIGHT_MID, -16, 0);
    if (on) {
        lv_obj_add_state(sw, LV_STATE_CHECKED);
    }
    lv_obj_set_style_bg_color(sw, lv_color_hex(COL_ACCENT), LV_PART_INDICATOR | LV_STATE_CHECKED);
    lv_obj_add_event_cb(sw, panel_slot_cb, LV_EVENT_VALUE_CHANGED, (void *)pack_sw);
    panel_card_set_active(card, NULL, NULL, NULL, on);
}

static void build_panel_detail(panel_cat_t cat)
{
    switch (cat) {
        case PANEL_CAT_BUTTON:
            build_panel_buttons_page();
            break;
        case PANEL_CAT_LIGHT:
            build_panel_lights_page();
            break;
        case PANEL_CAT_MOTOR:
            build_panel_motors_page();
            break;
        case PANEL_CAT_SENSOR:
            build_panel_sensors_page();
            break;
        case PANEL_CAT_WATER:
            build_panel_water_page();
            break;
        case PANEL_CAT_RGB:
            build_panel_rgb_page();
            break;
        default:
            break;
    }
}

/* ---------- Screen ---------- */
static void wallpaper_sw_cb(lv_event_t *e)
{
    lv_obj_t *sw = lv_event_get_target(e);
    panelWallpaperEnable = lv_obj_has_state(sw, LV_STATE_CHECKED) ? 1 : 0;
    commit_theme_language_settings();
    show_section(SET_SEC_SCREEN);
}

static void demo_sw_cb(lv_event_t *e)
{
    lv_obj_t *sw = lv_event_get_target(e);
    bool on = lv_obj_has_state(sw, LV_STATE_CHECKED);
    if (on && get_canbus_connection_status() != 0) {
        lv_obj_clear_state(sw, LV_STATE_CHECKED);
        on = false;
    }
    vango_dash_set_demo(on);
    show_section(SET_SEC_DEMO);
}

static void timeout_btn_cb(lv_event_t *e)
{
    int sec = (int)(intptr_t)lv_event_get_user_data(e);
    panelWallpaperTime = sec;
    commit_theme_language_settings();
    show_section(SET_SEC_SCREEN);
}

static void brightness_slider_cb(lv_event_t *e)
{
    lv_obj_t *sld = lv_event_get_target(e);
    lv_obj_t *pct = lv_event_get_user_data(e);
    int v = (int)lv_slider_get_value(sld);
    char buf[12];
    vango_dash_set_brightness(v);
    if (pct) {
        snprintf(buf, sizeof(buf), "%d%%", v);
        lv_label_set_text(pct, buf);
    }
}

static void hard_reset_cb(lv_event_t *e)
{
    uint8_t data[8] = {0};
    (void)e;
    /* 0x770 data[0]=2 → IO GPIO46 Display 5V OFF 3s then ON */
    data[0] = 2;
    send_can_frame(VANGO_CAN_FRAME_DISP_PWR, data);
    ESP_LOGW(TAG, "Hard reset TX 0x770 (Display 5V pulse)");
}

static void build_screen_section(void)
{
    int bright = vango_dash_get_brightness();
    char pct_buf[12];
    lv_obj_t *pct;
    const bool can_up = get_canbus_connection_status() != 0;

    make_label(s_content, vango_tx("DISPLAY", "EKRAN", "DISPLAY"), &vango_font_12, COL_ACCENT);
    lv_obj_set_pos(lv_obj_get_child(s_content, 0), 8, 4);

    lv_obj_t *card = lv_obj_create(s_content);
    lv_obj_set_size(card, CONT_W - 20, 310);
    lv_obj_set_pos(card, 8, 28);
    style_card(card);

    make_label(card, vango_tx("Brightness", "Parlaklık", "Helligkeit"), &vango_font_14, COL_TEXT);
    lv_obj_set_pos(lv_obj_get_child(card, 0), 16, 14);

    snprintf(pct_buf, sizeof(pct_buf), "%d%%", bright);
    pct = make_label(card, pct_buf, &vango_font_14, COL_ACCENT);
    lv_obj_align(pct, LV_ALIGN_TOP_RIGHT, -16, 14);

    lv_obj_t *sld = lv_slider_create(card);
    lv_obj_set_size(sld, CONT_W - 80, 12);
    lv_obj_set_pos(sld, 16, 48);
    lv_slider_set_range(sld, 10, 100);
    lv_slider_set_value(sld, bright, LV_ANIM_OFF);
    lv_obj_set_style_bg_color(sld, lv_color_hex(COL_ACCENT), LV_PART_INDICATOR);
    lv_obj_set_style_bg_color(sld, lv_color_hex(COL_ACCENT), LV_PART_KNOB);
    lv_obj_add_event_cb(sld, brightness_slider_cb, LV_EVENT_VALUE_CHANGED, pct);

    make_label(card, vango_tx("Screensaver", "Ekran Koruyucu", "Bildschirmschoner"), &vango_font_14, COL_TEXT);
    lv_obj_set_pos(lv_obj_get_child(card, lv_obj_get_child_cnt(card) - 1), 16, 84);

    lv_obj_t *sw = lv_switch_create(card);
    lv_obj_set_size(sw, 48, 26);
    lv_obj_align(sw, LV_ALIGN_TOP_RIGHT, -16, 82);
    if (panelWallpaperEnable) {
        lv_obj_add_state(sw, LV_STATE_CHECKED);
    }
    lv_obj_set_style_bg_color(sw, lv_color_hex(COL_ACCENT), LV_PART_INDICATOR | LV_STATE_CHECKED);
    lv_obj_add_event_cb(sw, wallpaper_sw_cb, LV_EVENT_VALUE_CHANGED, NULL);

    make_label(card, vango_tx("Timeout", "Zaman aşımı", "Zeitüberschreitung"), &vango_font_14, COL_MUTED);
    lv_obj_set_pos(lv_obj_get_child(card, lv_obj_get_child_cnt(card) - 1), 16, 128);

    static const int times[] = {30, 60, 120, 300, 600};
    const char *labels_tr[] = {"30 sn", "1 dk", "2 dk", "5 dk", "10 dk"};
    const char *labels_en[] = {"30 s", "1 m", "2 m", "5 m", "10 m"};
    const char *labels_de[] = {"30 s", "1 Min", "2 Min", "5 Min", "10 Min"};
    const char **labels = vango_is_tr() ? labels_tr : (vango_is_de() ? labels_de : labels_en);
    for (int i = 0; i < 5; i++) {
        bool on = (panelWallpaperTime == times[i]);
        lv_obj_t *b = lv_btn_create(card);
        lv_obj_set_size(b, 90, 34);
        lv_obj_set_pos(b, 16 + i * 100, 160);
        lv_obj_set_style_radius(b, 8, 0);
        lv_obj_set_style_shadow_width(b, 0, 0);
        lv_obj_set_style_bg_color(b, lv_color_hex(on ? COL_ACCENT : COL_BTN_IDLE), 0);
        lv_obj_t *t = make_label(b, labels[i], &vango_font_12, on ? COL_ON_ACCENT : COL_TEXT);
        lv_obj_center(t);
        lv_obj_add_event_cb(b, timeout_btn_cb, LV_EVENT_CLICKED, (void *)(intptr_t)times[i]);
    }

    make_label(card, vango_tx("Power", "Güç", "Strom"), &vango_font_14, COL_TEXT);
    lv_obj_set_pos(lv_obj_get_child(card, lv_obj_get_child_cnt(card) - 1), 16, 212);
    make_label(card,
               can_up
                   ? vango_tx("Cuts Display 5V for 3 s", "Display 5V'yi 3 sn keser", "Display-5V 3 s aus")
                   : vango_tx("Needs CAN link", "CAN bağlantısı gerekli", "CAN-Verbindung nötig"),
               &vango_font_12, COL_MUTED);
    lv_obj_set_pos(lv_obj_get_child(card, lv_obj_get_child_cnt(card) - 1), 16, 236);

    lv_obj_t *rst = lv_btn_create(card);
    lv_obj_set_size(rst, 200, 40);
    lv_obj_align(rst, LV_ALIGN_TOP_RIGHT, -16, 220);
    lv_obj_set_style_radius(rst, 8, 0);
    lv_obj_set_style_shadow_width(rst, 0, 0);
    lv_obj_set_style_bg_color(rst, lv_color_hex(can_up ? COL_ORANGE : COL_BTN_IDLE), 0);
    lv_obj_t *rst_lbl = make_label(rst,
                                   vango_tx("Hard Reset", "Hard Reset", "Hard Reset"),
                                   &vango_font_14, COL_TEXT);
    lv_obj_center(rst_lbl);
    if (!can_up) {
        lv_obj_add_state(rst, LV_STATE_DISABLED);
    } else {
        lv_obj_add_event_cb(rst, hard_reset_cb, LV_EVENT_CLICKED, NULL);
    }
}

/* ---------- Theme ---------- */
static void theme_commit_async(void *user)
{
    (void)user;
    s_sec = SET_SEC_THEME;
    commit_theme_language_settings();
}

static void theme_pick_cb(lv_event_t *e)
{
    int mode = (int)(intptr_t)lv_event_get_user_data(e);
    /* 0=light lock, 1=dark lock, 2=system (follow IO) */
    panelThemeType = mode;
    /* Event icinde widget silmek reset yapar — async uygula */
    lv_async_call(theme_commit_async, NULL);
}

static void accent_swatches_paint(void)
{
    for (int i = 0; i < VANGO_ACCENT_N; i++) {
        if (!s_accent_sw[i]) {
            continue;
        }
        bool on = (panelAccentIdx == i);
        lv_obj_set_style_border_width(s_accent_sw[i], on ? 3 : 1, 0);
        lv_obj_set_style_border_color(s_accent_sw[i],
                                      lv_color_hex(on ? COL_TEXT : COL_CARD_BORDER), 0);
    }
    for (int i = 0; i < 3; i++) {
        if (!s_theme_mode_card[i]) {
            continue;
        }
        bool on = (panelThemeType == i);
        lv_obj_set_style_border_width(s_theme_mode_card[i], on ? 2 : 1, 0);
        lv_obj_set_style_border_color(s_theme_mode_card[i],
                                      lv_color_hex(on ? COL_ACCENT : COL_CARD_BORDER), 0);
    }
    side_set_active(s_sec);
}

static void accent_chrome_async(void *user)
{
    (void)user;
    vango_dash_on_accent_changed();
}

static void accent_pick_cb(lv_event_t *e)
{
    int idx = (int)(intptr_t)lv_event_get_user_data(e);
    if (idx < 0 || idx >= VANGO_ACCENT_N) {
        idx = 0;
    }
    if (panelAccentIdx == idx) {
        return;
    }
    panelAccentIdx = idx;
    /* Yerinde boya — show_section/clean YOK (IWDT tetikliyordu). */
    accent_swatches_paint();
    persist_theme_settings();
    lv_async_call(accent_chrome_async, NULL);
}

static void build_theme_section(void)
{
    memset(s_accent_sw, 0, sizeof(s_accent_sw));
    memset(s_theme_mode_card, 0, sizeof(s_theme_mode_card));

    make_label(s_content, vango_tx("THEME", "TEMA", "THEMA"), &vango_font_12, COL_ACCENT);
    lv_obj_set_pos(lv_obj_get_child(s_content, 0), 8, 4);

    static const char *names_tr[] = {"Açık", "Koyu", "Sistem"};
    static const char *names_en[] = {"Light", "Dark", "System"};
    static const char *names_de[] = {"Hell", "Dunkel", "System"};
    const char **names = vango_is_tr() ? names_tr : (vango_is_de() ? names_de : names_en);
    for (int i = 0; i < 3; i++) {
        bool on = (panelThemeType == i);
        lv_obj_t *card = lv_obj_create(s_content);
        s_theme_mode_card[i] = card;
        lv_obj_set_size(card, 170, 110);
        lv_obj_set_pos(card, 8 + i * 185, 36);
        style_card(card);
        lv_obj_set_style_border_width(card, on ? 2 : 1, 0);
        lv_obj_set_style_border_color(card, lv_color_hex(on ? COL_ACCENT : COL_CARD_BORDER), 0);
        lv_obj_add_flag(card, LV_OBJ_FLAG_CLICKABLE);
        lv_obj_add_event_cb(card, theme_pick_cb, LV_EVENT_CLICKED, (void *)(intptr_t)i);

        lv_obj_t *swatch = lv_obj_create(card);
        lv_obj_set_size(swatch, 140, 48);
        lv_obj_align(swatch, LV_ALIGN_TOP_MID, 0, 12);
        lv_obj_set_style_radius(swatch, 8, 0);
        lv_obj_set_style_border_width(swatch, 0, 0);
        if (i == 0) {
            lv_obj_set_style_bg_color(swatch, lv_color_hex(0xF1F5F9), 0);
        } else if (i == 1) {
            lv_obj_set_style_bg_color(swatch, lv_color_hex(0x0B1220), 0);
        } else {
            /* System: split light/dark swatch */
            lv_obj_set_style_bg_color(swatch, lv_color_hex(0x334155), 0);
            lv_obj_t *half = lv_obj_create(swatch);
            lv_obj_set_size(half, 70, 48);
            lv_obj_align(half, LV_ALIGN_LEFT_MID, 0, 0);
            lv_obj_set_style_bg_color(half, lv_color_hex(0xF1F5F9), 0);
            lv_obj_set_style_border_width(half, 0, 0);
            lv_obj_set_style_radius(half, 0, 0);
            lv_obj_clear_flag(half, LV_OBJ_FLAG_CLICKABLE);
        }
        lv_obj_clear_flag(swatch, LV_OBJ_FLAG_CLICKABLE);

        lv_obj_t *lbl = make_label(card, names[i], &vango_font_14, COL_TEXT);
        lv_obj_align(lbl, LV_ALIGN_BOTTOM_MID, 0, -10);
    }

    int accent_y = 170;
    if (panelThemeType == 2) {
        extern int io_sot_theme;
        make_label(s_content,
                   io_sot_theme
                       ? vango_tx("Now: Dark (from IO)", "Şu an: Koyu (IO)", "Jetzt: Dunkel (von IO)")
                       : vango_tx("Now: Light (from IO)", "Şu an: Açık (IO)", "Jetzt: Hell (von IO)"),
                   &vango_font_12, COL_MUTED);
        lv_obj_set_pos(lv_obj_get_child(s_content, lv_obj_get_child_cnt(s_content) - 1), 8, 158);
        accent_y = 190;
    }

    make_label(s_content, vango_tx("Accent color", "Renk kombinasyonu", "Akzentfarbe"), &vango_font_12, COL_MUTED);
    lv_obj_set_pos(lv_obj_get_child(s_content, lv_obj_get_child_cnt(s_content) - 1), 8, accent_y);

    for (int i = 0; i < VANGO_ACCENT_N; i++) {
        bool on = (panelAccentIdx == i);
        lv_obj_t *c = lv_obj_create(s_content);
        s_accent_sw[i] = c;
        lv_obj_set_size(c, 40, 40);
        lv_obj_set_pos(c, 8 + i * 52, accent_y + 28);
        lv_obj_set_style_radius(c, LV_RADIUS_CIRCLE, 0);
        lv_obj_set_style_bg_color(c, lv_color_hex(vango_accent_at(i)), 0);
        lv_obj_set_style_border_width(c, on ? 3 : 1, 0);
        lv_obj_set_style_border_color(c, lv_color_hex(on ? COL_TEXT : COL_CARD_BORDER), 0);
        lv_obj_set_style_pad_all(c, 0, 0);
        lv_obj_clear_flag(c, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_add_flag(c, LV_OBJ_FLAG_CLICKABLE);
        lv_obj_add_event_cb(c, accent_pick_cb, LV_EVENT_CLICKED, (void *)(intptr_t)i);
    }
}

/* ---------- Language ---------- */
static void lang_commit_async(void *user)
{
    (void)user;
    s_sec = SET_SEC_LANG;
    commit_theme_language_settings();
}

static void lang_pick_cb(lv_event_t *e)
{
    int lang = (int)(intptr_t)lv_event_get_user_data(e);
    /* 0=EN lock, 1=TR lock, 2=system (follow IO), 3=DE lock */
    panelLanguageType = lang;
    /* Event icinde widget silmek reset yapar — async uygula */
    lv_async_call(lang_commit_async, NULL);
}

/** Mini drawn flags — font has no emoji glyphs. lang: 2=sys, 1=TR, 0=EN, 3=DE */
static void add_lang_flag(lv_obj_t *parent, int lang)
{
    const int fw = 28;
    const int fh = 18;
    lv_obj_t *flag = lv_obj_create(parent);
    lv_obj_set_size(flag, fw, fh);
    lv_obj_align(flag, LV_ALIGN_LEFT_MID, 48, 0);
    lv_obj_set_style_radius(flag, 3, 0);
    lv_obj_set_style_border_width(flag, 1, 0);
    lv_obj_set_style_border_color(flag, lv_color_hex(0x64748B), 0);
    lv_obj_set_style_pad_all(flag, 0, 0);
    lv_obj_set_style_clip_corner(flag, true, 0);
    lv_obj_clear_flag(flag, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);

    if (lang == 1) {
        /* Turkey: red + white crescent */
        lv_obj_set_style_bg_color(flag, lv_color_hex(0xE30A17), 0);
        lv_obj_t *cres = lv_obj_create(flag);
        lv_obj_set_size(cres, 10, 10);
        lv_obj_set_pos(cres, 7, 4);
        lv_obj_set_style_radius(cres, LV_RADIUS_CIRCLE, 0);
        lv_obj_set_style_bg_color(cres, lv_color_hex(0xFFFFFF), 0);
        lv_obj_set_style_border_width(cres, 0, 0);
        lv_obj_clear_flag(cres, LV_OBJ_FLAG_CLICKABLE);
        lv_obj_t *cut = lv_obj_create(flag);
        lv_obj_set_size(cut, 8, 8);
        lv_obj_set_pos(cut, 11, 5);
        lv_obj_set_style_radius(cut, LV_RADIUS_CIRCLE, 0);
        lv_obj_set_style_bg_color(cut, lv_color_hex(0xE30A17), 0);
        lv_obj_set_style_border_width(cut, 0, 0);
        lv_obj_clear_flag(cut, LV_OBJ_FLAG_CLICKABLE);
        lv_obj_t *star = lv_obj_create(flag);
        lv_obj_set_size(star, 4, 4);
        lv_obj_set_pos(star, 18, 7);
        lv_obj_set_style_radius(star, LV_RADIUS_CIRCLE, 0);
        lv_obj_set_style_bg_color(star, lv_color_hex(0xFFFFFF), 0);
        lv_obj_set_style_border_width(star, 0, 0);
        lv_obj_clear_flag(star, LV_OBJ_FLAG_CLICKABLE);
    } else if (lang == 0) {
        /* UK simplified: blue field + red/white cross */
        lv_obj_set_style_bg_color(flag, lv_color_hex(0x012169), 0);
        lv_obj_t *wx = lv_obj_create(flag);
        lv_obj_set_size(wx, fw, 6);
        lv_obj_align(wx, LV_ALIGN_CENTER, 0, 0);
        lv_obj_set_style_bg_color(wx, lv_color_hex(0xFFFFFF), 0);
        lv_obj_set_style_border_width(wx, 0, 0);
        lv_obj_set_style_radius(wx, 0, 0);
        lv_obj_clear_flag(wx, LV_OBJ_FLAG_CLICKABLE);
        lv_obj_t *wy = lv_obj_create(flag);
        lv_obj_set_size(wy, 6, fh);
        lv_obj_align(wy, LV_ALIGN_CENTER, 0, 0);
        lv_obj_set_style_bg_color(wy, lv_color_hex(0xFFFFFF), 0);
        lv_obj_set_style_border_width(wy, 0, 0);
        lv_obj_set_style_radius(wy, 0, 0);
        lv_obj_clear_flag(wy, LV_OBJ_FLAG_CLICKABLE);
        lv_obj_t *rx = lv_obj_create(flag);
        lv_obj_set_size(rx, fw, 3);
        lv_obj_align(rx, LV_ALIGN_CENTER, 0, 0);
        lv_obj_set_style_bg_color(rx, lv_color_hex(0xC8102E), 0);
        lv_obj_set_style_border_width(rx, 0, 0);
        lv_obj_set_style_radius(rx, 0, 0);
        lv_obj_clear_flag(rx, LV_OBJ_FLAG_CLICKABLE);
        lv_obj_t *ry = lv_obj_create(flag);
        lv_obj_set_size(ry, 3, fh);
        lv_obj_align(ry, LV_ALIGN_CENTER, 0, 0);
        lv_obj_set_style_bg_color(ry, lv_color_hex(0xC8102E), 0);
        lv_obj_set_style_border_width(ry, 0, 0);
        lv_obj_set_style_radius(ry, 0, 0);
        lv_obj_clear_flag(ry, LV_OBJ_FLAG_CLICKABLE);
    } else if (lang == 3) {
        /* Germany: black / red / gold */
        static const uint32_t bands[3] = {0x000000u, 0xDD0000u, 0xFFCE00u};
        for (int b = 0; b < 3; b++) {
            lv_obj_t *band = lv_obj_create(flag);
            lv_obj_set_size(band, fw, fh / 3 + (b == 2 ? fh % 3 : 0));
            lv_obj_set_pos(band, 0, b * (fh / 3));
            lv_obj_set_style_bg_color(band, lv_color_hex(bands[b]), 0);
            lv_obj_set_style_border_width(band, 0, 0);
            lv_obj_set_style_radius(band, 0, 0);
            lv_obj_clear_flag(band, LV_OBJ_FLAG_CLICKABLE);
        }
        lv_obj_set_style_bg_opa(flag, LV_OPA_TRANSP, 0);
    } else {
        /* System: globe — blue disc + meridian / parallel */
        lv_obj_set_style_bg_color(flag, lv_color_hex(0x0EA5E9), 0);
        lv_obj_t *mer = lv_obj_create(flag);
        lv_obj_set_size(mer, 2, fh - 4);
        lv_obj_align(mer, LV_ALIGN_CENTER, 0, 0);
        lv_obj_set_style_bg_color(mer, lv_color_hex(0xFFFFFF), 0);
        lv_obj_set_style_border_width(mer, 0, 0);
        lv_obj_set_style_radius(mer, 0, 0);
        lv_obj_clear_flag(mer, LV_OBJ_FLAG_CLICKABLE);
        lv_obj_t *par = lv_obj_create(flag);
        lv_obj_set_size(par, fw - 6, 2);
        lv_obj_align(par, LV_ALIGN_CENTER, 0, 0);
        lv_obj_set_style_bg_color(par, lv_color_hex(0xFFFFFF), 0);
        lv_obj_set_style_border_width(par, 0, 0);
        lv_obj_set_style_radius(par, 0, 0);
        lv_obj_clear_flag(par, LV_OBJ_FLAG_CLICKABLE);
        lv_obj_t *par2 = lv_obj_create(flag);
        lv_obj_set_size(par2, fw - 10, 2);
        lv_obj_align(par2, LV_ALIGN_CENTER, 0, -4);
        lv_obj_set_style_bg_color(par2, lv_color_hex(0xE0F2FE), 0);
        lv_obj_set_style_border_width(par2, 0, 0);
        lv_obj_set_style_radius(par2, 0, 0);
        lv_obj_clear_flag(par2, LV_OBJ_FLAG_CLICKABLE);
    }
}

static void build_lang_section(void)
{
    make_label(s_content, vango_tx("LANGUAGE", "DİL / LANGUAGE", "SPRACHE"), &vango_font_12, COL_ACCENT);
    lv_obj_set_pos(lv_obj_get_child(s_content, 0), 8, 4);

    /* Language names stay native; only "System" is localized. */
    const char *names[] = {
        vango_tx("System", "Sistem", "System"),
        "Türkçe",
        "English",
        "Deutsch",
    };
    static const int map[] = {2, 1, 0, 3};
    for (int i = 0; i < 4; i++) {
        bool on = (panelLanguageType == map[i]);
        lv_obj_t *row = lv_obj_create(s_content);
        lv_obj_set_size(row, CONT_W - 20, 48);
        lv_obj_set_pos(row, 8, 28 + i * 56);
        style_card(row);
        lv_obj_set_style_border_width(row, on ? 2 : 1, 0);
        lv_obj_set_style_border_color(row, lv_color_hex(on ? COL_ACCENT : COL_CARD_BORDER), 0);
        lv_obj_add_flag(row, LV_OBJ_FLAG_CLICKABLE);
        lv_obj_add_event_cb(row, lang_pick_cb, LV_EVENT_CLICKED, (void *)(intptr_t)map[i]);

        lv_obj_t *radio = lv_obj_create(row);
        lv_obj_set_size(radio, 20, 20);
        lv_obj_align(radio, LV_ALIGN_LEFT_MID, 12, 0);
        lv_obj_set_style_radius(radio, LV_RADIUS_CIRCLE, 0);
        lv_obj_set_style_border_width(radio, 2, 0);
        lv_obj_set_style_border_color(radio, lv_color_hex(on ? COL_ACCENT : COL_MUTED), 0);
        lv_obj_set_style_bg_color(radio, lv_color_hex(on ? COL_ACCENT : COL_CARD), 0);
        lv_obj_clear_flag(radio, LV_OBJ_FLAG_CLICKABLE);

        add_lang_flag(row, map[i]);

        lv_obj_t *lbl = make_label(row, names[i], &vango_font_16, COL_TEXT);
        lv_obj_align(lbl, LV_ALIGN_LEFT_MID, 86, 0);
    }
}

static void info_pair(lv_obj_t *card, int y, const char *key, const char *value, uint32_t value_color)
{
    make_label(card, key, &vango_font_12, COL_MUTED);
    lv_obj_set_pos(lv_obj_get_child(card, lv_obj_get_child_cnt(card) - 1), 16, y);
    make_label(card, value, &vango_font_16, value_color);
    lv_obj_set_pos(lv_obj_get_child(card, lv_obj_get_child_cnt(card) - 1), 16, y + 16);
}

/* ---------- Device info ---------- */
static void build_device_section(void)
{
    char line[96];
    char product[48];
    bool up = get_canbus_connection_status() != 0;

    make_label(s_content, vango_tx("DEVICE INFO", "CİHAZ BİLGİLERİ", "GERAETEINFO"), &vango_font_12, COL_ACCENT);
    lv_obj_set_pos(lv_obj_get_child(s_content, 0), 8, 4);

    lv_obj_t *card = lv_obj_create(s_content);
    lv_obj_set_size(card, CONT_W - 20, 314);
    lv_obj_set_pos(card, 8, 28);
    style_card(card);

    snprintf(product, sizeof(product), "VanGO Display %s", VANGO_PANEL_SIZE_LABEL);
    snprintf(line, sizeof(line), "%s  |  %dx%d",
             VANGO_PANEL_SIZE_LABEL, VANGO_LCD_H_RES, VANGO_LCD_V_RES);

    info_pair(card, 10, vango_tx("Product", "Ürün", "Produkt"), product, COL_TEXT);
    info_pair(card, 54, vango_tx("Software", "Yazılım", "Software"),
              HEXNET_DISP_RELEASE, COL_TEXT);
    info_pair(card, 98, vango_tx("Build", "Build", "Build"),
              hexnet_firmware_micro_string(), COL_TEXT);
    info_pair(card, 142, vango_tx("Hardware", "Donanım", "Hardware"), line, COL_TEXT);
    info_pair(card, 186, vango_tx("IO link", "IO bağlantısı", "IO-Verbindung"),
              up ? vango_tx("CAN, connected", "CAN, bağlı", "CAN, verbunden") : vango_tx("CAN, no link", "CAN, bağlantı yok", "CAN, keine Verbindung"),
              up ? COL_GLOW_G : COL_ORANGE);
    info_pair(card, 230, vango_tx("Support", "Destek", "Support"), "sales@hexnet.com.tr", COL_TEXT);
    info_pair(card, 274, vango_tx("Web", "Web", "Web"), "www.hexnet.com.tr", COL_TEXT);
}

/* ---------- Demo ---------- */
static void build_demo_section(void)
{
    const bool can_up = get_canbus_connection_status() != 0;
    bool demo_on = vango_dash_get_demo();

    if (can_up && demo_on) {
        vango_dash_set_demo(false);
        demo_on = false;
    }

    make_label(s_content, vango_tx("DEMO", "DEMO", "DEMO"), &vango_font_12, COL_ACCENT);
    lv_obj_set_pos(lv_obj_get_child(s_content, 0), 8, 4);

    lv_obj_t *card = lv_obj_create(s_content);
    lv_obj_set_size(card, CONT_W - 20, 180);
    lv_obj_set_pos(card, 8, 28);
    style_card(card);

    make_label(card, vango_tx("Demo mode", "Demo modu", "Demo-Modus"), &vango_font_16, COL_TEXT);
    lv_obj_set_pos(lv_obj_get_child(card, 0), 16, 24);

    lv_obj_t *demo_hint = make_label(card,
        can_up
            ? vango_tx("Off while CAN is linked", "CAN bağlıyken kapalı", "Aus bei CAN-Verbindung")
            : vango_tx("Relay, dimmer and water preview", "Röle, dimmer ve su önizleme", "Relais-, Dimmer- und Wasservorschau"),
        &vango_font_12, COL_MUTED);
    lv_obj_set_pos(demo_hint, 16, 54);

    lv_obj_t *demo_sw = lv_switch_create(card);
    lv_obj_set_size(demo_sw, 52, 28);
    lv_obj_align(demo_sw, LV_ALIGN_TOP_RIGHT, -16, 28);
    if (demo_on) {
        lv_obj_add_state(demo_sw, LV_STATE_CHECKED);
    }
    lv_obj_set_style_bg_color(demo_sw, lv_color_hex(COL_ACCENT), LV_PART_INDICATOR | LV_STATE_CHECKED);
    if (can_up) {
        lv_obj_add_state(demo_sw, LV_STATE_DISABLED);
    }
    lv_obj_add_event_cb(demo_sw, demo_sw_cb, LV_EVENT_VALUE_CHANGED, NULL);

    make_label(card,
        vango_tx("Use without IO module for UI preview.",
                 "IO modülü olmadan arayüz önizlemesi.",
                 "UI-Vorschau ohne IO-Modul."),
        &vango_font_12, COL_MUTED);
    lv_obj_set_pos(lv_obj_get_child(card, lv_obj_get_child_cnt(card) - 1), 16, 100);
}

static void show_section(set_sec_t sec)
{
    hexnet_wallpaper_note_activity();
    s_sec = sec;
    side_set_active(sec);
    clear_content();

    switch (sec) {
        case SET_SEC_PANEL:
            if (s_panel_detail < PANEL_CAT_COUNT) {
                build_panel_detail(s_panel_detail);
            } else {
                build_panel_overview();
            }
            break;
        case SET_SEC_SCREEN:
            build_screen_section();
            break;
        case SET_SEC_THEME:
            build_theme_section();
            break;
        case SET_SEC_LANG:
            build_lang_section();
            break;
        case SET_SEC_DEVICE:
            build_device_section();
            break;
        case SET_SEC_DEMO:
            build_demo_section();
            break;
        default:
            break;
    }
}

static void side_cb(lv_event_t *e)
{
    set_sec_t sec = (set_sec_t)(intptr_t)lv_event_get_user_data(e);
    s_panel_detail = PANEL_CAT_COUNT;
    show_section(sec);
}

static void add_menu_glyph(lv_obj_t *btn, int sec)
{
    if (sec == SET_SEC_PANEL) {
        lv_obj_t *img = lv_img_create(btn);
        lv_img_set_src(img, &vango_nav_settings);
        lv_img_set_zoom(img, 168);
        lv_obj_align(img, LV_ALIGN_LEFT_MID, 0, 0);
        lv_obj_set_style_img_recolor_opa(img, LV_OPA_COVER, 0);
        lv_obj_clear_flag(img, LV_OBJ_FLAG_CLICKABLE);
        s_side_glyphs[sec] = img;
        return;
    }

    lv_obj_t *box = lv_obj_create(btn);
    lv_obj_set_size(box, 24, 24);
    lv_obj_align(box, LV_ALIGN_LEFT_MID, 8, 0);
    lv_obj_set_style_bg_opa(box, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(box, 0, 0);
    lv_obj_set_style_pad_all(box, 0, 0);
    lv_obj_clear_flag(box, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);
    s_side_glyphs[sec] = box;

    if (sec == SET_SEC_SCREEN) {
        lv_obj_t *monitor = lv_obj_create(box);
        lv_obj_t *neck;
        lv_obj_t *base;
        lv_obj_set_size(monitor, 18, 12);
        lv_obj_align(monitor, LV_ALIGN_TOP_MID, 0, 1);
        lv_obj_set_style_bg_opa(monitor, LV_OPA_TRANSP, 0);
        lv_obj_set_style_border_width(monitor, 2, 0);
        lv_obj_set_style_radius(monitor, 2, 0);
        lv_obj_clear_flag(monitor, LV_OBJ_FLAG_CLICKABLE);
        neck = lv_obj_create(box);
        lv_obj_set_size(neck, 2, 3);
        lv_obj_align(neck, LV_ALIGN_BOTTOM_MID, 0, -5);
        lv_obj_set_style_border_width(neck, 0, 0);
        lv_obj_set_style_bg_opa(neck, LV_OPA_COVER, 0);
        lv_obj_set_style_radius(neck, 0, 0);
        lv_obj_clear_flag(neck, LV_OBJ_FLAG_CLICKABLE);
        base = lv_obj_create(box);
        lv_obj_set_size(base, 12, 2);
        lv_obj_align(base, LV_ALIGN_BOTTOM_MID, 0, -3);
        lv_obj_set_style_border_width(base, 0, 0);
        lv_obj_set_style_bg_opa(base, LV_OPA_COVER, 0);
        lv_obj_set_style_radius(base, 0, 0);
        lv_obj_clear_flag(base, LV_OBJ_FLAG_CLICKABLE);
    } else if (sec == SET_SEC_THEME) {
        static const int px[3] = {1, 9, 5};
        static const int py[3] = {11, 11, 3};
        for (int i = 0; i < 3; i++) {
            lv_obj_t *dot = lv_obj_create(box);
            lv_obj_set_size(dot, 8, 8);
            lv_obj_set_pos(dot, px[i], py[i]);
            lv_obj_set_style_radius(dot, LV_RADIUS_CIRCLE, 0);
            lv_obj_set_style_border_width(dot, 0, 0);
            lv_obj_set_style_bg_opa(dot, LV_OPA_COVER, 0);
            lv_obj_clear_flag(dot, LV_OBJ_FLAG_CLICKABLE);
        }
    } else if (sec == SET_SEC_LANG) {
        lv_obj_t *mark = make_label(box, "Aa", &vango_font_14, set_muted());
        lv_obj_center(mark);
    } else if (sec == SET_SEC_DEVICE) {
        lv_obj_t *ring = lv_obj_create(box);
        lv_obj_t *mark;
        lv_obj_set_size(ring, 16, 16);
        lv_obj_center(ring);
        lv_obj_set_style_radius(ring, LV_RADIUS_CIRCLE, 0);
        lv_obj_set_style_bg_opa(ring, LV_OPA_TRANSP, 0);
        lv_obj_set_style_border_width(ring, 2, 0);
        lv_obj_clear_flag(ring, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);
        mark = make_label(ring, "i", &vango_font_12, set_muted());
        lv_obj_center(mark);
    } else {
        /* Demo: play triangle */
        lv_obj_t *mark = make_label(box, ">", &vango_font_16, set_muted());
        lv_obj_center(mark);
    }
}

void vango_settings_build(lv_obj_t *parent)
{
    const char *items_tr[] = {
        "Panel Ayarları", "Ekran", "Tema", "Dil", "Cihaz Bilgileri", "Demo"
    };
    const char *items_en[] = {
        "Panel Settings", "Display", "Theme", "Language", "Device Info", "Demo"
    };
    const char *items_de[] = {
        "Paneleinstellungen", "Display", "Thema", "Sprache", "Geraeteinfo", "Demo"
    };
    const char **items = vango_is_tr() ? items_tr : (vango_is_de() ? items_de : items_en);
    const int item_h = 42;
    const int item_gap = 8;
    lv_obj_t *side;
    /* Prefs rebuild must not yank the user back to Panel. */
    set_sec_t keep_sec = s_sec;
    panel_cat_t keep_detail = s_panel_detail;

    if (keep_sec < SET_SEC_PANEL || keep_sec >= SET_SEC_COUNT) {
        keep_sec = SET_SEC_PANEL;
        keep_detail = PANEL_CAT_COUNT;
    }

    s_root = parent;
    lv_obj_set_style_bg_color(parent, lv_color_hex(set_bg()), 0);
    lv_obj_set_style_bg_opa(parent, LV_OPA_COVER, 0);

    side = lv_obj_create(parent);
    lv_obj_set_size(side, SIDE_W, PAGE_H);
    lv_obj_set_pos(side, 0, 0);
    lv_obj_set_style_bg_color(side, lv_color_hex(set_side_bg()), 0);
    lv_obj_set_style_bg_opa(side, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(side, 0, 0);
    lv_obj_set_style_border_width(side, 0, 0);
    lv_obj_set_style_pad_all(side, 0, 0);
    lv_obj_clear_flag(side, LV_OBJ_FLAG_SCROLLABLE);

    for (int i = 0; i < SET_SEC_COUNT; i++) {
        lv_obj_t *btn = lv_obj_create(side);
        lv_obj_set_size(btn, SIDE_W - 16, item_h);
        lv_obj_set_pos(btn, 8, 12 + i * (item_h + item_gap));
        lv_obj_set_style_radius(btn, 12, 0);
        lv_obj_set_style_bg_color(btn, lv_color_hex(set_sel_bg()), 0);
        lv_obj_set_style_bg_opa(btn, LV_OPA_TRANSP, 0);
        lv_obj_set_style_border_width(btn, 0, 0);
        lv_obj_set_style_border_color(btn, lv_color_hex(set_accent()), 0);
        lv_obj_set_style_pad_all(btn, 0, 0);
        lv_obj_clear_flag(btn, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_add_flag(btn, LV_OBJ_FLAG_CLICKABLE);
        lv_obj_add_event_cb(btn, side_cb, LV_EVENT_CLICKED, (void *)(intptr_t)i);
        s_side_btns[i] = btn;
        add_menu_glyph(btn, i);
        s_side_lbls[i] = make_label(btn, items[i], &vango_font_14, set_muted());
        lv_obj_align(s_side_lbls[i], LV_ALIGN_LEFT_MID, 40, 0);
    }

    s_content = lv_obj_create(parent);
    lv_obj_set_size(s_content, CONT_W, PAGE_H);
    lv_obj_set_pos(s_content, SIDE_W, 0);
    lv_obj_set_style_bg_color(s_content, lv_color_hex(set_bg()), 0);
    lv_obj_set_style_bg_opa(s_content, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(s_content, 0, 0);
    lv_obj_set_style_pad_all(s_content, 0, 0);
    lv_obj_set_style_radius(s_content, 0, 0);
    lv_obj_clear_flag(s_content, LV_OBJ_FLAG_SCROLLABLE);

    s_panel_detail = keep_detail;
    show_section(keep_sec);
    ESP_LOGI(TAG, "Settings UI built sec=%d", (int)keep_sec);
}

void vango_settings_set_relay_icon_key(int slot, const char *key)
{
    /* UART/E2E yolu: onay atlanir, dogrudan IO'ya yaz */
    if (s_icon_pending_slot >= 0) {
        icon_confirm_close(false);
    }
    relay_icon_apply_local(slot, key, true);
}

void vango_settings_show_section(const char *name)
{
    set_sec_t sec = SET_SEC_PANEL;
    panel_cat_t detail = PANEL_CAT_COUNT;
    if (!s_root || !name) {
        return;
    }
    if (strcmp(name, "screen") == 0) {
        sec = SET_SEC_SCREEN;
    } else if (strcmp(name, "theme") == 0) {
        sec = SET_SEC_THEME;
    } else if (strcmp(name, "lang") == 0) {
        sec = SET_SEC_LANG;
    } else if (strcmp(name, "about") == 0 || strcmp(name, "device") == 0 || strcmp(name, "conn") == 0) {
        sec = SET_SEC_DEVICE;
    } else if (strcmp(name, "demo") == 0) {
        sec = SET_SEC_DEMO;
    } else if (strcmp(name, "buttons") == 0 || strcmp(name, "buton") == 0) {
        sec = SET_SEC_PANEL;
        detail = PANEL_CAT_BUTTON;
    } else if (strcmp(name, "lights") == 0 || strcmp(name, "light") == 0 ||
               strcmp(name, "aydinlatma") == 0) {
        sec = SET_SEC_PANEL;
        detail = PANEL_CAT_LIGHT;
    } else if (strcmp(name, "panel_motors") == 0 || strcmp(name, "panel_motor") == 0) {
        sec = SET_SEC_PANEL;
        detail = PANEL_CAT_MOTOR;
    } else if (strcmp(name, "panel_sensors") == 0 || strcmp(name, "panel_sensor") == 0) {
        sec = SET_SEC_PANEL;
        detail = PANEL_CAT_SENSOR;
    } else if (strcmp(name, "panel_water") == 0 || strcmp(name, "panel_tanks") == 0) {
        sec = SET_SEC_PANEL;
        detail = PANEL_CAT_WATER;
    } else if (strcmp(name, "relaymenu") == 0) {
        sec = SET_SEC_PANEL;
        detail = PANEL_CAT_BUTTON;
    }
    s_panel_detail = detail;
    show_section(sec);
    if (strcmp(name, "relaymenu") == 0) {
        open_relay_type_menu(0);
    }
}

void vango_settings_on_prefs_changed(void)
{
    if (!s_root) {
        return;
    }
    /* s_sec / s_panel_detail kept across rebuild by vango_settings_build. */
    lv_obj_clean(s_root);
    memset(s_side_btns, 0, sizeof(s_side_btns));
    memset(s_side_lbls, 0, sizeof(s_side_lbls));
    memset(s_side_glyphs, 0, sizeof(s_side_glyphs));
    s_content = NULL;
    vango_settings_build(s_root);
}
