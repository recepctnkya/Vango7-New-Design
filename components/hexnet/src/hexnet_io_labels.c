#include "hexnet_io_labels.h"

#include <string.h>

#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"
#include "hexnet_io_profile.h"
#include "nvs.h"

static const char *TAG = "IO_LABELS";

#define NVS_NS   "hexnet"
#define NVS_KEY  "io_labels"
#define MAGIC    0x4C4F4948u /* HIOL */

typedef struct __attribute__((packed)) {
    uint32_t magic;
    uint8_t version;
    uint8_t reserved[3];
    hexnet_io_labels_t labels;
} hexnet_io_labels_blob_t;

static hexnet_io_labels_t s_labels;
static bool s_loaded;
static bool s_ui_dirty;
static bool s_nvs_dirty;
static QueueHandle_t s_save_q;
static TaskHandle_t s_save_task;
/* Assemble name/icon across CAN parts (slot-local). */
static char s_can_asm[2][HEXNET_LABEL_RELAYS][HEXNET_LABEL_NAME_MAX + 1];
static uint8_t s_can_parts[2][HEXNET_LABEL_RELAYS]; /* bit0..bit3 = part alindi */

static esp_err_t labels_flush_nvs(void)
{
    hexnet_io_labels_blob_t blob;
    memset(&blob, 0, sizeof(blob));
    blob.magic = MAGIC;
    blob.version = 1;
    blob.labels = s_labels;

    nvs_handle_t h;
    esp_err_t err = nvs_open(NVS_NS, NVS_READWRITE, &h);
    if (err != ESP_OK) {
        return err;
    }
    err = nvs_set_blob(h, NVS_KEY, &blob, sizeof(blob));
    if (err == ESP_OK) {
        err = nvs_commit(h);
    }
    nvs_close(h);
    if (err == ESP_OK) {
        ESP_LOGI(TAG, "labels flash'a yazildi (%u B)", (unsigned)sizeof(blob));
    }
    return err;
}

static void labels_save_task(void *arg)
{
    (void)arg;
    uint32_t token;
    for (;;) {
        if (xQueueReceive(s_save_q, &token, portMAX_DELAY) != pdTRUE) {
            continue;
        }
        /* LVGL task stack PSRAM — flash cache disable assert olmasin */
        vTaskDelay(pdMS_TO_TICKS(80));
        while (xQueueReceive(s_save_q, &token, 0) == pdTRUE) {
            /* coalesce */
        }
        if (!s_nvs_dirty) {
            continue;
        }
        s_nvs_dirty = false;
        (void)labels_flush_nvs();
    }
}

static void labels_schedule_nvs_save(void)
{
    s_nvs_dirty = true;
    if (!s_save_q) {
        s_save_q = xQueueCreate(1, sizeof(uint32_t));
    }
    if (!s_save_task && s_save_q) {
        BaseType_t ok = xTaskCreate(labels_save_task, "io_lbl_nvs", 4096, NULL, 3, &s_save_task);
        if (ok != pdPASS) {
            s_save_task = NULL;
            ESP_LOGE(TAG, "NVS save task create failed");
            return;
        }
    }
    if (s_save_q) {
        uint32_t token = 1;
        (void)xQueueOverwrite(s_save_q, &token);
    }
}

static void slot_clear(hexnet_label_slot_t *s)
{
    if (!s) {
        return;
    }
    s->name[0] = '\0';
    s->icon[0] = '\0';
}

static void labels_clear(hexnet_io_labels_t *L)
{
    for (int i = 0; i < HEXNET_LABEL_RELAYS; ++i) {
        slot_clear(&L->relays[i]);
    }
    for (int i = 0; i < HEXNET_LABEL_DIMS; ++i) {
        slot_clear(&L->dimmers[i]);
    }
    for (int i = 0; i < HEXNET_LABEL_SENSORS; ++i) {
        slot_clear(&L->sensors[i]);
    }
    for (int i = 0; i < HEXNET_LABEL_WATER; ++i) {
        slot_clear(&L->water[i]);
    }
}

static void copy_cstr(char *dst, size_t dst_sz, const char *src)
{
    if (!dst || dst_sz == 0) {
        return;
    }
    if (!src) {
        dst[0] = '\0';
        return;
    }
    strncpy(dst, src, dst_sz - 1);
    dst[dst_sz - 1] = '\0';
}

static void apply_slot_obj(hexnet_label_slot_t *slot, const cJSON *item)
{
    if (!slot || !cJSON_IsObject(item)) {
        return;
    }
    const cJSON *name = cJSON_GetObjectItem(item, "name");
    const cJSON *icon = cJSON_GetObjectItem(item, "icon");
    if (cJSON_IsString(name) && name->valuestring) {
        copy_cstr(slot->name, sizeof(slot->name), name->valuestring);
    }
    if (cJSON_IsString(icon) && icon->valuestring) {
        copy_cstr(slot->icon, sizeof(slot->icon), icon->valuestring);
    }
}

static void apply_channel_array(hexnet_label_slot_t *slots, int max_slots, const cJSON *arr)
{
    if (!cJSON_IsArray(arr) || !slots) {
        return;
    }
    const int n = cJSON_GetArraySize(arr);
    for (int i = 0; i < n; ++i) {
        const cJSON *item = cJSON_GetArrayItem(arr, i);
        if (!cJSON_IsObject(item)) {
            continue;
        }
        const cJSON *id_item = cJSON_GetObjectItem(item, "id");
        int id = cJSON_IsNumber(id_item) ? id_item->valueint : i;
        if (id < 0 || id >= max_slots) {
            continue;
        }
        apply_slot_obj(&slots[id], item);
    }
}

static cJSON *slot_to_json(int id, const hexnet_label_slot_t *slot, int enabled)
{
    cJSON *o = cJSON_CreateObject();
    if (!o) {
        return NULL;
    }
    cJSON_AddNumberToObject(o, "id", id);
    cJSON_AddBoolToObject(o, "enabled", enabled != 0);
    cJSON_AddStringToObject(o, "name", slot && slot->name[0] ? slot->name : "");
    cJSON_AddStringToObject(o, "icon", slot && slot->icon[0] ? slot->icon : "");
    return o;
}

void hexnet_io_labels_init(void)
{
    labels_clear(&s_labels);
    nvs_handle_t h;
    if (nvs_open(NVS_NS, NVS_READONLY, &h) != ESP_OK) {
        s_loaded = true;
        ESP_LOGW(TAG, "labels NVS yok — bos");
        return;
    }
    hexnet_io_labels_blob_t blob;
    size_t len = sizeof(blob);
    esp_err_t err = nvs_get_blob(h, NVS_KEY, &blob, &len);
    nvs_close(h);
    if (err == ESP_OK && len >= sizeof(blob) && blob.magic == MAGIC && blob.version == 1) {
        s_labels = blob.labels;
        ESP_LOGI(TAG, "labels NVS yuklendi");
    } else {
        ESP_LOGW(TAG, "labels blob yok/bozuk");
    }
    s_loaded = true;
}

const hexnet_io_labels_t *hexnet_io_labels_get(void)
{
    return &s_labels;
}

esp_err_t hexnet_io_labels_set_relay(int slot, const char *name, const char *icon)
{
    hexnet_label_slot_t *slotp;
    if (slot < 0 || slot >= HEXNET_LABEL_RELAYS) {
        return ESP_ERR_INVALID_ARG;
    }
    slotp = &s_labels.relays[slot];
    if (name) {
        copy_cstr(slotp->name, sizeof(slotp->name), name);
    }
    if (icon) {
        copy_cstr(slotp->icon, sizeof(slotp->icon), icon);
    }
    s_ui_dirty = true;
    labels_schedule_nvs_save();
    return ESP_OK;
}

esp_err_t hexnet_io_labels_save(void)
{
    /* LVGL/CAN callback guvenli degil — ayri task'ta yaz */
    labels_schedule_nvs_save();
    return ESP_OK;
}

bool hexnet_io_labels_apply_json(const cJSON *json)
{
    if (!json || !s_loaded) {
        return false;
    }
    const cJSON *channels = cJSON_GetObjectItem(json, "channels");
    if (!cJSON_IsObject(channels)) {
        channels = cJSON_GetObjectItem(json, "panel_channels");
    }
    if (!cJSON_IsObject(channels)) {
        return false;
    }

    bool any = false;
    const cJSON *relays = cJSON_GetObjectItem(channels, "relays");
    if (cJSON_IsArray(relays)) {
        apply_channel_array(s_labels.relays, HEXNET_LABEL_RELAYS, relays);
        any = true;
    }
    const cJSON *dims = cJSON_GetObjectItem(channels, "dimmers");
    if (cJSON_IsArray(dims)) {
        apply_channel_array(s_labels.dimmers, HEXNET_LABEL_DIMS, dims);
        any = true;
    }
    const cJSON *sensors = cJSON_GetObjectItem(channels, "sensors");
    if (cJSON_IsArray(sensors)) {
        apply_channel_array(s_labels.sensors, HEXNET_LABEL_SENSORS, sensors);
        any = true;
    }
    const cJSON *water = cJSON_GetObjectItem(channels, "water");
    if (cJSON_IsArray(water)) {
        apply_channel_array(s_labels.water, HEXNET_LABEL_WATER, water);
        any = true;
    }

    if (any) {
        s_ui_dirty = true;
        labels_schedule_nvs_save();
    }
    return any;
}

void hexnet_io_labels_can_chunk(uint8_t kind, uint8_t slot, uint8_t part, const uint8_t six[6])
{
    if (!s_loaded || !six || slot >= HEXNET_LABEL_RELAYS || kind > 1 || part > 3) {
        return;
    }
    char *asm_buf = s_can_asm[kind][slot];
    uint8_t *parts = &s_can_parts[kind][slot];

    if (part == 0) {
        memset(asm_buf, 0, HEXNET_LABEL_NAME_MAX + 1);
        *parts = 0;
    } else if ((*parts & 0x01u) == 0) {
        /* part0 yokken ara parca — bos/kirik commit etme */
        ESP_LOGW(TAG, "CAN label skip orphan part kind=%u slot=%u part=%u",
                 (unsigned)kind, (unsigned)slot, (unsigned)part);
        return;
    }

    const size_t off = (size_t)part * 6u;
    if (off >= HEXNET_LABEL_NAME_MAX) {
        return;
    }
    size_t n = 6;
    if (off + n > HEXNET_LABEL_NAME_MAX) {
        n = HEXNET_LABEL_NAME_MAX - off;
    }
    memcpy(asm_buf + off, six, n);
    asm_buf[HEXNET_LABEL_NAME_MAX] = '\0';
    *parts |= (uint8_t)(1u << part);

    bool has_nul = false;
    for (size_t i = 0; i < n; ++i) {
        if (six[i] == 0) {
            has_nul = true;
            break;
        }
    }
    if (!has_nul && part < 3) {
        return; /* more chunks coming */
    }

    /* 0..part arasi tum partlar gelmis olmali (gap = kirik isim) */
    const uint8_t need = (uint8_t)((1u << (part + 1)) - 1u);
    if ((*parts & need) != need) {
        ESP_LOGW(TAG, "CAN label skip gap kind=%u slot=%u parts=0x%02X need=0x%02X",
                 (unsigned)kind, (unsigned)slot, (unsigned)*parts, (unsigned)need);
        return;
    }

    hexnet_label_slot_t *slotp = &s_labels.relays[slot];
    /* Bos string = slot temizle (tablet ikonu kaldirinca). */
    if (kind == 0) {
        copy_cstr(slotp->name, sizeof(slotp->name), asm_buf);
    } else {
        copy_cstr(slotp->icon, sizeof(slotp->icon), asm_buf);
    }
    /* Burst sirasinda NVS yazma — CAN RX kaybettiriyordu; debounce ile kaydet */
    s_ui_dirty = true;
    labels_schedule_nvs_save();
    *parts = 0;
    ESP_LOGI(TAG, "CAN label slot=%u kind=%u '%s'", (unsigned)slot, (unsigned)kind,
             kind == 0 ? slotp->name : slotp->icon);
}

bool hexnet_io_labels_peek_ui_dirty(void)
{
    return s_ui_dirty;
}

bool hexnet_io_labels_consume_ui_dirty(void)
{
    bool d = s_ui_dirty;
    s_ui_dirty = false;
    if (s_nvs_dirty) {
        labels_schedule_nvs_save();
    }
    return d;
}

void hexnet_io_labels_append_channels(cJSON *io_obj)
{
    if (!io_obj) {
        return;
    }
    const hexnet_io_profile_t *prof = hexnet_io_profile_get();

    cJSON *channels = cJSON_CreateObject();
    cJSON *relays = cJSON_CreateArray();
    cJSON *dims = cJSON_CreateArray();
    cJSON *sensors = cJSON_CreateArray();
    cJSON *water = cJSON_CreateArray();
    if (!channels || !relays || !dims || !sensors || !water) {
        cJSON_Delete(channels);
        cJSON_Delete(relays);
        cJSON_Delete(dims);
        cJSON_Delete(sensors);
        cJSON_Delete(water);
        return;
    }

    const int n_rel = (prof && prof->num_relays > 0) ? prof->num_relays : 12;
    for (int i = 0; i < n_rel && i < HEXNET_LABEL_RELAYS; ++i) {
        cJSON_AddItemToArray(
            relays,
            slot_to_json(i, &s_labels.relays[i], hexnet_io_profile_relay_slot_enabled((uint8_t)i) ? 1 : 0));
    }
    const int n_dim = (prof && prof->num_dims > 0) ? prof->num_dims : 8;
    for (int i = 0; i < n_dim && i < HEXNET_LABEL_DIMS; ++i) {
        cJSON_AddItemToArray(
            dims,
            slot_to_json(i, &s_labels.dimmers[i], hexnet_io_profile_dim_slot_enabled((uint8_t)i) ? 1 : 0));
    }
    for (int i = 0; i < HEXNET_LABEL_SENSORS; ++i) {
        cJSON_AddItemToArray(
            sensors,
            slot_to_json(i, &s_labels.sensors[i], hexnet_io_profile_sensor_slot_enabled((uint8_t)i) ? 1 : 0));
    }
    for (int i = 0; i < HEXNET_LABEL_WATER; ++i) {
        const int en = (prof && (prof->tank_enable_mask & (1u << i))) ? 1 : 0;
        cJSON_AddItemToArray(water, slot_to_json(i, &s_labels.water[i], en));
    }

    cJSON_AddItemToObject(channels, "relays", relays);
    cJSON_AddItemToObject(channels, "dimmers", dims);
    cJSON_AddItemToObject(channels, "sensors", sensors);
    cJSON_AddItemToObject(channels, "water", water);
    cJSON_AddItemToObject(io_obj, "channels", channels);
}
