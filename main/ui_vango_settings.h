#pragma once

#include "lvgl.h"

#ifdef __cplusplus
extern "C" {
#endif

/** Build settings UI into an empty tile parent (lazy, once). */
void vango_settings_build(lv_obj_t *parent);

/** Refresh labels/selection after theme or language change. */
void vango_settings_on_prefs_changed(void);

/** Open a settings section: panel, screen, theme, lang, device, demo. */
void vango_settings_show_section(const char *name);

/**
 * UART/E2E: buton ikonunu catalog key ile degistir + CAN 0x762/763 (IO→tablet).
 * UI menu acmadan ayni SoT yolunu kullanir.
 */
void vango_settings_set_relay_icon_key(int slot, const char *key);

#ifdef __cplusplus
}
#endif
