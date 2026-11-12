#pragma once

#include "lvgl.h"

#ifdef __cplusplus
extern "C" {
#endif

/** RGB Aydınlatma sayfası — dash tile parent. */
void vango_rgb_build(lv_obj_t *parent);

/** Prefs/theme rebuild veya sayfa tekrar gösteriminde UI senkron. */
void vango_rgb_refresh(void);

#ifdef __cplusplus
}
#endif
