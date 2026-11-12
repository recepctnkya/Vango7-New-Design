#pragma once

/**
 * VanGO Display UI strings — follow panelLanguageType:
 *   0 = manual English (locked; ignore IO lang)
 *   1 = manual Turkish (locked; ignore IO lang)
 *   2 = system         (follow IO SoT via io_sot_language — 0/1 only)
 *   3 = manual German  (locked local Display; not pushed to IO SoT)
 */

#ifdef __cplusplus
extern "C" {
#endif

extern int panelLanguageType;
/** Last effective language from IO (0=EN, 1=TR). Used when panelLanguageType==2. */
extern int io_sot_language;

/** Effective UI language: 0=EN, 1=TR, 3=DE. */
static inline int vango_lang(void)
{
    if (panelLanguageType == 2) {
        return (io_sot_language == 1) ? 1 : 0;
    }
    if (panelLanguageType == 1 || panelLanguageType == 3) {
        return panelLanguageType;
    }
    return 0;
}

static inline int vango_is_tr(void)
{
    return vango_lang() == 1;
}

static inline int vango_is_de(void)
{
    return vango_lang() == 3;
}

/** Pick EN / TR / DE string for current language. */
static inline const char *vango_tx(const char *en, const char *tr, const char *de)
{
    const int l = vango_lang();
    if (l == 1) {
        return tr;
    }
    if (l == 3) {
        return de;
    }
    return en;
}

static inline const char *vango_tx_on(void)
{
    return vango_tx("On", "Açık", "An");
}

static inline const char *vango_tx_off(void)
{
    return vango_tx("Off", "Kapalı", "Aus");
}

#ifdef __cplusplus
}
#endif
