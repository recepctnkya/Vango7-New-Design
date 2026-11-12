#include "hexnet_canbus.h"
#include "panel_pinout.h"
#include "hexnet_io_profile.h"
#include "hexnet_io_labels.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"
#include "freertos/idf_additions.h"
#include "esp_err.h"
#include "esp_log.h"
#include "esp_heap_caps.h"
#include "driver/twai.h"
#include <esp_timer.h>



#define GPIO_OUTPUT_PIN_SEL  ((1ULL << VANGO_CAN_TX_GPIO) | (1ULL << VANGO_CAN_RX_GPIO))

/* Display: IO SoT theme/lang apply (implemented in display_manager.c) */
extern void apply_theme_language_from_io(int theme, int language);

/* --------------------- Definitions and static variables ------------------ */
#define TX_GPIO_NUM             VANGO_CAN_TX_GPIO
#define RX_GPIO_NUM             VANGO_CAN_RX_GPIO
#define EXAMPLE_TAG             "TWAI Master"

static bool driver_installed = false;
#define POLLING_RATE_MS 1000

// Error recovery tracking variables
static uint32_t error_recovery_count = 0; // Error recovery counter
static uint32_t last_successful_tx = 0;   // Last successful transmission timestamp
static uint32_t last_health_check = 0;    // Last health check timestamp
static uint32_t last_recovery_attempt = 0; // Last recovery attempt timestamp

/* Async TX: UI/LVGL/shot must never block on twai_transmit or recovery. */
typedef struct {
    uint32_t id;
    uint8_t data[8];
} can_tx_item_t;

#define CAN_TX_QUEUE_LEN 32
static QueueHandle_t s_can_tx_q = NULL;
static TaskHandle_t s_can_tx_task = NULL;
static TaskHandle_t s_twai_task = NULL;
static TaskHandle_t s_can_wd_task = NULL;

// Forward declarations
static void check_and_recover_from_errors(void);
static void monitor_can_health(void);
static void can_tx_task(void *pvParameter);

static const twai_timing_config_t t_config = TWAI_TIMING_CONFIG_500KBITS();
static const twai_filter_config_t f_config = TWAI_FILTER_CONFIG_ACCEPT_ALL();
static const twai_general_config_t g_config = TWAI_GENERAL_CONFIG_DEFAULT(TX_GPIO_NUM, RX_GPIO_NUM, TWAI_MODE_NO_ACK);




#define FRAME_1_ID  VANGO_CAN_FRAME_VOLTAGE_IO
#define FRAME_PROFILE_ID VANGO_CAN_FRAME_IO_PROFILE
#define FRAME_RELAY_TYPE_LO 0x111
#define FRAME_RELAY_TYPE_HI 0x112
#define FRAME_LABEL_NAME    0x118
#define FRAME_LABEL_ICON    0x119
#define FRAME_2_ID  VANGO_CAN_FRAME_ANALOG_DIM
#define FRAME_2B_ID VANGO_CAN_FRAME_ANALOG_DIM_B
#define FRAME_3_ID  VANGO_CAN_FRAME_RGB_SENSORS

static uint8_t s_relay_types_can[16];
static uint8_t s_relay_types_have; /* bit0=lo bit1=hi */

static const char *TAG = "CAN_HANDLER";

// Global variables to store received data
uint16_t voltage = 0;
uint16_t outputs = 0;
uint16_t inputs = 0;
uint8_t analog_inputs[5] = {0};
uint8_t dimmable_outputs[8] = {0}; /* 0..3 via 0x200, 4..7 via 0x210 */
uint8_t rgb_values[4] = {0};
uint8_t rgb_enabled = 0; // RGB'nin aktif olup olmadığını tutar
uint8_t canbusConnection = 0;
uint8_t sensorTemp = 0;
uint8_t sensorHum = 0;
/* IO link flags from 0x100 data[6]: bit0=wifi bit1=ble bit2=mqtt */
static uint8_t s_io_link_flags = 0;
/* Son IO frame zamani (ms) — baglanti bayragi buna bagli */
static uint32_t s_can_last_rx_ms = 0;

#define CAN_LINK_WIFI  0x01u
#define CAN_LINK_BLE   0x02u
#define CAN_LINK_MQTT  0x04u


// Getter Functions
uint16_t get_voltage() {
    return voltage;
}

uint16_t get_outputs() {
    return outputs;
}

uint16_t get_inputs() {
    return inputs;
}
uint8_t get_sensorTemp() {
    return sensorTemp;
}
uint8_t get_sensorHum() {
    return sensorHum;
}

uint8_t get_analog_input(uint8_t index) {
    if (index < 5) {
        return analog_inputs[index];
    }
    return 0;  // Hatalı index
}

uint8_t get_dimmable_output(uint8_t index) {
    if (index < 8) {
        return dimmable_outputs[index];
    }
    return 0;  /* Hatalı index */
}

uint8_t get_rgb_value(uint8_t index) {
    if (index < 4) {
        return rgb_values[index];
    }
    return 0;  // Hatalı index
}


uint8_t get_canbus_connection_status() {
    return canbusConnection;
}

uint8_t get_io_wifi_link(void)
{
    return (canbusConnection && (s_io_link_flags & CAN_LINK_WIFI)) ? 1u : 0u;
}

uint8_t get_io_ble_link(void)
{
    return (canbusConnection && (s_io_link_flags & CAN_LINK_BLE)) ? 1u : 0u;
}

uint8_t get_io_mqtt_link(void)
{
    return (canbusConnection && (s_io_link_flags & CAN_LINK_MQTT)) ? 1u : 0u;
}

// Error recovery function
static void check_and_recover_from_errors() {
    uint32_t current_time = esp_timer_get_time() / 1000;
    
    // Prevent excessive recovery attempts (max once every 10 seconds)
    if (current_time - last_recovery_attempt < 10000) {
        return;
    }
    last_recovery_attempt = current_time;
    
    twai_status_info_t status;
    twai_get_status_info(&status);
    
    // Check for Bus-Off condition
    if (status.state == TWAI_STATE_BUS_OFF) {
        ESP_LOGW(TAG, "CAN Bus-Off detected! Attempting recovery...");
        error_recovery_count++;
        
        // Stop and restart the driver
        twai_stop();
        vTaskDelay(pdMS_TO_TICKS(100));
        
        if (twai_start() == ESP_OK) {
            ESP_LOGI(TAG, "CAN driver restarted successfully (recovery #%"PRIu32")", error_recovery_count);
        } else {
            ESP_LOGE(TAG, "Failed to restart CAN driver (recovery #%"PRIu32")", error_recovery_count);
        }
    }
    
    // Check for high error counts
    if (status.tx_error_counter > 100 || status.rx_error_counter > 100) {
        ESP_LOGW(TAG, "High error counts detected");
        
        // Reset error counters
        twai_initiate_recovery();
        ESP_LOGI(TAG, "CAN error recovery initiated");
    }
}

// Watchdog monitoring function
static void monitor_can_health() {
    uint32_t current_time = esp_timer_get_time() / 1000;
    
    // Only check health every 5 seconds to prevent excessive calls
    if (current_time - last_health_check < 5000) {
        return;
    }
    last_health_check = current_time;
    
    // Check if we haven't had a successful transmission in 10 seconds
    if (last_successful_tx > 0 && (current_time - last_successful_tx) > 10000) {
        ESP_LOGW(TAG, "No successful CAN transmission");
        check_and_recover_from_errors();
    }
}

// Watchdog task to monitor CAN bus health
void can_watchdog_task(void *pvParameter) {
    uint32_t last_status_check = 0;
    uint32_t consecutive_failures = 0;
    
    while(1) {
        uint32_t current_time = esp_timer_get_time() / 1000;
        
        // Check CAN status every 5 seconds
        if (current_time - last_status_check >= 5000) {
            twai_status_info_t status;
            twai_get_status_info(&status);
            
            //ESP_LOGI(TAG, "CAN Status check");
            
            // Check for problematic states
            if (status.state == TWAI_STATE_BUS_OFF) {
                ESP_LOGE(TAG, "CAN Bus-Off state detected by watchdog!");
                check_and_recover_from_errors();
                consecutive_failures++;
            } else if (status.state == TWAI_STATE_RECOVERING) {
                ESP_LOGW(TAG, "CAN in recovery state");
            } else if (status.state == TWAI_STATE_RUNNING) {
                consecutive_failures = 0; // Reset failure counter on successful state
            }
            
            // If we have too many consecutive failures, try a full restart
            if (consecutive_failures >= 3) {
                ESP_LOGE(TAG, "Too many CAN failures, restarting");
                
                // Stop and uninstall driver
                twai_stop();
                vTaskDelay(pdMS_TO_TICKS(1000));
                twai_driver_uninstall();
                vTaskDelay(pdMS_TO_TICKS(1000));
                
                // Reinitialize
                twai_ini();
                if (driver_installed) {
                    ESP_LOGI(TAG, "CAN driver fully restarted successfully");
                    consecutive_failures = 0;
                } else {
                    ESP_LOGE(TAG, "Failed to restart CAN driver");
                }
            }
            
            last_status_check = current_time;
        }
        
        vTaskDelay(pdMS_TO_TICKS(1000)); // Check every second
    }
}

static void can_tx_task(void *pvParameter)
{
    (void)pvParameter;
    can_tx_item_t item;
    while (1) {
        if (xQueueReceive(s_can_tx_q, &item, portMAX_DELAY) != pdTRUE) {
            continue;
        }
        if (!driver_installed) {
            continue;
        }
        twai_message_t message = {0};
        message.identifier = item.id;
        message.data_length_code = 8;
        memcpy(message.data, item.data, 8);
        esp_err_t res = twai_transmit(&message, pdMS_TO_TICKS(50));
        if (res != ESP_OK) {
            ESP_LOGE(TAG, "TX fail id=0x%03X err=0x%x", (unsigned)item.id, res);
            check_and_recover_from_errors();
        } else {
            last_successful_tx = esp_timer_get_time() / 1000;
        }
    }
}

/* Non-blocking enqueue — safe from LVGL / shot / timers. */
void send_can_frame(uint32_t id, uint8_t *data)
{
    can_tx_item_t item;
    if (!data) {
        return;
    }
    if (s_can_tx_q == NULL) {
        ESP_LOGW(TAG, "TX queue not ready, drop id=0x%03X", (unsigned)id);
        return;
    }
    item.id = id;
    memcpy(item.data, data, 8);
    if (s_can_tx_task == NULL) {
        ESP_LOGW(TAG, "TX task missing, drop id=0x%03X", (unsigned)id);
        return;
    }
    if (xQueueSend(s_can_tx_q, &item, 0) != pdTRUE) {
        ESP_LOGW(TAG, "TX queue full, drop id=0x%03X", (unsigned)id);
    } else if (id == 0x720 || id == 0x730 || id == 0x750 ||
               id == VANGO_CAN_FRAME_IO_PROFILE_SET) {
        ESP_LOGI(TAG, "TX q id=0x%03X d0=%u d1=%u", (unsigned)id, data[0], data[1]);
    }
}

void hexnet_can_push_io_profile_set(void)
{
    const hexnet_io_profile_t *p = hexnet_io_profile_get();
    uint8_t data[8] = {0};
    if (!p) {
        return;
    }
    data[0] = 1; /* version */
    data[1] = (uint8_t)((p->motor_enabled ? 0x01u : 0u) | (p->rgb_enabled ? 0x02u : 0u));
    data[2] = (uint8_t)(p->relay_enable_mask & 0xFFu);
    data[3] = (uint8_t)((p->relay_enable_mask >> 8) & 0xFFu);
    data[4] = p->dim_enable_mask;
    data[5] = p->sensor_enable_mask;
    data[6] = p->tank_enable_mask;
    data[7] = 0;
    ESP_LOGI(TAG, "TX 0x761 profile-set mask=0x%04X", (unsigned)p->relay_enable_mask);
    send_can_frame(VANGO_CAN_FRAME_IO_PROFILE_SET, data);
}

static void can_push_str_chunks(uint32_t id, uint8_t slot, const char *s)
{
    char buf[24];
    memset(buf, 0, sizeof(buf));
    if (s) {
        strncpy(buf, s, sizeof(buf) - 1);
    }
    for (int part = 0; part < 4; ++part) {
        uint8_t data[8] = {0};
        data[0] = slot;
        data[1] = (uint8_t)part;
        memcpy(&data[2], &buf[part * 6], 6);
        send_can_frame(id, data);
        bool more = false;
        for (int i = (part + 1) * 6; i < 24; ++i) {
            if (buf[i] != '\0') {
                more = true;
                break;
            }
        }
        if (!more) {
            break;
        }
    }
}

void hexnet_can_push_relay_label(int slot, const char *name, const char *icon)
{
    if (slot < 0 || slot > 15) {
        return;
    }
    ESP_LOGW(TAG, "TX 0x762/763 label slot=%d name='%s' ico='%s'",
             slot, name ? name : "", icon ? icon : "");
    /* Ikon once (IO commit tetikler); isim sonra. Cift burst: CAN kaybi toleransi.
     * LVGL lock altinda delay YOK — sadece kuyruk. */
    for (int retry = 0; retry < 2; ++retry) {
        can_push_str_chunks(VANGO_CAN_FRAME_LABEL_ICON_SET, (uint8_t)slot, icon ? icon : "");
        can_push_str_chunks(VANGO_CAN_FRAME_LABEL_NAME_SET, (uint8_t)slot, name ? name : "");
    }
}

void handle_rx_message(twai_message_t message) {
    
    switch (message.identifier) {
        case FRAME_1_ID:
            // Assign to global variables
            voltage = (message.data[0] << 8) | message.data[1];
            outputs = (message.data[2] << 8) | message.data[3];
            inputs = (message.data[4] << 8) | message.data[5];
            if (message.data_length_code >= 7) {
                s_io_link_flags = message.data[6];
            }
            break;
        case FRAME_PROFILE_ID: {
            /* IO Module flash SoT: röle/dim/sensor enable maskeleri */
            if (message.data_length_code < 8) {
                ESP_LOGW(TAG, "RX 0x110 dlc=%d (kisa)", (int)message.data_length_code);
                break;
            }
            const uint16_t relay_mask = (uint16_t)message.data[2] | ((uint16_t)message.data[3] << 8);
            const uint8_t motor = (message.data[1] & 0x01u) ? 1u : 0u;
            const uint8_t rgb = (message.data[1] & 0x02u) ? 1u : 0u;
            const int theme = (message.data[1] >> 2) & 0x03; /* 0=light,1=dark (IO SoT) */
            const int lang = (message.data[1] >> 4) & 0x01;
            ESP_LOGW(TAG, "RX 0x110 seq=%u mask=0x%04X Th=%d Lng=%d",
                     (unsigned)message.data[7], (unsigned)relay_mask, theme, lang);
            (void)hexnet_io_profile_apply_can_core(
                    relay_mask,
                    message.data[4],
                    message.data[5],
                    message.data[6],
                    motor,
                    rgb,
                    message.data[7]);
            apply_theme_language_from_io(theme, lang);
            break;
        }
        case FRAME_RELAY_TYPE_LO:
            ESP_LOGW(TAG, "RX 0x111 types lo t0=%u t1=%u", message.data[0], message.data[1]);
            memcpy(&s_relay_types_can[0], message.data, 8);
            s_relay_types_have |= 0x01u;
            if (s_relay_types_have == 0x03u) {
                (void)hexnet_io_profile_apply_can_relay_types(s_relay_types_can);
                s_relay_types_have = 0;
            }
            break;
        case FRAME_RELAY_TYPE_HI:
            ESP_LOGW(TAG, "RX 0x112 types hi");
            memcpy(&s_relay_types_can[8], message.data, 8);
            s_relay_types_have |= 0x02u;
            if (s_relay_types_have == 0x03u) {
                (void)hexnet_io_profile_apply_can_relay_types(s_relay_types_can);
                s_relay_types_have = 0;
            }
            break;
        case FRAME_LABEL_NAME:
            if (message.data_length_code >= 8) {
                ESP_LOGW(TAG, "RX 0x118 name slot=%u part=%u", message.data[0], message.data[1]);
                hexnet_io_labels_can_chunk(0, message.data[0], message.data[1], &message.data[2]);
            }
            break;
        case FRAME_LABEL_ICON:
            if (message.data_length_code >= 8) {
                ESP_LOGW(TAG, "RX 0x119 icon slot=%u part=%u", message.data[0], message.data[1]);
                hexnet_io_labels_can_chunk(1, message.data[0], message.data[1], &message.data[2]);
            }
            break;
        case FRAME_2_ID:
            /* analog[0..3] + dim[0..3] */
            for (int i = 0; i < 4; i++) {
                analog_inputs[i] = message.data[i];
                dimmable_outputs[i] = message.data[i + 4];
            }
            break;
        case FRAME_2B_ID:
            /* dim[4..7] — IO v0.60 8-dimmer */
            for (int i = 0; i < 4; i++) {
                dimmable_outputs[i + 4] = message.data[i];
            }
            break;
        case FRAME_3_ID:
             rgb_values[0] = message.data[0];
             rgb_values[1] = message.data[1];
             rgb_values[2] = message.data[2];
             rgb_values[3] = message.data[3];
             sensorTemp = message.data[4];
             sensorHum = message.data[5];
            break;
        default:
            break;
    }

}

int twaiCounter = 0;
void twai_task(void *pvParameter)
{
    twai_message_t message;
    
    while(1)
    {
        if (!driver_installed) {
            // Driver not installed
            vTaskDelay(pdMS_TO_TICKS(1000));
            return;
        }
        
        /* Check if alert happened with timeout — init 0: TIMEOUT'ta garbage kullanma */
        uint32_t alerts_triggered = 0;
        esp_err_t alert_result = twai_read_alerts(&alerts_triggered, pdMS_TO_TICKS(100));
        bool got_rx = false;

        if (alert_result == ESP_OK) {
            twai_status_info_t twaistatus;
            twai_get_status_info(&twaistatus);

            if (alerts_triggered & TWAI_ALERT_ERR_PASS) {
                ESP_LOGW(TAG,"Alert: TWAI controller has become error passive.");
                check_and_recover_from_errors();
            }

            if (alerts_triggered & TWAI_ALERT_BUS_ERROR) {
                ESP_LOGW(TAG,"Alert: A (Bit, Stuff, CRC, Form, ACK) error has occurred on the bus.");
                ESP_LOGW(TAG,"Bus error count: %"PRIu32, twaistatus.bus_error_count);
                check_and_recover_from_errors();
            }

            if (alerts_triggered & TWAI_ALERT_RX_QUEUE_FULL) {
                /* Once filtre 0x100/110/2/3 idi — 0x111/112/118/119 dusuyordu (isim sync kirik) */
                while (twai_receive(&message, 0) == ESP_OK) {
                    handle_rx_message(message);
                    got_rx = true;
                }
            }

            if (alerts_triggered & TWAI_ALERT_RX_DATA) {
                while (twai_receive(&message, 0) == ESP_OK) {
                    handle_rx_message(message);
                    got_rx = true;
                }
            }
        } else if (alert_result == ESP_ERR_TIMEOUT) {
            /* normal — asagida kuyruk/timeout baglanti kontrolu */
        } else {
            ESP_LOGE(TAG, "Error reading alerts: 0x%x", alert_result);
        }

        /* Kuyrukta kalan frame (alert kacsa bile) — tum SoT ID'leri isle */
        twai_status_info_t status;
        twai_get_status_info(&status);
        if (status.msgs_to_rx > 0) {
            int cleared_count = 0;
            while (twai_receive(&message, 0) == ESP_OK && cleared_count < 32) {
                cleared_count++;
                handle_rx_message(message);
                got_rx = true;
            }
        }

        /*
         * Baglanti: son IO RX'ten 5sn gecmediyse bagli.
         * Eski kod last_activity'yi hic guncellemiyordu (else icinde RX_DATA imkansiz)
         * → 5sn sonra canbusConnection surekli 0'a dusuyordu (yanlis "bagli degil").
         */
        const uint32_t now_ms = (uint32_t)(esp_timer_get_time() / 1000);
        if (got_rx) {
            s_can_last_rx_ms = now_ms;
            canbusConnection = 1;
        } else if (s_can_last_rx_ms != 0 && (now_ms - s_can_last_rx_ms) > 5000u) {
            canbusConnection = 0;
        }

        vTaskDelay(pdMS_TO_TICKS(100)); // 100ms delay for stable operation

        if(twaiCounter == 100) {    
            twaiCounter = 0;
        }
        twaiCounter++;
    }
}

void twai_ini(void)
{
    esp_err_t ret;
    
    // Install TWAI driver
    ret = twai_driver_install(&g_config, &t_config, &f_config);
    if (ret == ESP_OK) {
        ESP_LOGI(TAG,"Driver installed");
    } else {
        ESP_LOGE(TAG,"Failed to install driver: 0x%x", ret);
        return;
    }
    
    // Start TWAI driver
    ret = twai_start();
    if (ret == ESP_OK) {
        ESP_LOGI(TAG,"Driver started");
    } else {
        ESP_LOGE(TAG,"Failed to start driver: 0x%x", ret);
        twai_driver_uninstall();
        return;
    }

    // Reconfigure alerts to detect frame receive, Bus-Off error and RX queue full states
    uint32_t alerts_to_enable = TWAI_ALERT_RX_DATA | TWAI_ALERT_ERR_PASS | TWAI_ALERT_BUS_ERROR | 
                               TWAI_ALERT_RX_QUEUE_FULL | TWAI_ALERT_TX_FAILED | TWAI_ALERT_TX_SUCCESS;
    ret = twai_reconfigure_alerts(alerts_to_enable, NULL);
    if (ret == ESP_OK) {
        ESP_LOGI(TAG,"CAN Alerts reconfigured");
    } else {
        ESP_LOGE(TAG,"Failed to reconfigure alerts: 0x%x", ret);
        twai_stop();
        twai_driver_uninstall();
        return;
    }

    // Initialize monitoring variables
    last_successful_tx = esp_timer_get_time() / 1000;
    error_recovery_count = 0;
    last_health_check = 0;
    last_recovery_attempt = 0;

    // TWAI driver is now successfully installed and started
    driver_installed = true;
    ESP_LOGI(TAG,"CAN bus initialization completed successfully");

    if (s_can_tx_q == NULL) {
        s_can_tx_q = xQueueCreate(CAN_TX_QUEUE_LEN, sizeof(can_tx_item_t));
    }

    /* TX stack INTERNAL — SPIRAM stack ile 0x720 display→IO sessiz dusuyordu.
     * RX/WD SPIRAM OK (tablet→IO→display calisiyordu). */
    if (s_can_tx_task == NULL && s_can_tx_q != NULL) {
        BaseType_t ok = xTaskCreatePinnedToCore(can_tx_task, "can_tx", 3072, NULL, 6,
                                                &s_can_tx_task, 1);
        if (ok != pdPASS) {
            ESP_LOGE(TAG, "can_tx task FAIL — display→IO komutlari dusmez");
            s_can_tx_task = NULL;
        }
    }
    if (s_twai_task == NULL) {
        xTaskCreatePinnedToCoreWithCaps(twai_task, "twai_task", 4096, NULL, 5, &s_twai_task, 1,
                                        MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    }
    if (s_can_wd_task == NULL) {
        xTaskCreatePinnedToCoreWithCaps(can_watchdog_task, "can_watchdog", 3072, NULL, 3,
                                        &s_can_wd_task, 1, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    }
}
