#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/** Tablet PanelIconCatalog key → firmware preset 1..18 (Display asset). */
uint8_t hexnet_icon_key_to_preset(const char *key);

#ifdef __cplusplus
}
#endif
