#pragma once
/* Force-included into LVGL TUs: UI objects go to SPIRAM so BLE keeps internal DRAM. */
#include "esp_heap_caps.h"
#include <stddef.h>

static inline void *lv_psram_malloc(size_t size)
{
    void *p = heap_caps_malloc(size, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    if (!p) {
        p = heap_caps_malloc(size, MALLOC_CAP_8BIT);
    }
    return p;
}

static inline void lv_psram_free(void *p)
{
    heap_caps_free(p);
}

static inline void *lv_psram_realloc(void *ptr, size_t size)
{
    void *p = heap_caps_realloc(ptr, size, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    if (!p && size) {
        p = heap_caps_realloc(ptr, size, MALLOC_CAP_8BIT);
    }
    return p;
}

#define LV_MEM_CUSTOM_ALLOC   lv_psram_malloc
#define LV_MEM_CUSTOM_FREE    lv_psram_free
#define LV_MEM_CUSTOM_REALLOC lv_psram_realloc
