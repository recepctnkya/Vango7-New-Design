#include "vango_shot.h"
#include "lvgl_port.h"
#include "ui_vango_dash.h"
#include "ui_vango_settings.h"
#include "display_manager.h"

#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "esp_clk_tree.h"
#include "esp_heap_caps.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/idf_additions.h"
#include "hal/uart_ll.h"
#include "sdkconfig.h"
#include "soc/uart_struct.h"

#define SHOT_W 800
#define SHOT_H 480
#define SHOT_BYTES (SHOT_W * SHOT_H * 2)
#define SHOT_BAUD_FAST 2000000
#define SHOT_BAUD_NORMAL 115200

static const char *TAG = "shot";
static const uint16_t *s_fb;
static volatile int s_shot_req;
static volatile int s_shot_ready;
static volatile int s_dump_busy;

void vango_shot_note_fb(const void *fb)
{
    if (fb) {
        s_fb = fb;
    }
}

static uint32_t shot_uart_sclk_hz(void)
{
    soc_module_clk_t src = SOC_MOD_CLK_INVALID;
    uint32_t hz = 0;
    uart_ll_get_sclk(&UART0, &src);
    if (esp_clk_tree_src_get_freq_hz(src, ESP_CLK_TREE_SRC_FREQ_PRECISION_CACHED, &hz) != ESP_OK || hz == 0) {
        hz = 80000000;
    }
    return hz;
}

static void shot_uart_set_baud(uint32_t baud)
{
    (void)uart_ll_set_baudrate(&UART0, baud, shot_uart_sclk_hz());
}

/* Binary TX: uart_ll (CRLF yok). ~her 200ms yield — WDT; 2M'de chunk delay yok. */
static void shot_ll_write(const void *data, size_t len)
{
    const uint8_t *p = data;
    int64_t last_yield = esp_timer_get_time();
    while (len) {
        uint32_t space = uart_ll_get_txfifo_len(&UART0);
        if (space == 0) {
            if ((esp_timer_get_time() - last_yield) > 200000) {
                last_yield = esp_timer_get_time();
                vTaskDelay(1);
            }
            continue;
        }
        uint32_t n = (uint32_t)(len > space ? space : len);
        if (n > 128) {
            n = 128;
        }
        uart_ll_write_txfifo(&UART0, p, n);
        p += n;
        len -= n;
        if ((esp_timer_get_time() - last_yield) > 200000) {
            last_yield = esp_timer_get_time();
            vTaskDelay(1);
        }
    }
}

static void shot_ll_write_str(const char *s)
{
    shot_ll_write(s, strlen(s));
}

static void shot_wait_tx_idle(void)
{
    int i;
    for (i = 0; i < 2000 && !uart_ll_is_tx_idle(&UART0); i++) {
        vTaskDelay(1);
    }
}

static void send_shot(void)
{
    uint16_t *copy;
    char hdr[48];
    bool locked;
    int64_t t0;
    uint32_t sclk;

    if (s_dump_busy) {
        ESP_LOGE(TAG, "send_shot busy skip");
        s_shot_req = 0;
        return;
    }
    s_dump_busy = 1;
    ESP_LOGE(TAG, "send_shot begin fb=%p", (void *)s_fb);

    copy = heap_caps_malloc(SHOT_BYTES, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    if (!copy) {
        ESP_LOGE(TAG, "SHOT_ERR nomem");
        s_dump_busy = 0;
        s_shot_req = 0;
        return;
    }
    locked = lvgl_port_lock(2000);
    if (!locked || !s_fb) {
        if (locked) {
            lvgl_port_unlock();
        }
        ESP_LOGE(TAG, "SHOT_ERR %s", s_fb ? "lock" : "nofb");
        heap_caps_free(copy);
        s_dump_busy = 0;
        s_shot_req = 0;
        return;
    }
    memcpy(copy, s_fb, SHOT_BYTES);
    lvgl_port_unlock();

    /* Host 115200'de duz satir bekler (ESP_LOG ANSI degil) — tools/vango_shot.py */
    esp_log_level_set("*", ESP_LOG_NONE);
    fflush(stdout);
    shot_wait_tx_idle();
    snprintf(hdr, sizeof(hdr), "SHOT_FAST %d\n", SHOT_BAUD_FAST);
    shot_ll_write_str(hdr);
    shot_wait_tx_idle();
    vTaskDelay(pdMS_TO_TICKS(30));

    sclk = shot_uart_sclk_hz();
    shot_uart_set_baud(SHOT_BAUD_FAST);
    vTaskDelay(pdMS_TO_TICKS(80));

    t0 = esp_timer_get_time();
    snprintf(hdr, sizeof(hdr), "SHOT %d %d %d\n", SHOT_W, SHOT_H, SHOT_BYTES);
    shot_ll_write_str(hdr);
    shot_ll_write(copy, SHOT_BYTES);
    shot_ll_write_str("\nSHOT_END\n");
    shot_wait_tx_idle();
    heap_caps_free(copy);

    shot_uart_set_baud(SHOT_BAUD_NORMAL);
    vTaskDelay(pdMS_TO_TICKS(40));
    esp_log_level_set("*", ESP_LOG_INFO);
    ESP_LOGE(TAG, "frame sent bytes=%d ms=%d baud=%d sclk=%u", SHOT_BYTES,
             (int)((esp_timer_get_time() - t0) / 1000), SHOT_BAUD_FAST, (unsigned)sclk);
    s_dump_busy = 0;
    s_shot_req = 0;
}

static void send_shot_task(void *arg)
{
    (void)arg;
    send_shot();
    vTaskDeleteWithCaps(NULL);
}

static void trigger_dump(void)
{
    if (!s_shot_ready || !s_fb) {
        ESP_LOGE(TAG, "shot ignored (ready=%d fb=%p)", s_shot_ready, (void *)s_fb);
        return;
    }
    if (s_shot_req || s_dump_busy) {
        return;
    }
    s_shot_req = 1;
    /* Stack SPIRAM — internal DRAM BLE icin kalsin */
    if (xTaskCreateWithCaps(send_shot_task, "rgb565", 4096, NULL, 4, NULL,
                            MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT) != pdPASS) {
        s_shot_req = 0;
        ESP_LOGE(TAG, "dump task FAIL spiram/internal %u/%u",
                 (unsigned)heap_caps_get_free_size(MALLOC_CAP_SPIRAM),
                 (unsigned)heap_caps_get_free_size(MALLOC_CAP_INTERNAL));
    }
}

static int page_from_name(const char *name)
{
    if (strcmp(name, "home") == 0) {
        return VANGO_PAGE_HOME;
    }
    if (strcmp(name, "dimmer") == 0) {
        return VANGO_PAGE_DIMMER;
    }
    if (strcmp(name, "rgb") == 0) {
        return VANGO_PAGE_RGB;
    }
    if (strcmp(name, "water") == 0) {
        return VANGO_PAGE_WATER;
    }
    if (strcmp(name, "sensors") == 0) {
        return VANGO_PAGE_SENSORS;
    }
    if (strcmp(name, "motors") == 0) {
        return VANGO_PAGE_MOTORS;
    }
    if (strcmp(name, "settings") == 0) {
        return VANGO_PAGE_SETTINGS;
    }
    return -1;
}

/* stdin VFS — uart_ll RX calma yok (driver race / reset). */
static void shot_cmd_task(void *arg)
{
    char line[64];
    int n = 0;
    (void)arg;
    ESP_LOGI(TAG, "shot cmd: send 'shot' / 'page <name>' / 'icon <slot> <key>'");
    while (1) {
        unsigned char ch;
        int r = read(STDIN_FILENO, &ch, 1);
        if (r != 1) {
            vTaskDelay(pdMS_TO_TICKS(20));
            continue;
        }
        if (ch == '\r' || ch == '\n') {
            if (n == 0) {
                continue;
            }
            line[n] = 0;
            n = 0;
            if (strcmp(line, "shot") == 0) {
                trigger_dump();
            } else if (strncmp(line, "page ", 5) == 0) {
                const char *pname = line + 5;
                if (strcmp(pname, "wallpaper") == 0 || strcmp(pname, "saver") == 0 ||
                    strcmp(pname, "screensaver") == 0) {
                    if (lvgl_port_lock(2000)) {
                        hexnet_wallpaper_show();
                        lvgl_port_unlock();
                    }
                /* settings alt sayfalari: page buttons / page lights / ... */
                } else if (strcmp(pname, "buttons") == 0 || strcmp(pname, "buton") == 0 ||
                    strcmp(pname, "relaymenu") == 0 ||
                    strcmp(pname, "lights") == 0 || strcmp(pname, "light") == 0 ||
                    strcmp(pname, "aydinlatma") == 0 ||
                    strcmp(pname, "panel_motors") == 0 || strcmp(pname, "panel_motor") == 0 ||
                    strcmp(pname, "panel_sensors") == 0 || strcmp(pname, "panel_sensor") == 0 ||
                    strcmp(pname, "panel_water") == 0 || strcmp(pname, "panel_tanks") == 0 ||
                    strcmp(pname, "screen") == 0 || strcmp(pname, "theme") == 0 ||
                    strcmp(pname, "lang") == 0 || strcmp(pname, "device") == 0 ||
                    strcmp(pname, "demo") == 0) {
                    if (lvgl_port_lock(2000)) {
                        vango_dash_show(VANGO_PAGE_SETTINGS);
                        vango_settings_show_section(pname);
                        lvgl_port_unlock();
                    }
                } else {
                    int page = page_from_name(pname);
                    if (page >= 0 && lvgl_port_lock(2000)) {
                        vango_dash_show((vango_page_t)page);
                        lvgl_port_unlock();
                    }
                }
            } else if (strncmp(line, "relay ", 6) == 0) {
                int slot = -1;
                int on = 0;
                if (sscanf(line + 6, "%d %d", &slot, &on) == 2 && lvgl_port_lock(2000)) {
                    vango_dash_test_relay(slot, on != 0);
                    lvgl_port_unlock();
                    ESP_LOGE(TAG, "test relay %d -> %d", slot, on != 0);
                }
            } else if (strncmp(line, "dim ", 4) == 0) {
                int slot = -1;
                int pct = 0;
                if (sscanf(line + 4, "%d %d", &slot, &pct) == 2 && lvgl_port_lock(2000)) {
                    vango_dash_test_dim(slot, pct);
                    lvgl_port_unlock();
                    ESP_LOGE(TAG, "test dim %d -> %d", slot, pct);
                }
            } else if (strncmp(line, "icon ", 5) == 0) {
                int slot = -1;
                char key[40] = {0};
                if (sscanf(line + 5, "%d %39s", &slot, key) == 2 && lvgl_port_lock(2000)) {
                    vango_settings_set_relay_icon_key(slot, key);
                    lvgl_port_unlock();
                    ESP_LOGE(TAG, "test icon %d -> %s", slot, key);
                }
            }
            continue;
        }
        if (n < (int)sizeof(line) - 1) {
            line[n++] = (char)ch;
        }
    }
}

void vango_shot_start(void)
{
    static bool cmd_started;
    /* Dash/BLE DRAM yemeden once cmd task — sonra internal ~50B kaliyor. */
    if (!cmd_started) {
        cmd_started = true;
        BaseType_t ok = xTaskCreateWithCaps(shot_cmd_task, "shotcmd", 3072, NULL, 1, NULL,
                                            MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
        ESP_LOGE(TAG, "shotcmd %s internal %u", ok == pdPASS ? "ok" : "FAIL",
                 (unsigned)heap_caps_get_free_size(MALLOC_CAP_INTERNAL));
    }
}

void vango_shot_set_ready(void)
{
    s_shot_ready = 1;
    ESP_LOGE(TAG, "shot ready internal %u",
             (unsigned)heap_caps_get_free_size(MALLOC_CAP_INTERNAL));
}

void vango_shot_request(void)
{
    trigger_dump();
}
