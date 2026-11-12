#include "hexnet_io_profile.h"
#include "hexnet_io_labels.h"

#include <string.h>

#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"

void save_panel_configuration_to_nvs(int totalOutps, int buffer1[16], int totalSensors, int buffer2[5], int totalDims, int buffer3[8]);
void load_panel_configuration_from_nvs(int *totalOutpts, int buffer1[16], int *totalSensors, int buffer2[5], int *totalDims, int buffer3[8]);
void set_RGBTurnONOFF(int val);
void hexnet_can_push_io_profile_set(void);
#include "nvs.h"

static const char *TAG = "HEXNET_IO_PROFILE";

#define NVS_NS "hexnet"
#define NVS_KEY "io_profile"
#define PROFILE_MAGIC 0x314F4948u /* "HIO1" */
#define PROFILE_VERSION 2

typedef struct __attribute__((packed)) {
    uint32_t magic;
    uint8_t version;
    uint8_t num_relays;
    uint8_t num_dims;
    uint8_t num_sensors;
    uint8_t num_tanks;
    uint8_t motor_enabled;
    uint8_t rgb_enabled;
    uint16_t relay_enable_mask;
    uint8_t dim_enable_mask;
    uint8_t sensor_enable_mask;
    uint8_t tank_enable_mask;
    uint8_t relay_type[HEXNET_IO_RELAY_SLOTS];
    uint8_t dim_type[HEXNET_IO_DIM_SLOTS];
    uint8_t sensor_type[HEXNET_IO_SENSOR_SLOTS];
    uint8_t link_flags;
    uint8_t reserved;
} hexnet_io_profile_blob_t;

static hexnet_io_profile_t s_profile;
static bool s_loaded;
static volatile bool s_ui_dirty;

extern int numOfOutputs;
extern int numOfDims;
extern int numOfSensors;
extern int outputsBuffer[16];
extern int sensorsBuffer[5];
extern int dimsBuffer[8];


static uint8_t count_bits8(uint8_t v)
{
    uint8_t n = 0;
    while (v) {
        n += (uint8_t)(v & 1u);
        v >>= 1;
    }
    return n;
}

static uint8_t count_bits16(uint16_t v)
{
    uint8_t n = 0;
    while (v) {
        n += (uint8_t)(v & 1u);
        v >>= 1;
    }
    return n;
}

static void profile_set_defaults(hexnet_io_profile_t *p)
{
    memset(p, 0, sizeof(*p));
    p->num_relays = 12;
    p->num_dims = 8;
    p->num_sensors = 5;
    p->num_tanks = 3;
    p->motor_enabled = 1;
    p->rgb_enabled = 1;
    p->relay_enable_mask = 0x0FFFu;
    p->dim_enable_mask = 0xFFu; /* IO/tablet ile ayni: 8 dimmer */
    p->sensor_enable_mask = 0x1Fu;
    p->tank_enable_mask = 0x07u;
    for (int i = 0; i < HEXNET_IO_RELAY_SLOTS; ++i) {
        p->relay_type[i] = 1;
    }
    for (int i = 0; i < HEXNET_IO_DIM_SLOTS; ++i) {
        p->dim_type[i] = 1;
    }
    for (int i = 0; i < HEXNET_IO_SENSOR_SLOTS; ++i) {
        p->sensor_type[i] = 1;
    }
}

static void profile_recompute_counts(hexnet_io_profile_t *p)
{
    p->num_relays = count_bits16(p->relay_enable_mask);
    p->num_dims = count_bits8(p->dim_enable_mask);
    p->num_sensors = count_bits8(p->sensor_enable_mask);
    p->num_tanks = count_bits8(p->tank_enable_mask);
    if (p->num_relays == 0) {
        p->num_relays = 1;
        p->relay_enable_mask |= 1u;
    }
}

static void blob_from_profile(const hexnet_io_profile_t *p, hexnet_io_profile_blob_t *b)
{
    memset(b, 0, sizeof(*b));
    b->magic = PROFILE_MAGIC;
    b->version = PROFILE_VERSION;
    b->num_relays = p->num_relays;
    b->num_dims = p->num_dims;
    b->num_sensors = p->num_sensors;
    b->num_tanks = p->num_tanks;
    b->motor_enabled = (uint8_t)(p->motor_enabled & 0x03u);
    b->rgb_enabled = p->rgb_enabled ? 1 : 0;
    b->relay_enable_mask = p->relay_enable_mask;
    b->dim_enable_mask = p->dim_enable_mask;
    b->sensor_enable_mask = p->sensor_enable_mask;
    b->tank_enable_mask = p->tank_enable_mask;
    memcpy(b->relay_type, p->relay_type, sizeof(b->relay_type));
    memcpy(b->dim_type, p->dim_type, sizeof(b->dim_type));
    memcpy(b->sensor_type, p->sensor_type, sizeof(b->sensor_type));
    b->link_flags = p->link_flags;
}

static void profile_from_blob(const hexnet_io_profile_blob_t *b, hexnet_io_profile_t *p)
{
    memset(p, 0, sizeof(*p));
    p->num_relays = b->num_relays;
    p->num_dims = b->num_dims;
    p->num_sensors = b->num_sensors;
    p->num_tanks = b->num_tanks;
    p->motor_enabled = b->motor_enabled;
    p->rgb_enabled = b->rgb_enabled;
    p->relay_enable_mask = b->relay_enable_mask;
    p->dim_enable_mask = b->dim_enable_mask;
    p->sensor_enable_mask = b->sensor_enable_mask;
    p->tank_enable_mask = b->tank_enable_mask;
    memcpy(p->relay_type, b->relay_type, sizeof(p->relay_type));
    memcpy(p->dim_type, b->dim_type, sizeof(p->dim_type));
    memcpy(p->sensor_type, b->sensor_type, sizeof(p->sensor_type));
    p->link_flags = b->link_flags;
    profile_recompute_counts(p);
}

static void profile_from_legacy_panel(void)
{
    profile_set_defaults(&s_profile);

    int out_cnt = 0;
    int sens_cnt = 0;
    int dim_cnt = 0;
    load_panel_configuration_from_nvs(&out_cnt, outputsBuffer, &sens_cnt, sensorsBuffer, &dim_cnt, dimsBuffer);

    if (out_cnt > 0 && out_cnt <= HEXNET_IO_RELAY_SLOTS) {
        s_profile.relay_enable_mask = (uint16_t)((1u << out_cnt) - 1u);
        s_profile.num_relays = (uint8_t)out_cnt;
    }
    for (int i = 0; i < HEXNET_IO_RELAY_SLOTS; ++i) {
        if (outputsBuffer[i] >= 1 && outputsBuffer[i] <= 18) {
            s_profile.relay_type[i] = (uint8_t)outputsBuffer[i];
            s_profile.relay_enable_mask |= (uint16_t)(1u << i);
        } else if (outputsBuffer[i] == 0) {
            s_profile.relay_enable_mask &= (uint16_t)~(1u << i);
        }
    }

    if (dim_cnt > 0 && dim_cnt <= HEXNET_IO_DIM_SLOTS) {
        s_profile.dim_enable_mask = (uint8_t)((1u << dim_cnt) - 1u);
    }
    for (int i = 0; i < HEXNET_IO_DIM_SLOTS; ++i) {
        if (dimsBuffer[i] >= 1 && dimsBuffer[i] <= 8) {
            s_profile.dim_type[i] = (uint8_t)dimsBuffer[i];
            s_profile.dim_enable_mask |= (uint8_t)(1u << i);
        } else {
            s_profile.dim_enable_mask &= (uint8_t)~(1u << i);
        }
    }

    if (sens_cnt > 0 && sens_cnt <= HEXNET_IO_SENSOR_SLOTS) {
        s_profile.sensor_enable_mask = (uint8_t)((1u << sens_cnt) - 1u);
    }
    s_profile.tank_enable_mask = 0;
    for (int i = 0; i < HEXNET_IO_SENSOR_SLOTS; ++i) {
        if (sensorsBuffer[i]) {
            s_profile.sensor_enable_mask |= (uint8_t)(1u << i);
            s_profile.sensor_type[i] = (uint8_t)sensorsBuffer[i];
            if (i >= 2) {
                s_profile.tank_enable_mask |= (uint8_t)(1u << (i - 2));
            }
        } else {
            s_profile.sensor_enable_mask &= (uint8_t)~(1u << i);
        }
    }

    s_profile.motor_enabled = 1;
    s_profile.rgb_enabled = 1;
    profile_recompute_counts(&s_profile);
}

static esp_err_t profile_load_blob(void)
{
    nvs_handle_t handle;
    esp_err_t err = nvs_open(NVS_NS, NVS_READONLY, &handle);
    if (err != ESP_OK) {
        return err;
    }

    hexnet_io_profile_blob_t blob;
    size_t len = sizeof(blob);
    err = nvs_get_blob(handle, NVS_KEY, &blob, &len);
    nvs_close(handle);
    if (err != ESP_OK || len != sizeof(blob) || blob.magic != PROFILE_MAGIC || blob.version != PROFILE_VERSION) {
        return ESP_ERR_NOT_FOUND;
    }

    profile_from_blob(&blob, &s_profile);
    return ESP_OK;
}

void hexnet_io_profile_sync_legacy_globals(void)
{
    numOfOutputs = s_profile.num_relays;
    numOfDims = s_profile.num_dims;
    numOfSensors = s_profile.num_sensors;

    for (int i = 0; i < HEXNET_IO_RELAY_SLOTS; ++i) {
        outputsBuffer[i] = (s_profile.relay_enable_mask & (1u << i)) ? (int)s_profile.relay_type[i] : 0;
        if (outputsBuffer[i] == 0 && (s_profile.relay_enable_mask & (1u << i))) {
            outputsBuffer[i] = 1;
        }
    }
    for (int i = 0; i < HEXNET_IO_DIM_SLOTS; ++i) {
        dimsBuffer[i] = (s_profile.dim_enable_mask & (1u << i)) ? (int)s_profile.dim_type[i] : 0;
        if (dimsBuffer[i] == 0 && (s_profile.dim_enable_mask & (1u << i))) {
            dimsBuffer[i] = 1;
        }
    }
    for (int i = 0; i < HEXNET_IO_SENSOR_SLOTS; ++i) {
        sensorsBuffer[i] = (s_profile.sensor_enable_mask & (1u << i)) ? (int)s_profile.sensor_type[i] : 0;
        if (sensorsBuffer[i] == 0 && (s_profile.sensor_enable_mask & (1u << i))) {
            sensorsBuffer[i] = 1;
        }
    }

    /* Sadece RAM legacy buffer. NVS blob zaten hexnet_io_profile_save ile yazilir.
     * Her switch'te 30+ nvs_write_int LVGL task'inda WDT/reset yapabiliyordu. */
}

static void profile_apply_hw_enable_flags(void)
{
    set_RGBTurnONOFF(s_profile.rgb_enabled ? 1 : 0);
}

void hexnet_io_profile_init(void)
{
    if (profile_load_blob() == ESP_OK) {
        ESP_LOGI(TAG, "Profil NVS yuklendi: R=%u D=%u S=%u tank=%u motor=%u rgb=%u link=0x%02X",
                 (unsigned)s_profile.num_relays,
                 (unsigned)s_profile.num_dims,
                 (unsigned)s_profile.num_sensors,
                 (unsigned)s_profile.num_tanks,
                 (unsigned)s_profile.motor_enabled,
                 (unsigned)s_profile.rgb_enabled,
                 (unsigned)s_profile.link_flags);
    } else {
        ESP_LOGW(TAG, "Profil blob yok; legacy NVS'ten migrate");
        profile_from_legacy_panel();
        (void)hexnet_io_profile_save();
    }

    hexnet_io_profile_sync_legacy_globals();
    profile_apply_hw_enable_flags();
    hexnet_io_labels_init();
    s_loaded = true;
}

const hexnet_io_profile_t *hexnet_io_profile_get(void)
{
    return &s_profile;
}

/*
 * NVS asla LVGL / esp_timer callback'inden yazilmamali
 * (S3: spi_flash cache assert / Interrupt WDT → reset).
 * Ayrı dusuk oncelikli task + debounce.
 */
static bool s_nvs_dirty;
static int64_t s_local_edit_guard_us;
static QueueHandle_t s_save_q;
static TaskHandle_t s_save_task;

static void profile_save_task(void *arg)
{
    (void)arg;
    uint32_t token;
    for (;;) {
        if (xQueueReceive(s_save_q, &token, portMAX_DELAY) != pdTRUE) {
            continue;
        }
        /* LVGL frame / SPI flush bitsin */
        vTaskDelay(pdMS_TO_TICKS(100));
        while (xQueueReceive(s_save_q, &token, 0) == pdTRUE) {
            /* coalesce */
        }
        if (!s_nvs_dirty) {
            continue;
        }
        s_nvs_dirty = false;
        ESP_LOGI(TAG, "Profil NVS task yaziyor mask=0x%04X",
                 (unsigned)s_profile.relay_enable_mask);
        (void)hexnet_io_profile_save();
    }
}

static void profile_schedule_nvs_save(void)
{
    s_nvs_dirty = true;
    if (!s_save_q) {
        s_save_q = xQueueCreate(1, sizeof(uint32_t));
    }
    if (!s_save_task && s_save_q) {
        BaseType_t ok = xTaskCreate(profile_save_task, "io_prof_nvs", 4096, NULL, 3, &s_save_task);
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

static esp_err_t profile_commit_local(void)
{
    profile_recompute_counts(&s_profile);
    s_profile.link_flags |= HEXNET_IO_LINK_DISPLAY;
    hexnet_io_profile_sync_legacy_globals();
    /* IO 0x110 eski maskeyi geri yazmasin; 0x761 ile SoT guncelle */
    s_local_edit_guard_us = esp_timer_get_time() + 15000000LL;
    profile_schedule_nvs_save();
    hexnet_can_push_io_profile_set();
    ESP_LOGI(TAG, "Profil RAM+TX0x761 mask=0x%04X",
             (unsigned)s_profile.relay_enable_mask);
    return ESP_OK;
}

void hexnet_io_profile_poll_save(void)
{
    /* no-op: kayit ayri task */
}

esp_err_t hexnet_io_profile_set_relay(int index, bool enabled, int type)
{
    if (index < 0 || index >= HEXNET_IO_RELAY_SLOTS) {
        return ESP_ERR_INVALID_ARG;
    }
    if (type < 1) {
        type = 1;
    }
    if (type > 18) {
        type = 18;
    }
    /* Tip her zaman yazilir (pasifken de secim kaybolmasin). */
    s_profile.relay_type[index] = (uint8_t)type;
    if (enabled) {
        s_profile.relay_enable_mask |= (uint16_t)(1u << index);
    } else {
        s_profile.relay_enable_mask &= (uint16_t)~(1u << index);
    }
    return profile_commit_local();
}

esp_err_t hexnet_io_profile_set_dim(int index, bool enabled, int type)
{
    if (index < 0 || index >= HEXNET_IO_DIM_SLOTS) {
        return ESP_ERR_INVALID_ARG;
    }
    if (type < 1) {
        type = 1;
    }
    if (type > 8) {
        type = 8;
    }
    if (enabled) {
        s_profile.dim_enable_mask |= (uint8_t)(1u << index);
        s_profile.dim_type[index] = (uint8_t)type;
    } else {
        s_profile.dim_enable_mask &= (uint8_t)~(1u << index);
    }
    return profile_commit_local();
}

esp_err_t hexnet_io_profile_set_sensor(int index, bool enabled)
{
    if (index < 0 || index >= HEXNET_IO_SENSOR_SLOTS) {
        return ESP_ERR_INVALID_ARG;
    }
    if (enabled) {
        s_profile.sensor_enable_mask |= (uint8_t)(1u << index);
        if (s_profile.sensor_type[index] == 0) {
            s_profile.sensor_type[index] = 1;
        }
    } else {
        s_profile.sensor_enable_mask &= (uint8_t)~(1u << index);
    }
    return profile_commit_local();
}

esp_err_t hexnet_io_profile_set_tank(int index, bool enabled)
{
    if (index < 0 || index >= HEXNET_IO_TANK_SLOTS) {
        return ESP_ERR_INVALID_ARG;
    }
    if (enabled) {
        s_profile.tank_enable_mask |= (uint8_t)(1u << index);
    } else {
        s_profile.tank_enable_mask &= (uint8_t)~(1u << index);
    }
    return profile_commit_local();
}

esp_err_t hexnet_io_profile_set_motor(int index, bool enabled)
{
    if (index < 0 || index >= HEXNET_IO_MOTOR_SLOTS) {
        return ESP_ERR_INVALID_ARG;
    }
    if (enabled) {
        s_profile.motor_enabled |= (uint8_t)(1u << index);
    } else {
        s_profile.motor_enabled &= (uint8_t)~(1u << index);
    }
    s_profile.motor_enabled &= 0x03u;
    return profile_commit_local();
}

esp_err_t hexnet_io_profile_set_rgb(bool enabled)
{
    s_profile.rgb_enabled = enabled ? 1 : 0;
    return profile_commit_local();
}

esp_err_t hexnet_io_profile_save(void)
{
    hexnet_io_profile_blob_t blob;
    blob_from_profile(&s_profile, &blob);

    nvs_handle_t handle;
    esp_err_t err = nvs_open(NVS_NS, NVS_READWRITE, &handle);
    if (err != ESP_OK) {
        return err;
    }
    err = nvs_set_blob(handle, NVS_KEY, &blob, sizeof(blob));
    if (err == ESP_OK) {
        err = nvs_commit(handle);
    }
    nvs_close(handle);

    if (err == ESP_OK) {
        hexnet_io_profile_sync_legacy_globals();
        profile_apply_hw_enable_flags();
        ESP_LOGI(TAG, "Profil flash'a yazildi");
    }
    return err;
}

static int json_bool(const cJSON *item, int default_val)
{
    if (!item) {
        return default_val;
    }
    if (cJSON_IsBool(item)) {
        return cJSON_IsTrue(item) ? 1 : 0;
    }
    if (cJSON_IsNumber(item)) {
        return item->valueint ? 1 : 0;
    }
    if (cJSON_IsString(item) && item->valuestring) {
        if (strcmp(item->valuestring, "yes") == 0 || strcmp(item->valuestring, "true") == 0 ||
            strcmp(item->valuestring, "1") == 0) {
            return 1;
        }
    }
    return default_val;
}

static void apply_enable_mask_u16(const cJSON *arr, uint16_t *mask, uint8_t *types, int max_slots)
{
    if (!cJSON_IsArray(arr)) {
        return;
    }
    const int n = cJSON_GetArraySize(arr);
    for (int i = 0; i < n && i < max_slots; ++i) {
        const cJSON *item = cJSON_GetArrayItem(arr, i);
        int active = 1;
        int type_code = 1;
        if (cJSON_IsNumber(item)) {
            type_code = item->valueint;
            active = (type_code != 0) ? 1 : 0;
        } else if (cJSON_IsObject(item)) {
            const cJSON *en = cJSON_GetObjectItem(item, "enabled");
            const cJSON *act = cJSON_GetObjectItem(item, "active");
            const cJSON *tp = cJSON_GetObjectItem(item, "type");
            active = json_bool(en, json_bool(act, 1));
            if (cJSON_IsNumber(tp)) {
                type_code = tp->valueint;
            }
        }
        if (active) {
            *mask |= (uint16_t)(1u << i);
            if (type_code > 0) {
                types[i] = (uint8_t)type_code;
            }
        } else {
            *mask &= (uint16_t)~(1u << i);
            types[i] = 0;
        }
    }
}

static void apply_enable_mask_u8(const cJSON *arr, uint8_t *mask, uint8_t *types, int max_slots)
{
    if (!cJSON_IsArray(arr)) {
        return;
    }
    const int n = cJSON_GetArraySize(arr);
    for (int i = 0; i < n && i < max_slots; ++i) {
        const cJSON *item = cJSON_GetArrayItem(arr, i);
        int active = 1;
        int type_code = 1;
        if (cJSON_IsNumber(item)) {
            type_code = item->valueint;
            active = (type_code != 0) ? 1 : 0;
        } else if (cJSON_IsObject(item)) {
            const cJSON *en = cJSON_GetObjectItem(item, "enabled");
            const cJSON *act = cJSON_GetObjectItem(item, "active");
            const cJSON *tp = cJSON_GetObjectItem(item, "type");
            active = json_bool(en, json_bool(act, 1));
            if (cJSON_IsNumber(tp)) {
                type_code = tp->valueint;
            }
        }
        if (active) {
            *mask |= (uint8_t)(1u << i);
            if (type_code > 0) {
                types[i] = (uint8_t)type_code;
            }
        } else {
            *mask &= (uint8_t)~(1u << i);
            types[i] = 0;
        }
    }
}

esp_err_t hexnet_io_profile_apply_json(const cJSON *json, uint8_t link_flag)
{
    if (!json || !s_loaded) {
        return ESP_ERR_INVALID_ARG;
    }

    const cJSON *nested = cJSON_GetObjectItem(json, "io_profile");
    if (cJSON_IsObject(nested)) {
        (void)hexnet_io_profile_apply_json(nested, 0);
    }

    const cJSON *num_relays = cJSON_GetObjectItem(json, "numOfOutputs");
    const cJSON *num_dims = cJSON_GetObjectItem(json, "numOfDims");
    const cJSON *num_sensors = cJSON_GetObjectItem(json, "numOfSensors");
    const cJSON *num_tanks = cJSON_GetObjectItem(json, "numOfTanks");
    const cJSON *motor_en = cJSON_GetObjectItem(json, "motorEnabled");
    const cJSON *rgb_en = cJSON_GetObjectItem(json, "RGBEnabled");
    const cJSON *rgb_en2 = cJSON_GetObjectItem(json, "rgbEnabled");
    const cJSON *relay_mask = cJSON_GetObjectItem(json, "relayEnableMask");
    const cJSON *dim_mask = cJSON_GetObjectItem(json, "dimEnableMask");
    const cJSON *sensor_mask = cJSON_GetObjectItem(json, "sensorEnableMask");
    const cJSON *tank_mask = cJSON_GetObjectItem(json, "tankEnableMask");

    if (cJSON_IsNumber(num_relays) && num_relays->valueint >= 0 && num_relays->valueint <= HEXNET_IO_RELAY_SLOTS) {
        const int n = num_relays->valueint;
        s_profile.relay_enable_mask = (uint16_t)((n > 0) ? ((1u << n) - 1u) : 0);
    }
    if (cJSON_IsNumber(num_dims) && num_dims->valueint >= 0 && num_dims->valueint <= HEXNET_IO_DIM_SLOTS) {
        const int n = num_dims->valueint;
        s_profile.dim_enable_mask = (uint8_t)((n > 0) ? ((1u << n) - 1u) : 0);
    }
    if (cJSON_IsNumber(num_sensors) && num_sensors->valueint >= 0 && num_sensors->valueint <= HEXNET_IO_SENSOR_SLOTS) {
        const int n = num_sensors->valueint;
        s_profile.sensor_enable_mask = (uint8_t)((n > 0) ? ((1u << n) - 1u) : 0);
    }
    if (cJSON_IsNumber(num_tanks) && num_tanks->valueint >= 0 && num_tanks->valueint <= HEXNET_IO_TANK_SLOTS) {
        const int n = num_tanks->valueint;
        s_profile.tank_enable_mask = (uint8_t)((n > 0) ? ((1u << n) - 1u) : 0);
    }

    if (cJSON_IsNumber(relay_mask)) {
        s_profile.relay_enable_mask = (uint16_t)relay_mask->valueint;
    }
    if (cJSON_IsNumber(dim_mask)) {
        s_profile.dim_enable_mask = (uint8_t)dim_mask->valueint;
    }
    if (cJSON_IsNumber(sensor_mask)) {
        s_profile.sensor_enable_mask = (uint8_t)sensor_mask->valueint;
    }
    if (cJSON_IsNumber(tank_mask)) {
        s_profile.tank_enable_mask = (uint8_t)tank_mask->valueint;
    }

    apply_enable_mask_u16(
        cJSON_GetObjectItem(json, "OutputsNameBuffer"),
        &s_profile.relay_enable_mask,
        s_profile.relay_type,
        HEXNET_IO_RELAY_SLOTS);
    apply_enable_mask_u16(
        cJSON_GetObjectItem(json, "relays"),
        &s_profile.relay_enable_mask,
        s_profile.relay_type,
        HEXNET_IO_RELAY_SLOTS);

    apply_enable_mask_u8(
        cJSON_GetObjectItem(json, "DimsNameBuffer"),
        &s_profile.dim_enable_mask,
        s_profile.dim_type,
        HEXNET_IO_DIM_SLOTS);
    apply_enable_mask_u8(
        cJSON_GetObjectItem(json, "dimmers"),
        &s_profile.dim_enable_mask,
        s_profile.dim_type,
        HEXNET_IO_DIM_SLOTS);

    apply_enable_mask_u8(
        cJSON_GetObjectItem(json, "SensorsNameBuffer"),
        &s_profile.sensor_enable_mask,
        s_profile.sensor_type,
        HEXNET_IO_SENSOR_SLOTS);
    apply_enable_mask_u8(
        cJSON_GetObjectItem(json, "sensors"),
        &s_profile.sensor_enable_mask,
        s_profile.sensor_type,
        HEXNET_IO_SENSOR_SLOTS);

    const cJSON *tanks = cJSON_GetObjectItem(json, "tanks");
    if (cJSON_IsArray(tanks)) {
        s_profile.tank_enable_mask = 0;
        const int n = cJSON_GetArraySize(tanks);
        for (int i = 0; i < n && i < HEXNET_IO_TANK_SLOTS; ++i) {
            const cJSON *item = cJSON_GetArrayItem(tanks, i);
            if (json_bool(item, 0)) {
                s_profile.tank_enable_mask |= (uint8_t)(1u << i);
            }
        }
    }

    if (motor_en) {
        if (json_bool(motor_en, s_profile.motor_enabled & 1u)) {
            s_profile.motor_enabled |= 0x01u;
        } else {
            s_profile.motor_enabled &= (uint8_t)~0x01u;
        }
    }
    if (rgb_en || rgb_en2) {
        s_profile.rgb_enabled = (uint8_t)json_bool(rgb_en ? rgb_en : rgb_en2, s_profile.rgb_enabled);
    }

    const cJSON *links = cJSON_GetObjectItem(json, "links");
    if (cJSON_IsObject(links)) {
        if (cJSON_GetObjectItem(links, "display")) {
            s_profile.link_flags = (uint8_t)(
                (s_profile.link_flags & ~(uint8_t)HEXNET_IO_LINK_DISPLAY) |
                (json_bool(cJSON_GetObjectItem(links, "display"), 0) ? HEXNET_IO_LINK_DISPLAY : 0));
        }
        if (cJSON_GetObjectItem(links, "ble")) {
            s_profile.link_flags = (uint8_t)(
                (s_profile.link_flags & ~(uint8_t)HEXNET_IO_LINK_BLE) |
                (json_bool(cJSON_GetObjectItem(links, "ble"), 0) ? HEXNET_IO_LINK_BLE : 0));
        }
        if (cJSON_GetObjectItem(links, "remote")) {
            s_profile.link_flags = (uint8_t)(
                (s_profile.link_flags & ~(uint8_t)HEXNET_IO_LINK_REMOTE) |
                (json_bool(cJSON_GetObjectItem(links, "remote"), 0) ? HEXNET_IO_LINK_REMOTE : 0));
        }
    }

    if (link_flag) {
        s_profile.link_flags |= link_flag;
    }

    profile_recompute_counts(&s_profile);
    return hexnet_io_profile_save();
}

void hexnet_io_profile_append_to_json(cJSON *parent, const char *object_name)
{
    if (!parent) {
        return;
    }
    const char *name = (object_name && object_name[0]) ? object_name : "io_profile";
    cJSON *io = cJSON_CreateObject();
    if (!io) {
        return;
    }

    cJSON_AddNumberToObject(io, "relays", s_profile.num_relays);
    cJSON_AddNumberToObject(io, "dimmers", s_profile.num_dims);
    cJSON_AddNumberToObject(io, "sensors", s_profile.num_sensors);
    cJSON_AddNumberToObject(io, "tanks", s_profile.num_tanks);
    cJSON_AddBoolToObject(io, "motor", (s_profile.motor_enabled & 0x01u) != 0);
    cJSON_AddNumberToObject(io, "motor_mask", s_profile.motor_enabled & 0x03u);
    cJSON_AddBoolToObject(io, "rgb", s_profile.rgb_enabled != 0);
    cJSON_AddNumberToObject(io, "relay_mask", s_profile.relay_enable_mask);
    cJSON_AddNumberToObject(io, "dim_mask", s_profile.dim_enable_mask);
    cJSON_AddNumberToObject(io, "sensor_mask", s_profile.sensor_enable_mask);
    cJSON_AddNumberToObject(io, "tank_mask", s_profile.tank_enable_mask);

    cJSON *links = cJSON_CreateObject();
    cJSON_AddBoolToObject(links, "display", (s_profile.link_flags & HEXNET_IO_LINK_DISPLAY) != 0);
    cJSON_AddBoolToObject(links, "ble", (s_profile.link_flags & HEXNET_IO_LINK_BLE) != 0);
    cJSON_AddBoolToObject(links, "remote", (s_profile.link_flags & HEXNET_IO_LINK_REMOTE) != 0);
    cJSON_AddItemToObject(io, "links", links);

    cJSON_AddItemToObject(parent, name, io);
}

bool hexnet_io_profile_relay_slot_enabled(uint8_t index)
{
    return index < HEXNET_IO_RELAY_SLOTS && (s_profile.relay_enable_mask & (1u << index)) != 0;
}

bool hexnet_io_profile_dim_slot_enabled(uint8_t index)
{
    return index < HEXNET_IO_DIM_SLOTS && (s_profile.dim_enable_mask & (1u << index)) != 0;
}

bool hexnet_io_profile_sensor_slot_enabled(uint8_t index)
{
    return index < HEXNET_IO_SENSOR_SLOTS && (s_profile.sensor_enable_mask & (1u << index)) != 0;
}

bool hexnet_io_profile_motor_enabled(void)
{
    return (s_profile.motor_enabled & 0x01u) != 0;
}

bool hexnet_io_profile_rgb_enabled(void)
{
    return s_profile.rgb_enabled != 0;
}

bool hexnet_io_profile_apply_can_core(uint16_t relay_mask,
                                      uint8_t dim_mask,
                                      uint8_t sensor_mask,
                                      uint8_t tank_mask,
                                      uint8_t motor_enabled,
                                      uint8_t rgb_enabled,
                                      uint8_t seq)
{
    static uint8_t s_last_seq = 0xFF;

    if (relay_mask == 0) {
        relay_mask = 1u;
    }

    const bool same =
        (s_profile.relay_enable_mask == relay_mask) &&
        (s_profile.dim_enable_mask == dim_mask) &&
        (s_profile.sensor_enable_mask == sensor_mask) &&
        (s_profile.tank_enable_mask == tank_mask) &&
        (((s_profile.motor_enabled & 0x01u) != 0) == (motor_enabled != 0)) &&
        ((s_profile.rgb_enabled != 0) == (rgb_enabled != 0));

    /* Display yerel edit: IO eski SoT gelirse ezme, 0x761 tekrar it */
    if (!same && (s_profile.link_flags & HEXNET_IO_LINK_DISPLAY) &&
        esp_timer_get_time() < s_local_edit_guard_us) {
        ESP_LOGW(TAG, "CAN 0x110 atlandi (local edit) can=0x%04X local=0x%04X — re-TX 0x761",
                 (unsigned)relay_mask, (unsigned)s_profile.relay_enable_mask);
        hexnet_can_push_io_profile_set();
        return false;
    }

    /* Hibrit: ayni seq + ayni icerik = heartbeat, UI dokunma */
    if (same && seq == s_last_seq) {
        s_profile.link_flags = (uint8_t)(s_profile.link_flags & ~(uint8_t)HEXNET_IO_LINK_DISPLAY);
        return false;
    }
    if (same && seq != s_last_seq) {
        s_last_seq = seq;
        s_profile.link_flags = (uint8_t)(s_profile.link_flags & ~(uint8_t)HEXNET_IO_LINK_DISPLAY);
        ESP_LOGW(TAG, "CAN 0x110 heartbeat seq=%u (icerik ayni R=%u)",
                 (unsigned)seq, (unsigned)s_profile.num_relays);
        return false;
    }

    s_profile.relay_enable_mask = relay_mask;
    s_profile.dim_enable_mask = dim_mask;
    s_profile.sensor_enable_mask = sensor_mask;
    s_profile.tank_enable_mask = tank_mask;
    if (motor_enabled) {
        s_profile.motor_enabled |= 0x01u;
    } else {
        s_profile.motor_enabled &= (uint8_t)~0x01u;
    }
    s_profile.rgb_enabled = rgb_enabled ? 1 : 0;
    /* Display bu profili uretmedi; IO flash SoT mirror */
    s_profile.link_flags = (uint8_t)(s_profile.link_flags & ~(uint8_t)HEXNET_IO_LINK_DISPLAY);

    for (int i = 0; i < HEXNET_IO_RELAY_SLOTS; ++i) {
        if ((relay_mask & (1u << i)) && s_profile.relay_type[i] == 0) {
            s_profile.relay_type[i] = 1;
        }
    }
    for (int i = 0; i < HEXNET_IO_DIM_SLOTS; ++i) {
        if ((dim_mask & (1u << i)) && s_profile.dim_type[i] == 0) {
            s_profile.dim_type[i] = 1;
        }
    }
    for (int i = 0; i < HEXNET_IO_SENSOR_SLOTS; ++i) {
        if ((sensor_mask & (1u << i)) && s_profile.sensor_type[i] == 0) {
            s_profile.sensor_type[i] = 1;
        }
    }

    profile_recompute_counts(&s_profile);
    hexnet_io_profile_sync_legacy_globals();
    profile_schedule_nvs_save(); /* NVS LVGL disinda */
    s_ui_dirty = true;
    s_last_seq = seq;
    ESP_LOGI(TAG, "CAN 0x110 IO SoT uygulandi: R=%u mask=0x%04X D=%u S=%u seq=%u",
             (unsigned)s_profile.num_relays,
             (unsigned)s_profile.relay_enable_mask,
             (unsigned)s_profile.num_dims,
             (unsigned)s_profile.num_sensors,
             (unsigned)seq);
    return true;
}

bool hexnet_io_profile_apply_can_relay_types(const uint8_t types[HEXNET_IO_RELAY_SLOTS])
{
    bool changed = false;
    if (!types || !s_loaded) {
        return false;
    }
    for (int i = 0; i < HEXNET_IO_RELAY_SLOTS; ++i) {
        uint8_t t = types[i];
        if (t > 18) {
            t = 1;
        }
        if (s_profile.relay_type[i] != t) {
            s_profile.relay_type[i] = t;
            changed = true;
        }
    }
    if (changed) {
        hexnet_io_profile_sync_legacy_globals();
        profile_schedule_nvs_save();
        s_ui_dirty = true;
        ESP_LOGI(TAG, "CAN 0x111/112 relay_type uygulandi");
    }
    return changed;
}

bool hexnet_io_profile_consume_ui_dirty(void)
{
    if (!s_ui_dirty) {
        return false;
    }
    s_ui_dirty = false;
    return true;
}
