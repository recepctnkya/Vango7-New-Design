/*
 * SPDX-FileCopyrightText: 2021-2024 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Unlicense OR CC0-1.0
 */

#include "hexnet_bluetooth.h"
#include "esp_heap_caps.h"
#include "freertos/idf_additions.h"


static const uint8_t spp_adv_data[] = {
    /* Flags */
    0x02,0x01,0x06,
    /* Complete List of 16-bit Service Class UUIDs */
    0x03,0x03,0xF0,0xAB,
    /* Complete Local Name in advertising */
    0x0E,0x09, 'V','A','N','G','O',' ','D','I','S','P','L','A','Y'
};


char* converted_json_data2 = NULL;


// Define the characteristic UUID
#define CHAR_UUID_REGS_DATA 0x2A56

uint8_t spp_cmd_buff[1000];

static uint16_t spp_mtu_size = 23;
static uint16_t spp_conn_id = 0xffff;
static esp_gatt_if_t spp_gatts_if = 0xff;
QueueHandle_t spp_uart_queue = NULL;
static QueueHandle_t cmd_cmd_queue = NULL;
static QueueHandle_t data_transmit_queue = NULL;

#ifdef SUPPORT_HEARTBEAT
static QueueHandle_t cmd_heartbeat_queue = NULL;
static uint8_t  heartbeat_s[9] = {'E','s','p','r','e','s','s','i','f'};
static bool enable_heart_ntf = false;
static uint8_t heartbeat_count_num = 0;
#endif

static bool enable_data_ntf = false;
static bool is_connected = false;
/* Dash + LVGL bitmeden advertise etme: BLE connect + config_save thread
 * internal DRAM yetmezse osi_thread_create → vQueueDelete assert → reset döngüsü. */
static bool s_adv_allowed = false;
static bool s_adv_data_ready = false;
static esp_bd_addr_t spp_remote_bda = {0x0,};
static TimerHandle_t s_periodic_timer = NULL;

// MTU negotiation state tracking
static bool mtu_negotiation_pending = false;

static bool ble_status_tx_ready(void)
{
    return is_connected && enable_data_ntf && spp_mtu_size >= 100 && converted_json_data2 != NULL;
}

static uint16_t spp_handle_table[SPP_IDX_NB];

static esp_ble_adv_params_t spp_adv_params = {
    .adv_int_min        = 0x20,
    .adv_int_max        = 0x40,
    .adv_type           = ADV_TYPE_IND,
    .own_addr_type      = BLE_ADDR_TYPE_PUBLIC,
    .channel_map        = ADV_CHNL_ALL,
    .adv_filter_policy  = ADV_FILTER_ALLOW_SCAN_ANY_CON_ANY,
};

struct gatts_profile_inst {
    esp_gatts_cb_t gatts_cb;
    uint16_t gatts_if;
    uint16_t app_id;
    uint16_t conn_id;
    uint16_t service_handle;
    esp_gatt_srvc_id_t service_id;
    uint16_t char_handle;
    esp_bt_uuid_t char_uuid;
    esp_gatt_perm_t perm;
    esp_gatt_char_prop_t property;
    uint16_t descr_handle;
    esp_bt_uuid_t descr_uuid;
};

typedef struct spp_receive_data_node{
    int32_t len;
    uint8_t * node_buff;
    struct spp_receive_data_node * next_node;
}spp_receive_data_node_t;

static spp_receive_data_node_t * temp_spp_recv_data_node_p1 = NULL;
static spp_receive_data_node_t * temp_spp_recv_data_node_p2 = NULL;

typedef struct spp_receive_data_buff{
    int32_t node_num;
    int32_t buff_size;
    spp_receive_data_node_t * first_node;
}spp_receive_data_buff_t;

static spp_receive_data_buff_t SppRecvDataBuff = {
    .node_num   = 0,
    .buff_size  = 0,
    .first_node = NULL
};

static void gatts_profile_event_handler(esp_gatts_cb_event_t event, esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param);

/* One gatt-based profile one app_id and one gatts_if, this array will store the gatts_if returned by ESP_GATTS_REG_EVT */
static struct gatts_profile_inst spp_profile_tab[SPP_PROFILE_NUM] = {
    [SPP_PROFILE_APP_IDX] = {
        .gatts_cb = gatts_profile_event_handler,
        .gatts_if = ESP_GATT_IF_NONE,       /* Not get the gatt_if, so initial is ESP_GATT_IF_NONE */
    },
};

/*
 *  SPP PROFILE ATTRIBUTES
 ****************************************************************************************
 */

#define CHAR_DECLARATION_SIZE   (sizeof(uint8_t))
static const uint16_t primary_service_uuid = ESP_GATT_UUID_PRI_SERVICE;
static const uint16_t character_declaration_uuid = ESP_GATT_UUID_CHAR_DECLARE;
static const uint16_t character_client_config_uuid = ESP_GATT_UUID_CHAR_CLIENT_CONFIG;

static const uint8_t char_prop_read_notify = ESP_GATT_CHAR_PROP_BIT_READ|ESP_GATT_CHAR_PROP_BIT_NOTIFY;
static const uint8_t char_prop_read_write = ESP_GATT_CHAR_PROP_BIT_WRITE_NR|ESP_GATT_CHAR_PROP_BIT_READ;

#ifdef SUPPORT_HEARTBEAT
static const uint8_t char_prop_read_write_notify = ESP_GATT_CHAR_PROP_BIT_READ|ESP_GATT_CHAR_PROP_BIT_WRITE_NR|ESP_GATT_CHAR_PROP_BIT_NOTIFY;
#endif

///SPP Service - data receive characteristic, read&write without response
static const uint16_t spp_data_receive_uuid = ESP_GATT_UUID_SPP_DATA_RECEIVE;
static const uint8_t  spp_data_receive_val[20] = {0x00};

///SPP Service - data notify characteristic, notify&read
static const uint16_t spp_data_notify_uuid = ESP_GATT_UUID_SPP_DATA_NOTIFY;
static const uint8_t  spp_data_notify_val[20] = {0x00};
static const uint8_t  spp_data_notify_ccc[2] = {0x00, 0x00};

///SPP Service - command characteristic, read&write without response
static const uint16_t spp_command_uuid = ESP_GATT_UUID_SPP_COMMAND_RECEIVE;
static const uint8_t  spp_command_val[10] = {0x00};

///SPP Service - status characteristic, notify&read
static const uint16_t spp_status_uuid = ESP_GATT_UUID_SPP_COMMAND_NOTIFY;
static const uint8_t  spp_status_val[10] = {0x00};
static const uint8_t  spp_status_ccc[2] = {0x00, 0x00};

#ifdef SUPPORT_HEARTBEAT
///SPP Server - Heart beat characteristic, notify&write&read
static const uint16_t spp_heart_beat_uuid = ESP_GATT_UUID_SPP_HEARTBEAT;
static const uint8_t  spp_heart_beat_val[2] = {0x00, 0x00};
static const uint8_t  spp_heart_beat_ccc[2] = {0x00, 0x00};
#endif

///Full HRS Database Description - Used to add attributes into the database
static const esp_gatts_attr_db_t spp_gatt_db[SPP_IDX_NB] =
{
    //SPP -  Service Declaration
    [SPP_IDX_SVC]                      	=
    {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&primary_service_uuid, ESP_GATT_PERM_READ,
    sizeof(spp_service_uuid), sizeof(spp_service_uuid), (uint8_t *)&spp_service_uuid}},

    //SPP -  data receive characteristic Declaration
    [SPP_IDX_SPP_DATA_RECV_CHAR]            =
    {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&character_declaration_uuid, ESP_GATT_PERM_READ,
    CHAR_DECLARATION_SIZE,CHAR_DECLARATION_SIZE, (uint8_t *)&char_prop_read_write}},

    //SPP -  data receive characteristic Value
    [SPP_IDX_SPP_DATA_RECV_VAL]             	=
    {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&spp_data_receive_uuid, ESP_GATT_PERM_READ|ESP_GATT_PERM_WRITE,
    SPP_DATA_MAX_LEN,sizeof(spp_data_receive_val), (uint8_t *)spp_data_receive_val}},

    //SPP -  data notify characteristic Declaration
    [SPP_IDX_SPP_DATA_NOTIFY_CHAR]  =
    {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&character_declaration_uuid, ESP_GATT_PERM_READ,
    CHAR_DECLARATION_SIZE,CHAR_DECLARATION_SIZE, (uint8_t *)&char_prop_read_notify}},

    //SPP -  data notify characteristic Value
    [SPP_IDX_SPP_DATA_NTY_VAL]   =
    {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&spp_data_notify_uuid, ESP_GATT_PERM_READ,
    SPP_DATA_MAX_LEN, sizeof(spp_data_notify_val), (uint8_t *)spp_data_notify_val}},

    //SPP -  data notify characteristic - Client Characteristic Configuration Descriptor
    [SPP_IDX_SPP_DATA_NTF_CFG]         =
    {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&character_client_config_uuid, ESP_GATT_PERM_READ|ESP_GATT_PERM_WRITE,
    sizeof(uint16_t),sizeof(spp_data_notify_ccc), (uint8_t *)spp_data_notify_ccc}},

    //SPP -  command characteristic Declaration
    [SPP_IDX_SPP_COMMAND_CHAR]            =
    {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&character_declaration_uuid, ESP_GATT_PERM_READ,
    CHAR_DECLARATION_SIZE,CHAR_DECLARATION_SIZE, (uint8_t *)&char_prop_read_write}},

    //SPP -  command characteristic Value
    [SPP_IDX_SPP_COMMAND_VAL]                 =
    {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&spp_command_uuid, ESP_GATT_PERM_READ|ESP_GATT_PERM_WRITE,
    SPP_CMD_MAX_LEN,sizeof(spp_command_val), (uint8_t *)spp_command_val}},

    //SPP -  status characteristic Declaration
    [SPP_IDX_SPP_STATUS_CHAR]            =
    {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&character_declaration_uuid, ESP_GATT_PERM_READ,
    CHAR_DECLARATION_SIZE,CHAR_DECLARATION_SIZE, (uint8_t *)&char_prop_read_notify}},

    //SPP -  status characteristic Value
    [SPP_IDX_SPP_STATUS_VAL]                 =
    {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&spp_status_uuid, ESP_GATT_PERM_READ,
    SPP_STATUS_MAX_LEN,sizeof(spp_status_val), (uint8_t *)spp_status_val}},

    //SPP -  status characteristic - Client Characteristic Configuration Descriptor
    [SPP_IDX_SPP_STATUS_CFG]         =
    {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&character_client_config_uuid, ESP_GATT_PERM_READ|ESP_GATT_PERM_WRITE,
    sizeof(uint16_t),sizeof(spp_status_ccc), (uint8_t *)spp_status_ccc}},

#ifdef SUPPORT_HEARTBEAT
    //SPP -  Heart beat characteristic Declaration
    [SPP_IDX_SPP_HEARTBEAT_CHAR]  =
    {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&character_declaration_uuid, ESP_GATT_PERM_READ,
    CHAR_DECLARATION_SIZE,CHAR_DECLARATION_SIZE, (uint8_t *)&char_prop_read_write_notify}},

    //SPP -  Heart beat characteristic Value
    [SPP_IDX_SPP_HEARTBEAT_VAL]   =
    {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&spp_heart_beat_uuid, ESP_GATT_PERM_READ|ESP_GATT_PERM_WRITE,
    sizeof(spp_heart_beat_val), sizeof(spp_heart_beat_val), (uint8_t *)spp_heart_beat_val}},

    //SPP -  Heart beat characteristic - Client Characteristic Configuration Descriptor
    [SPP_IDX_SPP_HEARTBEAT_CFG]         =
    {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&character_client_config_uuid, ESP_GATT_PERM_READ|ESP_GATT_PERM_WRITE,
    sizeof(uint16_t),sizeof(spp_data_notify_ccc), (uint8_t *)spp_heart_beat_ccc}},
#endif
};

static uint8_t find_char_and_desr_index(uint16_t handle)
{
    uint8_t error = 0xff;

    for(int i = 0; i < SPP_IDX_NB ; i++){
        if(handle == spp_handle_table[i]){
            return i;
        }
    }

    return error;
}

static bool store_wr_buffer(esp_ble_gatts_cb_param_t *p_data)
{
    temp_spp_recv_data_node_p1 = (spp_receive_data_node_t *)malloc(sizeof(spp_receive_data_node_t));

    if(temp_spp_recv_data_node_p1 == NULL){
        ESP_LOGI(GATTS_TABLE_TAG, "malloc error %s %d", __func__, __LINE__);
        return false;
    }
    if(temp_spp_recv_data_node_p2 != NULL){
        temp_spp_recv_data_node_p2->next_node = temp_spp_recv_data_node_p1;
    }
    temp_spp_recv_data_node_p1->len = p_data->write.len;
    SppRecvDataBuff.buff_size += p_data->write.len;
    temp_spp_recv_data_node_p1->next_node = NULL;
    temp_spp_recv_data_node_p1->node_buff = (uint8_t *)malloc(p_data->write.len);
    temp_spp_recv_data_node_p2 = temp_spp_recv_data_node_p1;
    if (temp_spp_recv_data_node_p1->node_buff == NULL) {
        ESP_LOGI(GATTS_TABLE_TAG, "malloc error %s %d\n", __func__, __LINE__);
        temp_spp_recv_data_node_p1->len = 0;
    } else {
        memcpy(temp_spp_recv_data_node_p1->node_buff,p_data->write.value,p_data->write.len);
    }

    if(SppRecvDataBuff.node_num == 0){
        SppRecvDataBuff.first_node = temp_spp_recv_data_node_p1;
        SppRecvDataBuff.node_num++;
    }else{
        SppRecvDataBuff.node_num++;
    }

    return true;
}

static void process_complete_data(void)
{
    if (SppRecvDataBuff.first_node == NULL || SppRecvDataBuff.buff_size == 0) {
        return;
    }

    // Allocate buffer for complete data
    char *complete_data = malloc(SppRecvDataBuff.buff_size + 1);
    if (complete_data == NULL) {
        ESP_LOGE(GATTS_TABLE_TAG, "Failed to allocate memory for complete data");
        free_write_buffer();
        return;
    }

    // Reassemble data from all chunks
    temp_spp_recv_data_node_p1 = SppRecvDataBuff.first_node;
    int offset = 0;
    
    while(temp_spp_recv_data_node_p1 != NULL){
        if (temp_spp_recv_data_node_p1->node_buff && temp_spp_recv_data_node_p1->len > 0) {
            memcpy(complete_data + offset, temp_spp_recv_data_node_p1->node_buff, temp_spp_recv_data_node_p1->len);
            offset += temp_spp_recv_data_node_p1->len;
        }
        temp_spp_recv_data_node_p1 = temp_spp_recv_data_node_p1->next_node;
    }
    
    complete_data[offset] = '\0'; // Null terminate
    
    ESP_LOGI(GATTS_TABLE_TAG, "Complete data received (%d bytes): %s", offset, complete_data);
    
    // Process the complete JSON data
    extern void parse_ble_data(const char* json_data);
    parse_ble_data(complete_data);
    
    // Clean up
    free(complete_data);
    free_write_buffer();
}

static void free_write_buffer(void)
{
    temp_spp_recv_data_node_p1 = SppRecvDataBuff.first_node;

    while(temp_spp_recv_data_node_p1 != NULL){
        temp_spp_recv_data_node_p2 = temp_spp_recv_data_node_p1->next_node;
        if (temp_spp_recv_data_node_p1->node_buff) {
            free(temp_spp_recv_data_node_p1->node_buff);
        }
        free(temp_spp_recv_data_node_p1);
        temp_spp_recv_data_node_p1 = temp_spp_recv_data_node_p2;
    }

    SppRecvDataBuff.node_num = 0;
    SppRecvDataBuff.buff_size = 0;
    SppRecvDataBuff.first_node = NULL;
}

//BURASSSSSSSSSSSSSSSSSIIIIIIIIIIIIIIIIIII  COOOOOOOOKKKK    ONEEEEEEEEEEMMMMLLLIIII
#ifdef SUPPORT_HEARTBEAT
void spp_heartbeat_task(void * arg)
{
    uint16_t cmd_id;

    for(;;) {
        vTaskDelay(50 / portTICK_PERIOD_MS);
        if(xQueueReceive(cmd_heartbeat_queue, &cmd_id, portMAX_DELAY)) {
            while(1){
                heartbeat_count_num++;
                vTaskDelay(5000/ portTICK_PERIOD_MS);
                if((heartbeat_count_num >3)&&(is_connected)){
                    esp_ble_gap_disconnect(spp_remote_bda);
                }
                if(is_connected && enable_heart_ntf){
                    esp_ble_gatts_send_indicate(spp_gatts_if, spp_conn_id, spp_handle_table[SPP_IDX_SPP_HEARTBEAT_VAL],sizeof(heartbeat_s), heartbeat_s, false);
                }else if(!is_connected){
                    break;
                }
            }
        }
    }
    vTaskDelete(NULL);
}
#endif


const char example_json_data_packet[] = "{\"temperature\": 25.0, \"humidity\": 50.0, \"pressure\": 1013.25}";

// Data transmission task to handle chunking
void data_transmit_task(void *arg)
{
    char *data_to_send = NULL;
    
    for(;;) {
        if(xQueueReceive(data_transmit_queue, &data_to_send, portMAX_DELAY)) {
            if (data_to_send == NULL || !ble_status_tx_ready()) {
                continue;
            }
            int data_len = (int)strlen(data_to_send);
            int max_payload = (int)spp_mtu_size - 3; /* GATT overhead */
            int offset = 0;

            if (max_payload < 20) {
                max_payload = 20;
            }

            while (offset < data_len && ble_status_tx_ready()) {
                int chunk = data_len - offset;
                if (chunk > max_payload) {
                    chunk = max_payload;
                }
                esp_err_t err = esp_ble_gatts_send_indicate(
                    spp_gatts_if, spp_conn_id, spp_handle_table[SPP_IDX_SPP_DATA_NTY_VAL],
                    chunk, (uint8_t *)data_to_send + offset, false);
                if (err != ESP_OK) {
                    ESP_LOGW(GATTS_TABLE_TAG, "indicate fail off=%d err=%s", offset,
                             esp_err_to_name(err));
                    break;
                }
                offset += chunk;
                if (offset < data_len) {
                    vTaskDelay(pdMS_TO_TICKS(15));
                }
            }
            /* pointer = converted_json_data2; free display_manager'da */
        }
    }
    vTaskDeleteWithCaps(NULL);
}
void bluetooth_send_periodic_callback(TimerHandle_t xTimer)
{
    (void)xTimer;
    if (!ble_status_tx_ready()) {
        return;
    }
    char *data_ptr = converted_json_data2;
    xQueueSend(data_transmit_queue, &data_ptr, 0);
}

static void gap_event_handler(esp_gap_ble_cb_event_t event, esp_ble_gap_cb_param_t *param)
{
    esp_err_t err;
    ESP_LOGI(GATTS_TABLE_TAG, "GAP_EVT, event %d", event);

    switch (event) {
    case ESP_GAP_BLE_ADV_DATA_RAW_SET_COMPLETE_EVT:
        s_adv_data_ready = true;
        if (s_adv_allowed) {
            esp_ble_gap_start_advertising(&spp_adv_params);
        } else {
            ESP_LOGW(GATTS_TABLE_TAG, "adv data ready — waiting for dash before advertise");
        }
        break;
    case ESP_GAP_BLE_ADV_START_COMPLETE_EVT:
        //advertising start complete event to indicate advertising start successfully or failed
        if((err = param->adv_start_cmpl.status) != ESP_BT_STATUS_SUCCESS) {
            ESP_LOGE(GATTS_TABLE_TAG, "Advertising start failed: %s", esp_err_to_name(err));
        } else {
            ESP_LOGI(GATTS_TABLE_TAG, "Advertising started successfully (internal free=%u)",
                     (unsigned)heap_caps_get_free_size(MALLOC_CAP_INTERNAL));
        }
        break;
    // Note: ESP_GAP_BLE_CONN_PARAMS_UPD_EVT and ESP_GAP_BLE_CONN_PARAMS_UPD_FAIL_EVT 
    // are no longer available in ESP-IDF v5.3. Connection parameter updates are now
    // handled automatically by the stack or through different mechanisms.
    default:
        break;
    }
}

static void gatts_profile_event_handler(esp_gatts_cb_event_t event, esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param)
{
    esp_ble_gatts_cb_param_t *p_data = (esp_ble_gatts_cb_param_t *) param;
    uint8_t res = 0xff;

    //ESP_LOGI(GATTS_TABLE_TAG, "event = %x",event);
    switch (event) {
    	case ESP_GATTS_REG_EVT:
    	    ESP_LOGI(GATTS_TABLE_TAG, "%s %d", __func__, __LINE__);
        	esp_ble_gap_set_device_name(SAMPLE_DEVICE_NAME);

        	ESP_LOGI(GATTS_TABLE_TAG, "%s %d", __func__, __LINE__);
        	esp_ble_gap_config_adv_data_raw((uint8_t *)spp_adv_data, sizeof(spp_adv_data));

        	ESP_LOGI(GATTS_TABLE_TAG, "%s %d", __func__, __LINE__);
        	esp_ble_gatts_create_attr_tab(spp_gatt_db, gatts_if, SPP_IDX_NB, SPP_SVC_INST_ID);
       	break;
    	case ESP_GATTS_READ_EVT:
            res = find_char_and_desr_index(p_data->read.handle);
            if(res == SPP_IDX_SPP_STATUS_VAL){
                //TODO:client read the status characteristic
            }
       	 break;
    	case ESP_GATTS_WRITE_EVT: {
    	    res = find_char_and_desr_index(p_data->write.handle);
            if(p_data->write.is_prep == false){
                //ESP_LOGI(GATTS_TABLE_TAG, "ESP_GATTS_WRITE_EVT : handle = %d", res);
                if(res == SPP_IDX_SPP_COMMAND_VAL){
                    // Use the buffer directly instead of malloc
                    memset(spp_cmd_buff, 0x0, sizeof(spp_cmd_buff));
                    memcpy(spp_cmd_buff, p_data->write.value, p_data->write.len);
                    ESP_LOGI(GATTS_TABLE_TAG, "Received Buffer Data: %s", (char *)spp_cmd_buff);
                    
                    // Process the JSON data for command messages
                    extern void parse_ble_data(const char* json_data);
                    parse_ble_data((const char*)spp_cmd_buff);


                }else if(res == SPP_IDX_SPP_DATA_NTF_CFG){
                    if((p_data->write.len == 2)&&(p_data->write.value[0] == 0x01)&&(p_data->write.value[1] == 0x00)){
                        enable_data_ntf = true;
                    }else if((p_data->write.len == 2)&&(p_data->write.value[0] == 0x00)&&(p_data->write.value[1] == 0x00)){
                        enable_data_ntf = false;
                        ESP_LOGI(GATTS_TABLE_TAG, "ESP_GATTS_WRITE_EVT : handle = ");
                    }
                }
#ifdef SUPPORT_HEARTBEAT
                else if(res == SPP_IDX_SPP_HEARTBEAT_CFG){
                    if((p_data->write.len == 2)&&(p_data->write.value[0] == 0x01)&&(p_data->write.value[1] == 0x00)){
                        enable_heart_ntf = true;
                    }else if((p_data->write.len == 2)&&(p_data->write.value[0] == 0x00)&&(p_data->write.value[1] == 0x00)){
                        enable_heart_ntf = false;
                    }
                }else if(res == SPP_IDX_SPP_HEARTBEAT_VAL){
                    if((p_data->write.len == sizeof(heartbeat_s))&&(memcmp(heartbeat_s,p_data->write.value,sizeof(heartbeat_s)) == 0)){
                        heartbeat_count_num = 0;
                    }
                }
#endif
                else if(res == SPP_IDX_SPP_DATA_RECV_VAL){
                    ESP_LOGI(GATTS_TABLE_TAG, "Single write data received (%d bytes): %.*s", p_data->write.len, p_data->write.len, (char *)p_data->write.value);
                    esp_log_buffer_char(GATTS_TABLE_TAG,(char *)(p_data->write.value),p_data->write.len);
                    
                    // Process the JSON data directly for single writes
                    extern void parse_ble_data(const char* json_data);
                    parse_ble_data((const char*)p_data->write.value);
                }else{
                    //TODO:
                }
            }else if((p_data->write.is_prep == true)&&(res == SPP_IDX_SPP_DATA_RECV_VAL)){
                
                ESP_LOGI(GATTS_TABLE_TAG, "ESP_GATTS_PREP_WRITE_EVT : handle = %d", res);
                store_wr_buffer(p_data);
            }
      	 	break;
    	}
    	case ESP_GATTS_EXEC_WRITE_EVT:{
    	    ESP_LOGI(GATTS_TABLE_TAG, "ESP_GATTS_EXEC_WRITE_EVT");
    	    if(p_data->exec_write.exec_write_flag){
    	        // Process the complete reassembled data
    	        process_complete_data();
    	    }
    	    break;
    	}
    	case ESP_GATTS_MTU_EVT:
    	    spp_mtu_size = p_data->mtu.mtu;
    	    mtu_negotiation_pending = false; // MTU negotiation completed
    	    ESP_LOGI(GATTS_TABLE_TAG, "MTU size updated to: %d", spp_mtu_size);
            /* MTU hazir + notify aciksa periyodik status baslat (connect aninda degil) */
            if (s_periodic_timer == NULL && is_connected) {
                s_periodic_timer = xTimerCreate("PeriodicTimer", pdMS_TO_TICKS(1000), pdTRUE,
                                                (void *)0, bluetooth_send_periodic_callback);
                if (s_periodic_timer && xTimerStart(s_periodic_timer, 0) != pdPASS) {
                    ESP_LOGE(GATTS_TABLE_TAG, "Failed to start status timer");
                }
            }
    	    break;
    	case ESP_GATTS_CONF_EVT:
    	    break;
    	case ESP_GATTS_UNREG_EVT:
        	break;
    	case ESP_GATTS_DELETE_EVT:
        	break;
    	case ESP_GATTS_START_EVT:
        	break;
    	case ESP_GATTS_STOP_EVT:
        	break;
    	case ESP_GATTS_CONNECT_EVT:
    	    spp_conn_id = p_data->connect.conn_id;
    	    spp_gatts_if = gatts_if;
    	    is_connected = true;

            ESP_LOGI(GATTS_TABLE_TAG, "Client connected, current MTU: %d", spp_mtu_size);

            // Initialize MTU negotiation state
            mtu_negotiation_pending = true;

            // Note: MTU exchange is initiated by the client, server responds to it
            // The local MTU is already set to 500 in ble_init()
            ESP_LOGI(GATTS_TABLE_TAG, "Waiting for client MTU exchange request...");

            // Set connection parameters for better stability
            esp_ble_conn_update_params_t conn_params = {0};
            memcpy(conn_params.bda, p_data->connect.remote_bda, sizeof(esp_bd_addr_t));
            conn_params.min_int = 0x10;    // 20ms
            conn_params.max_int = 0x20;    // 40ms  
            conn_params.latency = 0;       // No latency
            conn_params.timeout = 400;     // 4 seconds
            esp_ble_gap_update_conn_params(&conn_params);
            /* Status TX: MTU_EVT'te baslar — connect aninda MTU=23 flood IWDT yapiyordu */

    	    memcpy(&spp_remote_bda,&p_data->connect.remote_bda,sizeof(esp_bd_addr_t));
#ifdef SUPPORT_HEARTBEAT
    	    uint16_t cmd = 0;
            xQueueSend(cmd_heartbeat_queue,&cmd,10/portTICK_PERIOD_MS);
#endif
        	break;
    	case ESP_GATTS_DISCONNECT_EVT:
            ESP_LOGI(GATTS_TABLE_TAG, "Client disconnected, reason: %d", p_data->disconnect.reason);
            spp_mtu_size = 23;
            mtu_negotiation_pending = false; // Reset MTU negotiation state
    	    is_connected = false;
    	    enable_data_ntf = false;
            if (s_periodic_timer) {
                xTimerStop(s_periodic_timer, 0);
                xTimerDelete(s_periodic_timer, 0);
                s_periodic_timer = NULL;
            }
#ifdef SUPPORT_HEARTBEAT
    	    enable_heart_ntf = false;
    	    heartbeat_count_num = 0;
#endif
    	    if (s_adv_allowed) {
    	        esp_ble_gap_start_advertising(&spp_adv_params);
    	    }
    	    break;
    	case ESP_GATTS_OPEN_EVT:
    	    break;
    	case ESP_GATTS_CANCEL_OPEN_EVT:
    	    break;
    	case ESP_GATTS_CLOSE_EVT:
    	    break;
    	case ESP_GATTS_LISTEN_EVT:
    	    break;
    	case ESP_GATTS_CONGEST_EVT:
    	    break;
    	case ESP_GATTS_CREAT_ATTR_TAB_EVT:{
    	    ESP_LOGI(GATTS_TABLE_TAG, "The number handle =%x",param->add_attr_tab.num_handle);
    	    if (param->add_attr_tab.status != ESP_GATT_OK){
    	        ESP_LOGE(GATTS_TABLE_TAG, "Create attribute table failed, error code=0x%x", param->add_attr_tab.status);
    	    }
    	    else if (param->add_attr_tab.num_handle != SPP_IDX_NB){
    	        ESP_LOGE(GATTS_TABLE_TAG, "Create attribute table abnormally, num_handle (%d) doesn't equal to HRS_IDX_NB(%d)", param->add_attr_tab.num_handle, SPP_IDX_NB);
    	    }
    	    else {
    	        memcpy(spp_handle_table, param->add_attr_tab.handles, sizeof(spp_handle_table));
    	        esp_ble_gatts_start_service(spp_handle_table[SPP_IDX_SVC]);
    	    }
    	    break;
    	}
    	default:
    	    break;
    }
}


static void gatts_event_handler(esp_gatts_cb_event_t event, esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param)
{
    /* If event is register event, store the gatts_if for each profile */
    if (event == ESP_GATTS_REG_EVT) {
        if (param->reg.status == ESP_GATT_OK) {
            spp_profile_tab[SPP_PROFILE_APP_IDX].gatts_if = gatts_if; 
        } else {
            ESP_LOGI(GATTS_TABLE_TAG, "Reg app failed, app_id %04x, status %d",param->reg.app_id, param->reg.status);
            return;
        }
    }

    do {
        int idx;
        for (idx = 0; idx < SPP_PROFILE_NUM; idx++) {
            if (gatts_if == ESP_GATT_IF_NONE || /* ESP_GATT_IF_NONE, not specify a certain gatt_if, need to call every profile cb function */
                    gatts_if == spp_profile_tab[idx].gatts_if) {
                if (spp_profile_tab[idx].gatts_cb) {
                    spp_profile_tab[idx].gatts_cb(event, gatts_if, param);
                }
            }
        }
    } while (0);
}



void ble_init(void)
{
    esp_err_t ret;
    esp_bt_controller_config_t bt_cfg = BT_CONTROLLER_INIT_CONFIG_DEFAULT();



    // Initialize NVS
    ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK( ret );

    ESP_ERROR_CHECK(esp_bt_controller_mem_release(ESP_BT_MODE_CLASSIC_BT));

    ret = esp_bt_controller_init(&bt_cfg);
    if (ret) {
        ESP_LOGE(GATTS_TABLE_TAG, "%s enable controller failed: %s", __func__, esp_err_to_name(ret));
        return;
    }

    ret = esp_bt_controller_enable(ESP_BT_MODE_BLE);
    if (ret) {
        ESP_LOGE(GATTS_TABLE_TAG, "%s enable controller failed: %s", __func__, esp_err_to_name(ret));
        return;
    }

    ESP_LOGI(GATTS_TABLE_TAG, "%s init bluetooth", __func__);

    ret = esp_bluedroid_init();
    if (ret) {
        ESP_LOGE(GATTS_TABLE_TAG, "%s init bluetooth failed: %s", __func__, esp_err_to_name(ret));
        return;
    }
    ret = esp_bluedroid_enable();
    if (ret) {
        ESP_LOGE(GATTS_TABLE_TAG, "%s enable bluetooth failed: %s", __func__, esp_err_to_name(ret));
        return;
    }

    esp_ble_gatts_register_callback(gatts_event_handler);
    esp_ble_gap_register_callback(gap_event_handler);
    esp_ble_gatts_app_register(ESP_SPP_APP_ID);

    // Create queues
    cmd_cmd_queue = xQueueCreate(10, sizeof(uint16_t));
    data_transmit_queue = xQueueCreate(5, sizeof(char*));
    
    if (cmd_cmd_queue == NULL || data_transmit_queue == NULL) {
        ESP_LOGE(GATTS_TABLE_TAG, "Failed to create queues");
        return;
    }

    /* Stack SPIRAM — internal DRAM BLE controller / connect icin kalsin */
    xTaskCreateWithCaps(data_transmit_task, "data_transmit", 3072, NULL, 5, NULL,
                        MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);

    esp_err_t local_mtu_ret = esp_ble_gatt_set_local_mtu(500);
    if (local_mtu_ret){
        ESP_LOGE(GATTS_TABLE_TAG, "set local  MTU failed, error code = %x", local_mtu_ret);
    }

    ESP_LOGI(GATTS_TABLE_TAG, "ble_init done internal free=%u (advertise deferred)",
             (unsigned)heap_caps_get_free_size(MALLOC_CAP_INTERNAL));
    return;
}

void ble_start_advertising(void)
{
    s_adv_allowed = true;
    ESP_LOGI(GATTS_TABLE_TAG, "ble_start_advertising allow internal free=%u ready=%d",
             (unsigned)heap_caps_get_free_size(MALLOC_CAP_INTERNAL), (int)s_adv_data_ready);
    if (s_adv_data_ready) {
        esp_err_t err = esp_ble_gap_start_advertising(&spp_adv_params);
        if (err != ESP_OK) {
            ESP_LOGE(GATTS_TABLE_TAG, "start advertising failed: %s", esp_err_to_name(err));
        }
    }
}

// Function to get the connection status
bool get_connection_status() {
    return is_connected;
}


// Function to set the global variable converted_json_data2
void set_converted_json_data(char* converted_json_data) {
    if (converted_json_data2 != NULL) {
        free(converted_json_data2);
    }
    converted_json_data2 = converted_json_data;
    //ESP_LOGI(GATTS_TABLE_TAG, "Converted JSON Data: %s", converted_json_data2);
}

// Function to get the spp_cmd_buff
uint8_t* get_spp_cmd_buff() {
    return spp_cmd_buff;
}

// Function to reset the spp_cmd_buff
void reset_spp_cmd_buff() {
    memset(spp_cmd_buff, 0x0, sizeof(spp_cmd_buff));
}