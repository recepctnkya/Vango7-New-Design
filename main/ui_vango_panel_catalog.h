#pragma once

#include <stddef.h>

/** Tablet PanelIconCatalog ile hizali — Display panel ikon anahtarlari. */
typedef struct {
    const char *key;
    const char *en;
    const char *tr;
} vango_panel_catalog_item_t;

const vango_panel_catalog_item_t *vango_panel_catalog(void);
size_t vango_panel_catalog_count(void);
