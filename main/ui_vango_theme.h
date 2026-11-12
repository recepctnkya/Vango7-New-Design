#pragma once

/**
 * VanGO dash / settings palette — follows panelThemeType:
 *   0 = manual light (locked; ignore IO theme)
 *   1 = manual dark  (locked; ignore IO theme)
 *   2 = system       (follow IO SoT effective 0/1 via io_sot_theme)
 *
 * Display has no OS theme — "system" means follow IO Module SoT.
 */

#ifdef __cplusplus
extern "C" {
#endif

extern int panelThemeType;
/** Last effective appearance from IO (0=light, 1=dark). Used when panelThemeType==2. */
extern int io_sot_theme;
/** Accent swatch index — same set as tablet ThemeRedesignPage (local Display, not IO). */
extern int panelAccentIdx;

#define VANGO_ACCENT_N 8

/** Tablet accents: vango, blue, purple, pink, orange, yellow, lime, green */
static inline uint32_t vango_accent_at(int idx)
{
    static const uint32_t k[VANGO_ACCENT_N] = {
        0x2EC5FFu, 0x3B82F6u, 0x7C3AEDu, 0xEC4899u,
        0xF97316u, 0xEAB308u, 0x84CC16u, 0x10B981u
    };
    if (idx < 0 || idx >= VANGO_ACCENT_N) {
        idx = 0;
    }
    return k[idx];
}

/** Effective appearance (resolves system → IO SoT). */
static inline int vango_theme_dark(void)
{
    if (panelThemeType == 2) {
        return io_sot_theme != 0;
    }
    return panelThemeType != 0;
}

static inline uint32_t vango_col_bg(void)
{
    return vango_theme_dark() ? 0x0B1220u : 0xF1F5F9u;
}

static inline uint32_t vango_col_card(void)
{
    return vango_theme_dark() ? 0x151D2Eu : 0xFFFFFFFFu;
}

static inline uint32_t vango_col_card_border(void)
{
    return vango_theme_dark() ? 0x243044u : 0xE2E8F0u;
}

static inline uint32_t vango_col_nav(void)
{
    return vango_theme_dark() ? 0x0D1729u : 0xFFFFFFFFu;
}

static inline uint32_t vango_col_nav_border(void)
{
    return vango_theme_dark() ? 0x24445Bu : 0xE2E8F0u;
}

static inline uint32_t vango_col_side(void)
{
    return vango_theme_dark() ? 0x101828u : 0xE8EEF5u;
}

static inline uint32_t vango_col_text(void)
{
    return vango_theme_dark() ? 0xFFFFFFFFu : 0x0F172Au;
}

static inline uint32_t vango_col_muted(void)
{
    return vango_theme_dark() ? 0x8B95A8u : 0x64748Bu;
}

static inline uint32_t vango_col_accent(void)
{
    return vango_accent_at(panelAccentIdx);
}

static inline uint32_t vango_col_on_accent(void)
{
    const uint32_t a = vango_col_accent();
    /* Light accents need dark label text. */
    if (a == 0xEAB308u || a == 0x84CC16u) {
        return 0x0F172Au;
    }
    return 0xFFFFFFFFu;
}

/** Soft selected fill tinted from accent (nav / side). */
static inline uint32_t vango_col_sel_tint(void)
{
    const uint32_t a = vango_col_accent();
    const unsigned ar = (a >> 16) & 0xFFu;
    const unsigned ag = (a >> 8) & 0xFFu;
    const unsigned ab = a & 0xFFu;
    if (vango_theme_dark()) {
        unsigned r = 0x12u + (ar * 40u) / 255u;
        unsigned g = 0x18u + (ag * 48u) / 255u;
        unsigned b = 0x28u + (ab * 70u) / 255u;
        if (r > 255u) {
            r = 255u;
        }
        if (g > 255u) {
            g = 255u;
        }
        if (b > 255u) {
            b = 255u;
        }
        return (r << 16) | (g << 8) | b;
    }
    unsigned r = 0xE8u + (ar * 20u) / 255u;
    unsigned g = 0xECu + (ag * 18u) / 255u;
    unsigned b = 0xF4u + (ab * 10u) / 255u;
    if (r > 255u) {
        r = 255u;
    }
    if (g > 255u) {
        g = 255u;
    }
    if (b > 255u) {
        b = 255u;
    }
    return (r << 16) | (g << 8) | b;
}

static inline uint32_t vango_col_nav_sel(void)
{
    return vango_col_sel_tint();
}

static inline uint32_t vango_col_side_sel(void)
{
    return vango_col_sel_tint();
}

/** Accent mixed into dark/light for borders, rules, card edges. */
static inline uint32_t vango_col_accent_dim(void)
{
    const uint32_t a = vango_col_accent();
    const unsigned ar = (a >> 16) & 0xFFu;
    const unsigned ag = (a >> 8) & 0xFFu;
    const unsigned ab = a & 0xFFu;
    if (vango_theme_dark()) {
        unsigned r = 0x14u + (ar * 70u) / 255u;
        unsigned g = 0x1Cu + (ag * 80u) / 255u;
        unsigned b = 0x28u + (ab * 100u) / 255u;
        if (r > 255u) {
            r = 255u;
        }
        if (g > 255u) {
            g = 255u;
        }
        if (b > 255u) {
            b = 255u;
        }
        return (r << 16) | (g << 8) | b;
    }
    unsigned r = 0x90u + (ar * 50u) / 255u;
    unsigned g = 0x98u + (ag * 48u) / 255u;
    unsigned b = 0xA8u + (ab * 40u) / 255u;
    if (r > 255u) {
        r = 255u;
    }
    if (g > 255u) {
        g = 255u;
    }
    if (b > 255u) {
        b = 255u;
    }
    return (r << 16) | (g << 8) | b;
}

/** Deep card face tinted by accent (dark pages). */
static inline uint32_t vango_col_accent_card(void)
{
    const uint32_t a = vango_col_accent();
    const unsigned ar = (a >> 16) & 0xFFu;
    const unsigned ag = (a >> 8) & 0xFFu;
    const unsigned ab = a & 0xFFu;
    if (vango_theme_dark()) {
        unsigned r = 0x0Au + (ar * 28u) / 255u;
        unsigned g = 0x12u + (ag * 32u) / 255u;
        unsigned b = 0x1Au + (ab * 40u) / 255u;
        return (r << 16) | (g << 8) | b;
    }
    return vango_col_card();
}

static inline uint32_t vango_col_dot_off(void)
{
    return vango_theme_dark() ? 0x2A3448u : 0xCBD5E1u;
}

static inline uint32_t vango_col_btn_idle(void)
{
    return vango_theme_dark() ? 0x1A2438u : 0xE2E8F0u;
}

static inline uint32_t vango_col_light_on(void)
{
    return vango_theme_dark() ? 0x1A2438u : 0xFFFBEBu;
}

static inline uint32_t vango_col_tank_track(void)
{
    return vango_theme_dark() ? 0x0A1020u : 0xE2E8F0u;
}

#define COL_BG            (vango_col_bg())
#define COL_CARD          (vango_col_card())
#define COL_CARD_BORDER   (vango_col_card_border())
#define COL_NAV           (vango_col_nav())
#define COL_NAV_SEL       (vango_col_nav_sel())
#define COL_NAV_BORDER    (vango_col_nav_border())
#define COL_SIDE          (vango_col_side())
#define COL_SIDE_SEL      (vango_col_side_sel())
#define COL_ACCENT        (vango_col_accent())
#define COL_ACCENT_DIM    (vango_col_accent_dim())
#define COL_ACCENT_CARD   (vango_col_accent_card())
#define COL_ON_ACCENT     (vango_col_on_accent())
#define COL_TEXT          (vango_col_text())
#define COL_MUTED         (vango_col_muted())
#define COL_DOT_OFF       (vango_col_dot_off())
#define COL_BTN_IDLE      (vango_col_btn_idle())
#define COL_LIGHT_ON      (vango_col_light_on())
#define COL_TANK_TRACK    (vango_col_tank_track())

#define COL_GREEN         0x22C55Eu
#define COL_WATER_C       0x3B82F6u
#define COL_WATER_W       0xC4A06Au
#define COL_WATER_G       0xC4C4C4u
#define COL_GLOW_Y        0xFBBF24u
#define COL_GLOW_G        0x22C55Eu
#define COL_GLOW_B        (vango_col_accent())
#define COL_GLOW_P        0xA78BFAu
#define COL_ORANGE        0xF97316u
#define COL_RED           0xEF4444u
/* Ust menu IO mikroçip durum renkleri (referans) */
#define COL_IO_UP         0x22C55Eu /* Bagli */
#define COL_IO_WAIT       0xF59E0Bu /* Baglaniyor */
#define COL_IO_DOWN       0xEF4444u /* Baglanti yok */
#define COL_IO_OFF        0x6B7280u /* Pasif / kapali */

#ifdef __cplusplus
}
#endif
