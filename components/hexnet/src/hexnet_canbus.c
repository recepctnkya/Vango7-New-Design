#include "hexnet_canbus.h"
#include "panel_pinout.h"
#include "hexnet_io_profile.h"
#include <stdio.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"
#include "esp_err.h"
#include "esp_log.h"
#include "driver/twai.h"
#include <esp_timer.h>



#define GPIO_OUTPUT_PIN_SEL  ((1ULL << VANGO_CAN_TX_GPIO) | (1ULL << VANGO_CAN_RX_GPIO))

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

// Forward declarations
static void check_and_recover_from_errors(void);
static void monitor_can_health(void);

static const twai_timing_config_t t_config = TWAI_TIMING_CONFIG_500KBITS();
static const twai_filter_config_t f_config = TWAI_FILTER_CONFIG_ACCEPT_ALL();
static const twai_general_config_t g_config = TWAI_GENERAL_CONFIG_DEFAULT(TX_GPIO_NUM, RX_GPIO_NUM, TWAI_MODE_NO_ACK);




#define FRAME_1_ID  VANGO_CAN_FRAME_VOLTAGE_IO
#define FRAME_PROFILE_ID VANGO_CAN_FRAME_IO_PROFILE
#define FRAME_2_ID  VANGO_CAN_FRAME_ANALOG_DIM
#define FRAME_3_ID  VANGO_CAN_FRAME_RGB_SENSORS

static const char *TAG = "CAN_HANDLER";

// Global variables to store received data
uint16_t voltage = 0;
uint16_t outputs = 0;
uint16_t inputs = 0;
uint8_t analog_inputs[5] = {0};
uint8_t dimmable_outputs[4] = {0};
uint8_t rgb_values[4] = {0};
uint8_t rgb_enabled = 0; // RGB'nin aktif olup olmadığını tutar
uint8_t canbusConnection = 0;
uint8_t sensorTemp = 0;
uint8_t sensorHum = 0;


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
    if (index < 4) {
        return dimmable_outputs[index];
    }
    return 0;  // Hatalı index
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

// Function to send a CAN frame
void send_can_frame(uint32_t id, uint8_t *data) {
    twai_message_t message;
    message.identifier = id;
    message.rtr = 0;  // Data frame
    message.data_length_code = 8;  // Max CAN data length is 8 bytes
    memcpy(message.data, data, 8);

    // Send the message over the CAN bus
    esp_err_t res = twai_transmit(&message, pdMS_TO_TICKS(100));  // 100 ms timeout
    if (res != ESP_OK) {
        ESP_LOGE(TAG, "Failed to send frame ID: 0x%03X, error: 0x%x", (unsigned int)id, res);
        // Check if we need to recover from errors
        check_and_recover_from_errors();
    } else {
        last_successful_tx = esp_timer_get_time() / 1000; // Update successful TX timestamp
    }
}

void handle_rx_message(twai_message_t message) {
    
    switch (message.identifier) {
        case FRAME_1_ID:
            // Assign to global variables
            voltage = (message.data[0] << 8) | message.data[1];
            outputs = (message.data[2] << 8) | message.data[3];
            inputs = (message.data[4] << 8) | message.data[5];
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
            ESP_LOGW(TAG, "RX 0x110 seq=%u mask=0x%04X",
                     (unsigned)message.data[7], (unsigned)relay_mask);
            (void)hexnet_io_profile_apply_can_core(
                    relay_mask,
                    message.data[4],
                    message.data[5],
                    message.data[6],
                    motor,
                    rgb,
                    message.data[7]);
            break;
        }
        case FRAME_2_ID:
            // Assign to global arrays
            for (int i = 0; i < 4; i++) {
                analog_inputs[i] = message.data[i];
                // Assign to global RGB array
                dimmable_outputs[i] = message.data[i + 4];

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
        
        // Check if alert happened with timeout
        uint32_t alerts_triggered;
        esp_err_t alert_result = twai_read_alerts(&alerts_triggered, pdMS_TO_TICKS(100));
        
        if (alert_result == ESP_OK) {
            twai_status_info_t twaistatus;
            twai_get_status_info(&twaistatus);

            // Handle alerts
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
                //ESP_LOGW(TAG,"RX queue full, clearing");
                
                // Clear the queue aggressively to prevent further issues
                int cleared_count = 0;
                while (twai_receive(&message, 0) == ESP_OK) {
                    cleared_count++;
                    // Process the message if it's important, otherwise discard
                    if (message.identifier == FRAME_1_ID || message.identifier == FRAME_PROFILE_ID ||
                        message.identifier == FRAME_2_ID || message.identifier == FRAME_3_ID) {
                        handle_rx_message(message);
                    }
                }
            }

            // Check if message is received — kuyruktaki tum frame'leri isle (0x110 kacmasin)
            if (alerts_triggered & TWAI_ALERT_RX_DATA) {
                while (twai_receive(&message, 0) == ESP_OK) {
                    handle_rx_message(message);
                    canbusConnection = 1;
                }
            }
        } else if (alert_result == ESP_ERR_TIMEOUT) {
            // Timeout is normal, continue monitoring
        } else {
            ESP_LOGE(TAG, "Error reading alerts: 0x%x", alert_result);
        }
        
        // Proactive queue monitoring - check queue level and clear if getting full
        twai_status_info_t status;
        twai_get_status_info(&status);
        if (status.msgs_to_rx > 3) { // If more than 3 messages queued
            //ESP_LOGW(TAG, "RX queue full, clearing");
            int cleared_count = 0;
            while (twai_receive(&message, 0) == ESP_OK && cleared_count < 5) { // Clear up to 5 messages
                cleared_count++;
                if (message.identifier == FRAME_1_ID || message.identifier == FRAME_PROFILE_ID ||
                    message.identifier == FRAME_2_ID || message.identifier == FRAME_3_ID) {
                    handle_rx_message(message);
                }
            }
        }
        
        // Update connection status based on recent activity
        if (alerts_triggered & TWAI_ALERT_RX_DATA) {
            canbusConnection = 1;
        } else {
            // Only set to 0 if we haven't had activity for a while
            static uint32_t last_activity = 0;
            uint32_t current_time = esp_timer_get_time() / 1000;
            if (current_time - last_activity > 5000) { // 5 seconds without activity
                canbusConnection = 0;
            }
            if (alerts_triggered & TWAI_ALERT_RX_DATA) {
                last_activity = current_time;
            }
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
 
    //define twai task
    xTaskCreate(twai_task, "twai_task", 4096, NULL, 5, NULL);
    
    //define watchdog task
    xTaskCreate(can_watchdog_task, "can_watchdog", 3072, NULL, 3, NULL);
}
