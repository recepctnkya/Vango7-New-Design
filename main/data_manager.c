
// data_manager.c


#include <stdio.h>
#include "string.h"
#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "esp_timer.h"
#include "esp_lcd_panel_ops.h"
#include "esp_lcd_panel_rgb.h"
#include "driver/gpio.h"
#include "esp_err.h"
#include "esp_log.h"
#include "lvgl.h"
#include <nvs_flash.h>
#include "data_manager.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/uart.h"
#include "display_manager.h"
#include "modbus_params.h"
#include "mbcontroller.h"
#include "display_manager.h"


#define MAX_INPUTS 20
#define MAX_OUTPUTS 50
#define MAX_ANALOG_INPUTS 20

typedef struct {
    uint16_t total_inputs[MAX_INPUTS];           // Total digital inputs
    uint16_t total_outputs[MAX_OUTPUTS];         // Total digital outputs
    uint16_t total_analog_inputs[MAX_ANALOG_INPUTS]; // Total analog inputs
    uint16_t R;                                   // Red value for RGB
    uint16_t G;                                   // Green value for RGB
    uint16_t B;                                   // Blue value for RGB
    uint16_t voltage;                             // Total voltage
    uint16_t current;                             // Total current
    uint8_t status;                               // Status information
    uint8_t CID;                                  // Control ID
} total_data_t;



#define MAX_SLAVES 89  // Total number of possible slave IDs
#define MAX_REGS 70 // Maximum registers for any slave type (24 for PlusThree)
#define MAX_ANALOG_POINTS 5

// Structure to hold two-point calibration data
typedef struct {
    float point1_value;  // Measured value at calibration point 1
    float point1_output; // Expected output at calibration point 1
    float point2_value;  // Measured value at calibration point 2
    float point2_output; // Expected output at calibration point 2
} analog_calibration_t;

typedef struct {
    uint8_t slave_id;               // Slave ID for the device
    uint8_t num_outputs;            // Number of digital outputs
    uint8_t num_inputs;             // Number of digital inputs
    uint8_t num_analog_inputs;      // Number of analog inputs
    uint8_t num_dimmable_outputs;   // Number of dimmable outputs
    uint8_t has_rgb_output;         // Boolean flag to indicate if RGB output is available
    analog_calibration_t analog_inputs[MAX_ANALOG_POINTS]; // Calibration data for up to 5 analog inputs
    uint16_t regs_data[MAX_REGS];   // Buffer to store register data for the slave
    uint16_t num_regs;              // Number of registers to store
} modbus_slave_t;

// Array to store information for each slave
modbus_slave_t slaves[MAX_SLAVES];


#define MB_PORT_NUM     (2)   // Number of UART port used for Modbus connection
#define MB_DEV_SPEED    (115200)  // The communication speed of the UART


#define MASTER_MAX_CIDS num_device_parameters

// Number of reading of parameters from slave
#define MASTER_MAX_RETRY 30

// Timeout to update cid over Modbus
#define UPDATE_CIDS_TIMEOUT_MS          (500)
#define UPDATE_CIDS_TIMEOUT_TICS        (UPDATE_CIDS_TIMEOUT_MS / portTICK_PERIOD_MS)

// Timeout between polls
#define POLL_TIMEOUT_MS                 (1)
#define POLL_TIMEOUT_TICS               (POLL_TIMEOUT_MS / portTICK_PERIOD_MS)


// The macro to get offset for parameter in the appropriate structure
#define HOLD_OFFSET(field) ((uint16_t)(offsetof(holding_reg_params_t, field) + 1))
#define INPUT_OFFSET(field) ((uint16_t)(offsetof(input_reg_params_t, field) + 1))
#define COIL_OFFSET(field) ((uint16_t)(offsetof(coil_reg_params_t, field) + 1))
// Discrete offset macro
#define DISCR_OFFSET(field) ((uint16_t)(offsetof(discrete_reg_params_t, field) + 1))

#define STR(fieldname) ((const char*)( fieldname ))
// Options can be used as bit masks or parameter limits
#define OPTS(min_val, max_val, step_val) { .opt1 = min_val, .opt2 = max_val, .opt3 = step_val }

static const char *TAG = "DATA_MANAGER";
uint8_t numOfSlaves = 0;
void print_total_data(const total_data_t* data);
void parse_slaves(modbus_slave_t* slaves, uint8_t num_slaves, total_data_t* result);

// Enumeration of modbus device addresses accessed by master device
enum {
    MB_DEVICE_ADDR1 = 1 // Only one slave device used for the test (add other slave addresses here)
};

// Enumeration of all supported CIDs for device (used in parameter definition table)
enum {
    CID_INP_DATA_0 = 0,
    CID_HOLD_DATA_0,
    CID_INP_DATA_1,
    CID_HOLD_DATA_1,
    CID_INP_DATA_2,
    CID_HOLD_DATA_2,
    CID_HOLD_TEST_REG,
    CID_RELAY_P1,
    CID_RELAY_P2,
    CID_DISCR_P1,
    CID_COUNT
};


const mb_parameter_descriptor_t device_parameters[] = {
    // { CID, Param Name, Units, Modbus Slave Addr, Modbus Reg Type, Reg Start, Reg Size, Instance Offset, Data Type, Data Size, Parameter Options, Access Mode}
    { CID_INP_DATA_0, STR("Data_channel_0"), STR("Volts"), MB_DEVICE_ADDR1, MB_PARAM_INPUT, 0, 2, INPUT_OFFSET(input_data0), PARAM_TYPE_U16, 4, OPTS( -10, 10, 1 ), PAR_PERMS_READ_WRITE_TRIGGER },
    { CID_HOLD_DATA_0, STR("Humidity_1"), STR("%rH"), MB_DEVICE_ADDR1, MB_PARAM_HOLDING, 0, 2, HOLD_OFFSET(holding_data0), PARAM_TYPE_U16, 4, OPTS( 0, 100, 1 ), PAR_PERMS_READ_WRITE_TRIGGER }
};

// Calculate number of parameters in the table
const uint16_t num_device_parameters = (sizeof(device_parameters)/sizeof(device_parameters[0]));




// Maximum number of devices in each range
#define MAX_MINIONE    9
#define MAX_MINITWO    19
#define MAX_MINITHREE  29
#define MAX_MIDIONE    39
#define MAX_MIDITWO    49
#define MAX_MIDITHREE  59
#define MAX_PLUSONE    69
#define MAX_PLUSTWO    79
#define MAX_PLUSTHREE  89




#define PARAM_LENGTH 16  // Adjust as needed


// Slave ID
#define SLAVE_ID 50

// General CID
#define CID_MIDITHREE 0

// Voltage and Current
#define VOLTAGE_INDIS 1
#define CURRENT_INDIS 2

// Control Registers
#define RESET_INDIS 3
#define BLUETOOTH_INDIS 4

// Outputs
#define OUTPUT_1_INDIS 5
#define OUTPUT_2_INDIS 6
#define OUTPUT_3_INDIS 7
#define OUTPUT_4_INDIS 8
#define OUTPUT_5_INDIS 9
#define OUTPUT_6_INDIS 10
#define OUTPUT_7_INDIS 11
#define OUTPUT_8_INDIS 12
#define OUTPUT_9_INDIS 13
#define OUTPUT_10_INDIS 14
#define OUTPUT_11_INDIS 15
#define OUTPUT_12_INDIS 16
#define OUTPUT_13_INDIS 17
#define OUTPUT_14_INDIS 18
#define OUTPUT_15_INDIS 19
#define OUTPUT_16_INDIS 20

// Inputs
#define INPUT_1_INDIS 21
#define INPUT_2_INDIS 22
#define INPUT_3_INDIS 23
#define INPUT_4_INDIS 24
#define INPUT_5_INDIS 25
#define INPUT_6_INDIS 26
#define INPUT_7_INDIS 27
#define INPUT_8_INDIS 28
#define INPUT_9_INDIS 29
#define INPUT_10_INDIS 30
#define INPUT_11_INDIS 31
#define INPUT_12_INDIS 32
#define INPUT_13_INDIS 33
#define INPUT_14_INDIS 34
#define INPUT_15_INDIS 35
#define INPUT_16_INDIS 36

// Analog Inputs
#define ANALOG_INPUT_1_INDIS 37
#define ANALOG_INPUT_2_INDIS 38
#define ANALOG_INPUT_3_INDIS 39
#define ANALOG_INPUT_4_INDIS 40

// Dimmable Outputs
#define DIMMABLE_OUTPUT_1_INDIS 41

// RGB Outputs
#define RGB_R_INDIS 42
#define RGB_G_INDIS 43
#define RGB_B_INDIS 44



// Modbus master initialization
static esp_err_t master_init(void)
{
    // Initialize and start Modbus controller
    mb_communication_info_t comm = {
            .port = MB_PORT_NUM,
#if CONFIG_MB_COMM_MODE_ASCII
            .mode = MB_MODE_ASCII,
#elif CONFIG_MB_COMM_MODE_RTU
            .mode = MB_MODE_RTU,
#endif
            .baudrate = MB_DEV_SPEED,
            .parity = MB_PARITY_NONE
    };
    void* master_handler = NULL;

    esp_err_t err = mbc_master_init(MB_PORT_SERIAL_MASTER, &master_handler);
    MB_RETURN_ON_FALSE((master_handler != NULL), ESP_ERR_INVALID_STATE, TAG,
                                "mb controller initialization fail.");
    MB_RETURN_ON_FALSE((err == ESP_OK), ESP_ERR_INVALID_STATE, TAG,
                            "mb controller initialization fail, returns(0x%x).", (int)err);
    err = mbc_master_setup((void*)&comm);
    MB_RETURN_ON_FALSE((err == ESP_OK), ESP_ERR_INVALID_STATE, TAG,
                            "mb controller setup fail, returns(0x%x).", (int)err);

    // Set UART pin numbers
    err = uart_set_pin(MB_PORT_NUM, 16, 15,
                              11, UART_PIN_NO_CHANGE);
    MB_RETURN_ON_FALSE((err == ESP_OK), ESP_ERR_INVALID_STATE, TAG,
        "mb serial set pin failure, uart_set_pin() returned (0x%x).", (int)err);

    err = mbc_master_start();
    MB_RETURN_ON_FALSE((err == ESP_OK), ESP_ERR_INVALID_STATE, TAG,
                            "mb controller start fail, returned (0x%x).", (int)err);

    // Set driver mode to Half Duplex
    err = uart_set_mode(MB_PORT_NUM, UART_MODE_RS485_HALF_DUPLEX);
    MB_RETURN_ON_FALSE((err == ESP_OK), ESP_ERR_INVALID_STATE, TAG,
            "mb serial set mode failure, uart_set_mode() returned (0x%x).", (int)err);

    vTaskDelay(5);
    err = mbc_master_set_descriptor(&device_parameters[0], num_device_parameters);
    MB_RETURN_ON_FALSE((err == ESP_OK), ESP_ERR_INVALID_STATE, TAG,
                                "mb controller set descriptor fail, returns(0x%x).", (int)err);
    ESP_LOGI(TAG, "Modbus master stack initialized...");
    return err;
}





// Function to initialize Modbus slave based on device type
void init_modbus_slave(modbus_slave_t* slave, uint8_t slave_id) {
    slave->slave_id = slave_id;

    if (slave_id >= 1 && slave_id <= 9) { // MiniOne
        slave->num_outputs = 8;
        slave->num_inputs = 0;
        slave->num_analog_inputs = 0;
        slave->num_dimmable_outputs = 0;
        slave->has_rgb_output = 0;
        slave->num_regs = 15;
    } else if (slave_id >= 10 && slave_id <= 19) { // MiniTwo
        slave->num_outputs = 16;
        slave->num_inputs = 0;
        slave->num_analog_inputs = 0;
        slave->num_dimmable_outputs = 0;
        slave->has_rgb_output = 0;
        slave->num_regs = 25;
    } else if (slave_id >= 20 && slave_id <= 29) { // MiniThree
        slave->num_outputs = 24;
        slave->num_inputs = 0;
        slave->num_analog_inputs = 0;
        slave->num_dimmable_outputs = 0;
        slave->has_rgb_output = 0;
        slave->num_regs = 30;
    } else if (slave_id >= 30 && slave_id <= 39) { // MidiOne
        slave->num_outputs = 8;
        slave->num_inputs = 8;
        slave->num_analog_inputs = 0;
        slave->num_dimmable_outputs = 0;
        slave->has_rgb_output = 0;
        slave->num_regs = 25;
    } else if (slave_id >= 40 && slave_id <= 49) { // MidiTwo
        slave->num_outputs = 16;
        slave->num_inputs = 8;
        slave->num_analog_inputs = 0;
        slave->num_dimmable_outputs = 0;
        slave->has_rgb_output = 0;
        slave->num_regs = 40;
    } else if (slave_id >= 50 && slave_id <= 59) { // MidiThree
        slave->num_outputs = 24;
        slave->num_inputs = 8;
        slave->num_analog_inputs = 0;
        slave->num_dimmable_outputs = 0;
        slave->has_rgb_output = 0;
        slave->num_regs = 50;
    } else if (slave_id >= 60 && slave_id <= 69) { // PlusOne
        slave->num_outputs = 8;
        slave->num_inputs = 8;
        slave->num_analog_inputs = 5;
        slave->num_dimmable_outputs = 5;
        slave->has_rgb_output = 1;
        slave->num_regs = 30;
    } else if (slave_id >= 70 && slave_id <= 79) { // PlusTwo
        slave->num_outputs = 16;
        slave->num_inputs = 8;
        slave->num_analog_inputs = 5;
        slave->num_dimmable_outputs = 5;
        slave->has_rgb_output = 1;
        slave->num_regs = 50;
    } else if (slave_id >= 80 && slave_id <= 89) { // PlusThree
        slave->num_outputs = 24;
        slave->num_inputs = 8;
        slave->num_analog_inputs = 5;
        slave->num_dimmable_outputs = 5;
        slave->has_rgb_output = 1;
        slave->num_regs = 60;
    }
}




// Example function to write holding registers
esp_err_t write_holding_registers(uint16_t start_address, uint16_t* data, size_t num_regs) {
    mb_param_request_t request = {
        .slave_addr = 50,         // Set Modbus slave address
        .command = 0x10,                  // Command to write multiple holding registers (0x10)
        .reg_start = start_address,       // Start address of the registers
        .reg_size = num_regs              // Number of registers to write
    };

    esp_err_t err = mbc_master_send_request(&request, (void*)data);  // Send the request with the data
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to write holding registers: %s", esp_err_to_name(err));
    }
    return err;
}

// Function to toggle the Modbus register value
void toggle_regs_data(uint16_t index) {
    uint16_t* regs_data = getSlavesRegsData();
    if (regs_data != NULL) {
        uint16_t current_value = regs_data[index];
        uint16_t new_value = !current_value;
        esp_err_t err = write_holding_registers(index, &new_value, 1);
        if (err == ESP_OK) {
            ESP_LOGI(TAG, "Successfully toggled Modbus register %d to %d", index, new_value);
            regs_data[index] = new_value; // Update the regs_data with the new value
        } else {
            ESP_LOGE(TAG, "Failed to write to Modbus register %d", index);
        }
    } else {
        ESP_LOGE(TAG, "Failed to get slave registers data");
    }
}


total_data_t aggregated_data;
uint16_t reg_data[MAX_REGS];  // Buffer to store register data; MAX_REGS should be the max possible registers

// Function to communicate with connected Modbus slaves and read their registers
void comm_slaves() {
    esp_err_t error;
    mb_param_request_t request;

    // Iterate over each slave and read their registers based on slave->num_regs
    for (int i = 0; i < MAX_SLAVES; i++) {
        modbus_slave_t* slave = &slaves[i];  // Pointer to the current slave structure

        // Skip if slave is not initialized
        if (slave->slave_id == 0) {
            continue;
        }

        // Initialize the Modbus request for the current slave
        request.slave_addr = slave->slave_id;
        request.command = 0x03;  // Command to read holding registers
        request.reg_start = 0;   // Starting register address
        request.reg_size = slave->num_regs;  // Number of registers to read based on num_regs

        // Send the Modbus request and store the data in reg_data
        error = mbc_master_send_request(&request, reg_data);

        // Check if the request was successful
        if (error == ESP_OK) {
            // Copy read data into the slave's regs_data buffer
            for (int j = 0; j < slave->num_regs; j++) {
                slave->regs_data[j] = reg_data[j];
            }

            char log_buffer[256];
            int offset = snprintf(log_buffer, sizeof(log_buffer), "Slave ID %d - Registers: ", slave->slave_id);

            // Append each register value to log_buffer
            for (int j = 0; j < slave->num_regs && offset < sizeof(log_buffer); j++) {
                offset += snprintf(log_buffer + offset, sizeof(log_buffer) - offset, "%d ", slave->regs_data[j]);
            }

            ESP_LOGI(TAG, "%s", log_buffer);

            parse_slaves(slaves, numOfSlaves, &aggregated_data);
        } else {
            ESP_LOGW(TAG, "Slave ID %d: Failed to read registers", slave->slave_id);
        }
    }
}


// Function to check Modbus connection status based on error parameter
int get_modbus_connection_status() {
    esp_err_t error;
    mb_param_request_t request;
    static int no_response_count = 0;

    // Iterate over each slave and check their connection status

    modbus_slave_t* slave = &slaves[0];  // Pointer to the current slave structure

    // Initialize the Modbus request for the current slave
    request.slave_addr = 50;
    request.command = 0x03;  // Command to read holding registers
    request.reg_start = 0;   // Starting register address
    request.reg_size = 1;    // Check only one register to verify connection
    
    if(no_response_count > 5) {
        // Send the Modbus request
        error = mbc_master_send_request(&request, reg_data);
        ESP_LOGW(TAG, "no_response_count variable Resetted. Checking Slave Connection after 5 Seconds now!");
        // Check if the request was successful
        if (error != ESP_OK) {
            ESP_LOGW(TAG, "Slave ID %d: Failed to connect", slave->slave_id);
            return 0;
        }
        no_response_count = 0;
        return 1;
    }
    no_response_count++;
    // Return false if there was no response after 3 attempts
    return 1;
}


// Task to communicate with Modbus slaves and read their registers
void comm_slaves_task(void *pvParameters) {
    static int check_connection_on_bus = 0;
    const TickType_t delay = pdMS_TO_TICKS(1000); // Delay of 1 second between each call to comm_slaves
    vTaskDelay(200);
    while (true) {
        comm_slaves();
        // Delay before next communication cycle
        vTaskDelay(delay);
        check_connection_on_bus++;
        if(check_connection_on_bus > 18) {
            ESP_LOGW(TAG, "18 Seconds timeout has expired. All slaves on bus will be checked again now...");
            check_connected_devices();
            check_connection_on_bus = 0;
        }
    }
}


// Function to check if a Modbus slave is responding (over Modbus)
bool check_slave_present(uint8_t slave_id) {

    esp_err_t error;
    uint16_t reg_data[2];   // Buffer to store the read register data
    mb_param_request_t request;

    // Initialize the Modbus master request structure
    request.slave_addr = slave_id;
    request.command = 0x03;        // Command to read holding registers (0x03)
    request.reg_start = 0;
    request.reg_size = 1;
    // Send the Modbus request to read registers
    error = mbc_master_send_request(&request, reg_data);
    // Check for errors in sending the request
    if (error != ESP_OK) {
        return false;
    }
    return true;
}


// Function to add connected slave to the buffer
void add_slave_to_buffer(uint8_t slave_id) {
    // Check if the slave ID already exists in the buffer
    for (int i = 0; i < numOfSlaves; i++) {
        if (slaves[i].slave_id == slave_id) {
            ESP_LOGI(TAG, "Slave ID %d already exists in the buffer", slave_id);
            return; // Do not add the slave if the ID already exists
        }
    }

    // Add the new slave to the buffer
    modbus_slave_t* slave = &slaves[numOfSlaves]; // Adjust index for 0-based array
    numOfSlaves++;
    init_modbus_slave(slave, slave_id);
    ESP_LOGI(TAG, "Number of Connected Slaves: %d", numOfSlaves);
}


// Function to check connected devices and add them to the buffer
void check_connected_devices() {
    uint8_t current_id = 1;

    ESP_LOGI(TAG, "Checking for connected devices...\n");

    while (current_id <= MAX_SLAVES) {
        if (check_slave_present(current_id)) {
            add_slave_to_buffer(current_id);

            // Determine the device type based on ID ranges and print the connected device
            if (current_id >= 1 && current_id <= 9) {
                ESP_LOGI(TAG, "MiniOne with ID %d is connected\n", current_id);
            } else if (current_id >= 10 && current_id <= 19) {
                ESP_LOGI(TAG, "MiniTwo with ID %d is connected\n", current_id);
            } else if (current_id >= 20 && current_id <= 29) {
                ESP_LOGI(TAG, "MiniThree with ID %d is connected\n", current_id);
            } else if (current_id >= 30 && current_id <= 39) {
                ESP_LOGI(TAG, "MidiOne with ID %d is connected\n", current_id);
            } else if (current_id >= 40 && current_id <= 49) {
                ESP_LOGI(TAG, "MidiTwo with ID %d is connected\n", current_id);
            } else if (current_id >= 50 && current_id <= 59) {
                ESP_LOGI(TAG, "MidiThree with ID %d is connected\n", current_id);
            } else if (current_id >= 60 && current_id <= 69) {
                ESP_LOGI(TAG, "PlusOne with ID %d is connected\n", current_id);
            } else if (current_id >= 70 && current_id <= 79) {
                ESP_LOGI(TAG, "PlusTwo with ID %d is connected\n", current_id);
            } else if (current_id >= 80 && current_id <= 89) {
                ESP_LOGI(TAG, "PlusThree with ID %d is connected\n", current_id);
            }
        }

        // Increment the ID or skip to the next range based on device type
        if (current_id >= 1 && current_id <= 9) {
            current_id = 10;
        } else if (current_id >= 10 && current_id <= 19) {
            current_id = 20;
        } else if (current_id >= 20 && current_id <= 29) {
            current_id = 30;
        } else if (current_id >= 30 && current_id <= 39) {
            current_id = 40;
        } else if (current_id >= 40 && current_id <= 49) {
            current_id = 50;
        } else if (current_id >= 50 && current_id <= 59) {
            current_id = 60;
        } else if (current_id >= 60 && current_id <= 69) {
            current_id = 70;
        } else if (current_id >= 70 && current_id <= 79) {
            current_id = 80;
        } else if (current_id >= 80 && current_id <= 89) {
            break;
        }
    }
}


void parse_slaves(modbus_slave_t* slaves, uint8_t num_slaves, total_data_t* result) {
    // Initialize totals
    memset(result->total_inputs, 0, sizeof(uint16_t) * MAX_INPUTS);
    memset(result->total_outputs, 0, sizeof(uint16_t) * MAX_OUTPUTS);
    memset(result->total_analog_inputs, 0, sizeof(uint16_t) * MAX_ANALOG_INPUTS);
    
    // Initialize RGB, voltage, current, status, and CID
    result->R = result->G = result->B = 0;
    result->voltage = 0;
    result->current = 0;
    result->status = 0;
    result->CID = 0;

    for (uint8_t i = 0; i < num_slaves; i++) {
        modbus_slave_t* slave = &slaves[i];

        result->total_outputs[0] += slave->num_outputs; // Assuming outputs are in regs_data
        result->total_inputs[0] += slave->num_inputs; // Assuming inputs follow outputs
        result->total_analog_inputs[0] += slave->num_analog_inputs; // Adjust index accordingly


        // Aggregate RGB outputs if available
        if (slave->has_rgb_output) {
            result->R += slave->regs_data[slave->num_outputs + slave->num_inputs + slave->num_analog_inputs]; // Assuming R is stored
            result->G += slave->regs_data[slave->num_outputs + slave->num_inputs + slave->num_analog_inputs + 1]; // Assuming G is next
            result->B += slave->regs_data[slave->num_outputs + slave->num_inputs + slave->num_analog_inputs + 2]; // Assuming B is next
        }

        // Aggregate voltage and current (if they are in a known location in regs_data)
        result->voltage += slave->regs_data[slave->num_outputs + slave->num_inputs + slave->num_analog_inputs + 3]; // Assuming voltage
        result->current += slave->regs_data[slave->num_outputs + slave->num_inputs + slave->num_analog_inputs + 4]; // Assuming current

        // Update status and CID based on logic (this may need to be defined based on your application)
        result->status |= slave->regs_data[slave->num_outputs + slave->num_inputs + slave->num_analog_inputs + 5]; // Assuming status
        result->CID |= slave->regs_data[slave->num_outputs + slave->num_inputs + slave->num_analog_inputs + 6]; // Assuming CID
    }
}




void print_total_data(const total_data_t* data) {
    // Print total digital inputs
    ESP_LOGI(TAG, "Total Digital Inputs: %d", data->total_inputs[0]);

    // Print total digital outputs
    ESP_LOGI(TAG, "Total Digital Outputs: %d", data->total_outputs[0]);

    // Print total analog inputs
    ESP_LOGI(TAG, "Total Analog Inputs: %d", data->total_analog_inputs[0]);


    // Print RGB values
    ESP_LOGI(TAG, "RGB Values:");
    ESP_LOGI(TAG, "R: %d", data->R);
    ESP_LOGI(TAG, "G: %d", data->G);
    ESP_LOGI(TAG, "B: %d", data->B);

    // Print voltage and current
    ESP_LOGI(TAG, "Voltage: %d", data->voltage);
    ESP_LOGI(TAG, "Current: %d", data->current);
    
    // Print status and CID
    ESP_LOGI(TAG, "Status: %d", data->status);
    ESP_LOGI(TAG, "CID: %d", data->CID);
}



// Function to send data to Bluetooth
void send_to_bluetooth(const uint8_t *data, int length) {
    // Bluetooth ile veri gönderme işlemleri
    // Örneğin, Bluetooth API'si ile veri gönderin
}

// Function to send data to Wi-Fi
void send_to_wifi(const uint8_t *data, int length) {
    // Wi-Fi ile veri gönderme işlemleri
    // Örneğin, Wi-Fi API'si ile veri gönderin
}

// Function to update display
void display_update(const uint8_t *data, int length) {
    // Ekran güncellemelerini yapın
    update_display_with_data(data, length); // display_manager'daki fonksiyonu çağırın
}

// Task for reading serial data
void serial_read_task(void *pvParameters) {
}


void data_manager_init() {
    ESP_ERROR_CHECK(master_init());
    check_connected_devices();
    vTaskDelay(100);
    xTaskCreate(comm_slaves_task, "Comm_Slaves_Task", 4096, NULL, 5, NULL);
}



uint16_t* getSlavesRegsData() {
    return slaves[0].regs_data;
}

