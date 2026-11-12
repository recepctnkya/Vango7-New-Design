// display_manager.c

#include "display_manager.h"
#include "vango_shot.h"
#include "ui_vango_dash.h"
#include "ui_vango_i18n.h"
#include "ui_vango_theme.h"
#include "data_manager.h"
#include "definitions.h"
#include "ui_files/ui.h"
#include "hexnet_bluetooth.h"
#include "hexnet_nvs.h"
#include "hexnet_canbus.h"
#include "hexnet_io_profile.h"
#include "hexnet_io_labels.h"
#include "panel_pinout.h"
#include "esp_mac.h"
#include "settings_page.h"
#include "waveshare_rgb_lcd_port.h"

#include <stdio.h>
#include "string.h"
#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "freertos/queue.h"
#include "esp_timer.h"
#include "esp_lcd_panel_ops.h"
#include "esp_lcd_panel_rgb.h"
#include "driver/gpio.h"
#include "esp_err.h"
#include "esp_log.h"
#include "esp_system.h"
#include "lvgl.h"
#include <nvs_flash.h>

void apply_theme_language_from_io(int theme, int language);
static void push_theme_language_to_io(void);
static void flush_pending_theme_language_from_io(void);
static volatile int s_pending_io_theme = -1;
static volatile int s_pending_io_lang = -1;
static volatile bool s_pending_io_prefs = false;
#include "cJson.h"

#include "driver/i2c.h" 
#include "esp_lcd_touch_gt911.h"
#include "waveshare_rgb_lcd_port.h"



static const char *TAG = "DISPLAY_MANAGER";



extern lv_obj_t * ui_scrTheme;
extern lv_obj_t * ui_scrWallpaper;
extern lv_obj_t * ui_scrPages;
extern lv_obj_t * ui_scrDimsandSensorsandWaters;



extern lv_obj_t * ui_lblVangoText;
extern lv_obj_t *ui_imgDevice;
extern lv_obj_t *ui_imgsconnected;
extern lv_obj_t *ui_imgsnotconnected;
extern lv_obj_t *ui_imgBluetoothNotConnected;
extern lv_obj_t *ui_imgBluetoothConnected;
extern lv_obj_t *ui_arcGrup1;
extern lv_obj_t *ui_arcGrup2;
extern lv_obj_t *ui_swO1; 
extern lv_obj_t *ui_swO2; 
extern lv_obj_t *ui_swO3; 
extern lv_obj_t *ui_swO4; 
extern lv_obj_t *ui_swO5; 
extern lv_obj_t *ui_swO6; 
extern lv_obj_t *ui_swO7; 
extern lv_obj_t *ui_swO8; 
extern lv_obj_t *ui_swO9; 
extern lv_obj_t *ui_swO10;
extern lv_obj_t *ui_swO11;
extern lv_obj_t *ui_swO12;
extern lv_obj_t *ui_swO13;
extern lv_obj_t *ui_swO14;
extern lv_obj_t *ui_swO15;
extern lv_obj_t *ui_swO16;

extern lv_obj_t* ui_cbxO1;
extern lv_obj_t* ui_cbxO2;
extern lv_obj_t* ui_cbxO3;
extern lv_obj_t* ui_cbxO4;
extern lv_obj_t* ui_cbxO5;
extern lv_obj_t* ui_cbxO6;
extern lv_obj_t* ui_cbxO7;
extern lv_obj_t* ui_cbxO8;
extern lv_obj_t* ui_cbxO9;
extern lv_obj_t* ui_cbxO10;
extern lv_obj_t* ui_cbxO11;
extern lv_obj_t* ui_cbxO12;
extern lv_obj_t* ui_cbxO13;
extern lv_obj_t* ui_cbxO14;
extern lv_obj_t* ui_cbxO15;
extern lv_obj_t* ui_cbxO16;



extern lv_obj_t* ui_swDim1;
extern lv_obj_t* ui_swDim2;
extern lv_obj_t* ui_swDim3;
extern lv_obj_t* ui_swDim4;

extern lv_obj_t* ui_cbxDim1;
extern lv_obj_t* ui_cbxDim2;
extern lv_obj_t* ui_cbxDim3;
extern lv_obj_t* ui_cbxDim4;

extern lv_obj_t* ui_cbRSelect;
extern lv_obj_t* ui_cbGSelect;
extern lv_obj_t* ui_cbBSelect;

// Dim slider widgets
extern lv_obj_t* ui_slDim1;
extern lv_obj_t* ui_slDim2;
extern lv_obj_t* ui_slDim3;
extern lv_obj_t* ui_slDim4;



// Missing extern declarations for theme functions
extern lv_obj_t * ui_Label9;
extern lv_obj_t * ui_Label12;
extern lv_obj_t * ui_lblDim1;
extern lv_obj_t * ui_lblDim2;
extern lv_obj_t * ui_lblDim3;
extern lv_obj_t * ui_lblDim4;
extern lv_obj_t * ui_lblDimLevel1;
extern lv_obj_t * ui_lblDimLevel2;
extern lv_obj_t * ui_lblDimLevel3;
extern lv_obj_t * ui_lblDimLevel4;
extern lv_obj_t * ui_lblTemperature1;
extern lv_obj_t * ui_lblTemperature2;
extern lv_obj_t * ui_lblUnderArcTemperature1;
extern lv_obj_t * ui_lblUnderArcTemperature2;
extern lv_obj_t * ui_lblWater1;
extern lv_obj_t * ui_lblWater2;
extern lv_obj_t * ui_lblUnderArcWater1;
extern lv_obj_t * ui_lblUnderArcWater2;

extern lv_obj_t* ui_Checkbox1;
extern lv_obj_t* ui_Checkbox2;
extern lv_obj_t* ui_Checkbox3;
extern lv_obj_t* ui_Checkbox4;
extern lv_obj_t* ui_Checkbox5;


extern lv_obj_t* ui_pnlOutputs;
extern lv_obj_t* ui_pnlConnectionLost;
extern lv_obj_t * ui_Panel9;
extern lv_obj_t * ui_Panel1;

extern lv_obj_t *ui_imgWForecast;
extern lv_obj_t *ui_lblDateAndTime;
extern lv_obj_t *ui_lblGrup1;
extern lv_obj_t *ui_lblGrup2;
extern lv_obj_t *ui_lblGrup1Oran1;
extern lv_obj_t *ui_lblGrup1Oran2;
extern lv_obj_t *ui_lblPnlGrup1Sicaklik1;
extern lv_obj_t *ui_lblPnlGrup1Sicaklik2;
lv_obj_t * ui_btnIOGot;

extern lv_obj_t *ui_swRGBTurnON;


extern lv_obj_t *ui_lblSelectTheme;
extern lv_obj_t *ui_lblPnlGrup1SicaklikDeger1;
extern lv_obj_t *ui_lblPnlGrup1SicaklikDeger2;
extern lv_obj_t *ui_arcTemperature1;
extern lv_obj_t *ui_arcTemperature2;
extern lv_obj_t *ui_lblTemperature1;
extern lv_obj_t *ui_lblTemperature2;
extern lv_obj_t *ui_arcWater1;
extern lv_obj_t *ui_arcWater2;
extern lv_obj_t *ui_lblWater1;
extern lv_obj_t *ui_lblWater2;


extern lv_obj_t * ui_scrInit;
extern lv_obj_t *ui_scrPanelSettings;
extern lv_obj_t *ui_lblPanelSettings;
extern lv_obj_t *ui_lblSensors;
extern lv_obj_t *ui_lblDimmableOutputs;
extern lv_obj_t *ui_pnlSensors;
extern lv_obj_t *ui_lblWeather;
extern lv_obj_t *ui_lblSettingsB;

extern lv_obj_t *ui_Colorwheel1;
extern lv_obj_t *ui_btnRGBColor;
extern lv_obj_t *ui_brInit;
extern lv_obj_t *ui_Label1;

extern lv_obj_t * ui_pbSaveConfigs;
extern lv_obj_t * ui_pnlSaveConfigs;



extern const lv_img_dsc_t ui_img_lamp_png;
extern const lv_img_dsc_t ui_img_water_png;
extern const lv_img_dsc_t ui_img_outlet_png;
extern const lv_img_dsc_t ui_img_oven_png;
extern const lv_img_dsc_t ui_img_tv_png;
extern const lv_img_dsc_t ui_img_refrigerator_png;
extern const lv_img_dsc_t ui_img_toilet_png;
extern const lv_img_dsc_t ui_img_usb_png;
extern const lv_img_dsc_t ui_img_ac_png;
extern const lv_img_dsc_t ui_img_readinglamp_png;
extern const lv_img_dsc_t ui_img_heater_png;

lv_color_t slc;

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



// Define weather conditions
#define WEATHER_SUNNY         1
#define WEATHER_PARTLY_SUNNY  2
#define WEATHER_THUNDER       3
#define WEATHER_RAINY         4
#define WEATHER_SNOWY         5
#define WEATHER_CLOUDY        6

// Define weather icon positions
#define ICON_SUNNY            0
#define ICON_PARTLY_SUNNY     34
#define ICON_THUNDER          75
#define ICON_RAINY            115
#define ICON_SNOWY            150
#define ICON_CLOUDY           190

// Analog Inputs
#define ANALOG_INPUT_1_INDIS 37
#define ANALOG_INPUT_2_INDIS 38
#define ANALOG_INPUT_3_INDIS 39
#define ANALOG_INPUT_4_INDIS 40
#define ANALOG_INPUT_5_INDIS 41

// Dimmable Outputs
#define DIMMABLE_OUTPUT_1_INDIS 42
#define DIMMABLE_OUTPUT_2_INDIS 43
#define DIMMABLE_OUTPUT_3_INDIS 44
#define DIMMABLE_OUTPUT_4_INDIS 45

// RGB Outputs
#define RGB_R_INDIS 46
#define RGB_G_INDIS 47
#define RGB_B_INDIS 48



#define I2C_MASTER_SCL_IO           9       /*!< GPIO number used for I2C master clock */
#define I2C_MASTER_SDA_IO           8       /*!< GPIO number used for I2C master data  */
#define I2C_MASTER_NUM              0       /*!< I2C master i2c port number, the number of i2c peripheral interfaces available will depend on the chip */
#define I2C_MASTER_FREQ_HZ          400000                     /*!< I2C master clock frequency */
#define I2C_MASTER_TX_BUF_DISABLE   0                          /*!< I2C master doesn't need buffer */
#define I2C_MASTER_RX_BUF_DISABLE   0                          /*!< I2C master doesn't need buffer */
#define I2C_MASTER_TIMEOUT_MS       1000

#define GPIO_INPUT_IO_4    4
#define GPIO_INPUT_PIN_SEL  1ULL<<GPIO_INPUT_IO_4


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////// Please update the following configuration according to your LCD spec //////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#define EXAMPLE_LCD_PIXEL_CLOCK_HZ     (18 * 1000 * 1000)
#define EXAMPLE_LCD_BK_LIGHT_ON_LEVEL  1
#define EXAMPLE_LCD_BK_LIGHT_OFF_LEVEL !EXAMPLE_LCD_BK_LIGHT_ON_LEVEL
#define EXAMPLE_PIN_NUM_BK_LIGHT       -1
#define EXAMPLE_PIN_NUM_HSYNC          46
#define EXAMPLE_PIN_NUM_VSYNC          3
#define EXAMPLE_PIN_NUM_DE             5
#define EXAMPLE_PIN_NUM_PCLK           7
#define EXAMPLE_PIN_NUM_DATA0          14 // B3
#define EXAMPLE_PIN_NUM_DATA1          38 // B4
#define EXAMPLE_PIN_NUM_DATA2          18 // B5
#define EXAMPLE_PIN_NUM_DATA3          17 // B6
#define EXAMPLE_PIN_NUM_DATA4          10 // B7
#define EXAMPLE_PIN_NUM_DATA5          39 // G2
#define EXAMPLE_PIN_NUM_DATA6          0 // G3
#define EXAMPLE_PIN_NUM_DATA7          45 // G4
#define EXAMPLE_PIN_NUM_DATA8          48 // G5
#define EXAMPLE_PIN_NUM_DATA9          47 // G6
#define EXAMPLE_PIN_NUM_DATA10         21 // G7
#define EXAMPLE_PIN_NUM_DATA11         1  // R3
#define EXAMPLE_PIN_NUM_DATA12         2  // R4
#define EXAMPLE_PIN_NUM_DATA13         42 // R5
#define EXAMPLE_PIN_NUM_DATA14         41 // R6
#define EXAMPLE_PIN_NUM_DATA15         40 // R7
#define EXAMPLE_PIN_NUM_DISP_EN        -1

// The pixel number in horizontal and vertical
#define EXAMPLE_LCD_H_RES              800
#define EXAMPLE_LCD_V_RES              480

#if CONFIG_EXAMPLE_DOUBLE_FB
#define EXAMPLE_LCD_NUM_FB             2
#else
#define EXAMPLE_LCD_NUM_FB             1
#endif // CONFIG_EXAMPLE_DOUBLE_FB

#define EXAMPLE_LVGL_TICK_PERIOD_MS    2
#define EXAMPLE_LVGL_TASK_MAX_DELAY_MS 1000
#define EXAMPLE_LVGL_TASK_MIN_DELAY_MS 1
#define EXAMPLE_LVGL_TASK_STACK_SIZE   (8 * 1024)
#define EXAMPLE_LVGL_TASK_PRIORITY     2

static SemaphoreHandle_t lvgl_mux = NULL;
static lv_timer_t * comm_animation_timer = NULL;



// Custom opacity animation callback
static void set_opacity_cb(void * var, int32_t val) {
    lv_obj_set_style_opa((lv_obj_t*)var, val, LV_PART_MAIN | LV_STATE_DEFAULT);
}

// Custom repetitive animation function
static void commScreen_RepetitiveAnimation(lv_obj_t * TargetObject) {
    // Create a simple repetitive animation that moves the object back and forth
    lv_anim_t anim;
    lv_anim_init(&anim);
    lv_anim_set_var(&anim, TargetObject);
    lv_anim_set_time(&anim, 1000);  // 1 second duration
    lv_anim_set_repeat_count(&anim, LV_ANIM_REPEAT_INFINITE);  // Repeat infinitely
    lv_anim_set_playback_time(&anim, 1000);  // 1 second playback
    lv_anim_set_values(&anim, -175, -75);  // Move from -175px to -75px (100px range)
    lv_anim_set_exec_cb(&anim, (lv_anim_exec_xcb_t) lv_obj_set_x);
    lv_anim_set_path_cb(&anim, lv_anim_path_ease_in_out);
    lv_anim_start(&anim);
}

// Custom repetitive animation function for Bluetooth connection
static void btScreen_RepetitiveAnimation(lv_obj_t * TargetObject) {
    // Create a simple repetitive animation that moves the object back and forth
    lv_anim_t anim;
    lv_anim_init(&anim);
    lv_anim_set_var(&anim, TargetObject);
    lv_anim_set_time(&anim, 1000);  // 1 second duration
    lv_anim_set_repeat_count(&anim, LV_ANIM_REPEAT_INFINITE);  // Repeat infinitely
    lv_anim_set_playback_time(&anim, 1000);  // 1 second playback
    lv_anim_set_values(&anim, 160, 60);  // Move from 160px to 60px (100px range)
    lv_anim_set_exec_cb(&anim, (lv_anim_exec_xcb_t) lv_obj_set_x);
    lv_anim_set_path_cb(&anim, lv_anim_path_ease_in_out);
    lv_anim_start(&anim);
}

// we use two semaphores to sync the VSYNC event and the LVGL task, to avoid potential tearing effect
#if CONFIG_EXAMPLE_AVOID_TEAR_EFFECT_WITH_SEM
SemaphoreHandle_t sem_vsync_end;
SemaphoreHandle_t sem_gui_ready;
#endif

// Define the maximum number of outputs
#define MAX_OUTPUTS 16

// Declare button, label, and image objects
lv_obj_t* btnIO[MAX_OUTPUTS] = {NULL};
lv_obj_t* lblIO[MAX_OUTPUTS] = {NULL};
lv_obj_t* imgIO[MAX_OUTPUTS] = {NULL};
lv_obj_t* sldDims[4] = {NULL};
lv_obj_t* lblDims[4] = {NULL};
// Define the button names and icons
const char* lblBtnNames[18] = {
    "LAMP", "TOILET", "KITCHEN", "BEDROOM", "CORRIDOR", "STEP", "AC", "USB", "REGRIGE.", "WATER P.", "OUTLET", "OVEN", "TV", "EX.LIGHT", "EX.OUTLET", "HEATER", "SPOT", "READING L."
};

const char* lblDimBtnNames[8] = {
    "LAMP", "TOILET", "KITCHEN", "BEDROOM", "CORRIDOR", "STEP", "EX. LIGHT", "SPOT"
};

//define turkish version og lblBtnNames and lblDimBtnNames
const char* lblBtnNames_TR[18] = {
    "LAMBA", "TUVALET", "MUTFAK", "YATAK OD.", "KORIDOR", "MERDIVEN", "KLIMA", "USB", "BUZDOLABI", "SU POMPASI", "PRIZ", "FIRIN", "TV", "DIS. LAMBA", "DIS. PRIZ", "ISI TICICI", "SPOT", "OKUMA L."
};
const char* lblDimBtnNames_TR[8] = {
    "LAMBA", "TUVALET", "MUTFAK", "YATAK OD.", "KORIDOR", "MERDIVEN", "DIS. LAMBA", "SPOT"
};

const char* lblBtnNames_DE[18] = {
    "LAMPE", "WC", "KUECHE", "SCHLAFZ.", "FLUR", "STUFE", "KLIMA", "USB", "KUEHL.",
    "WASSERP.", "STECKD.", "OFEN", "TV", "AUSSENL.", "AUSSENST.", "HEIZUNG", "SPOT", "LESEL."
};
const char* lblDimBtnNames_DE[8] = {
    "LAMPE", "WC", "KUECHE", "SCHLAFZ.", "FLUR", "STUFE", "AUSSENL.", "SPOT"
};


// Example data to save
int numOfOutputs = 16;
int numOfDims = 8;
int numOfSensors = 5;
float batarya_volt = 0;
bool slaveConnectionStatus = true;
int panelThemeType = 2; /* 0=light lock, 1=dark lock, 2=system (follow IO) */
int panelAccentIdx = 0; /* tablet accent: 0=vango … 7=green */
int panelWallpaperEnable = false;
int panelWallpaperTime = 0;
int numberOfNotifications = 0;
cJSON* notifications = NULL;
/** IO Module SoT effective appearance/lang (0/1). Used when local mode is system. */
int io_sot_theme = 0;
int io_sot_language = 0;
// Declare the global outputsBuffer
int outputsBuffer[16] = {0};
int sensorsBuffer[5] = {0};
int dimsBuffer[8] = {0};
int rgbBuffer[3] = {0};
int btn_index = 0;
uint8_t rgbEna = 0; // RGB LED enable variable
uint8_t fromBleConfig = 0;
int panelWallpaperEnableCounter = 1;
int panelLanguageType = 2; /* 0=EN lock, 1=TR lock, 2=system (follow IO), 3=DE lock */
int motorData = 0;
int pnlConnectionLostTimeout = 0;
int selected_R = 0;
int selected_G = 1;
int selected_B = 2;
extern int rgbCalibration = 0;

void parse_read_data(cJSON* json);
void parse_write_data(cJSON* json);
void parse_configuration_data(cJSON* json);
void parse_rules_data(cJSON* json);
char* create_json_data_packet(const uint16_t* regs_data, int numOfOutputs, int numOfDims, int numOfSensors, bool slaveConnectionStatus, int themeType, int numberOfNotifications, cJSON* notifications);
void parse_ble_data(const char* json_data);



// extern lv_obj_t *scr;
void example_lvgl_touch_cb(lv_indev_drv_t * drv, lv_indev_data_t * data)
{
    uint16_t touchpad_x[1] = {0};
    uint16_t touchpad_y[1] = {0};
    uint8_t touchpad_cnt = 0;

    /* Read touch controller data */
    esp_lcd_touch_read_data(drv->user_data);

    /* Get coordinates */
    bool touchpad_pressed = esp_lcd_touch_get_coordinates(drv->user_data, touchpad_x, touchpad_y, NULL, &touchpad_cnt, 1);
    ESP_LOGI(TAG, "Touchpad pressed: %d, x: %d, y: %d, cnt: %d", touchpad_pressed, touchpad_x[0], touchpad_y[0], touchpad_cnt);

    if (touchpad_pressed && touchpad_cnt > 0) {
        data->point.x = touchpad_x[0];
        data->point.y = touchpad_y[0];
        data->state = LV_INDEV_STATE_PR;
        panelWallpaperEnableCounter = 0;
    } else {
        data->state = LV_INDEV_STATE_REL;
    }
}


//###############################   DISPLAY MANAGER FUNCTIONS   ##########################################
void my_btnThemeWhiteFunc(void)
{
    if (!ui_scrMain) {
        return;
    }
    panelThemeType = 0;
    lv_obj_set_style_bg_color(ui_scrMain, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui_scrTheme, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui_scrSettings, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui_scrPanelSettings, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui_scrRGB, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui_scrPages, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui_scrDimsandSensorsandWaters, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);

    
    lv_obj_set_style_text_color(ui_lblPanelSettings, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui_lblDimmableOutputs, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);


    lv_obj_set_style_text_color(ui_lblPnlGrup1Sicaklik1, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui_lblPnlGrup1Sicaklik2, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui_lblGrup1Oran1, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui_lblGrup1Oran2, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui_lblGrup1, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui_lblGrup2, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);




    lv_obj_set_style_bg_color(ui_pnlSensors, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui_pnlOutputs, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);

    lv_obj_set_style_text_color(ui_lblVangoText, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);


    lv_obj_set_style_text_color(ui_lblSelectTheme, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui_lblWallpaper, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);

    lv_obj_set_style_text_color(ui_lblWeather, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui_lblDateAndTime, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui_lblSettingsB, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
    
    // Add missing widgets for white theme
    lv_obj_set_style_text_color(ui_lblPnlGrup1SicaklikDeger1, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui_lblPnlGrup1SicaklikDeger2, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);

    lv_obj_set_style_text_color(ui_Label8, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui_Label9, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
    
    // Dim and sensor widgets
    lv_obj_set_style_text_color(ui_lblDim1, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui_lblDim2, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui_lblDim3, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui_lblDim4, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui_lblTemperature1, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui_lblTemperature2, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui_lblUnderArcTemperature1, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui_lblUnderArcTemperature2, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui_lblWater1, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui_lblWater2, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui_lblUnderArcWater1, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui_lblUnderArcWater2, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);

    lv_obj_set_style_text_color(ui_lblDimmableOutputsB, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui_lblPanelSettingsB, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui_lblThemeSettingsB, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui_lblConnectionSettingsB, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui_lblSensorsB, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui_lblWaterLevelsB, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui_lblRGBsB, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui_lblDimLevel1, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui_lblDimLevel2, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui_lblDimLevel3, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui_lblDimLevel4, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);

    for (int i = 0; i < numOfDims && i < 4; i++) {
        lv_obj_set_style_text_color(lblDims[i], lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
    }

}




void my_btnBlackThemeFunc(void)
{
    if (!ui_scrMain) {
        return;
    }
    panelThemeType = 1;
    lv_obj_set_style_bg_color(ui_scrMain, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui_scrTheme, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui_scrSettings, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui_scrPanelSettings, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui_scrRGB, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui_scrPages, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui_scrDimsandSensorsandWaters, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);

    
    lv_obj_set_style_text_color(ui_lblPanelSettings, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui_lblDimmableOutputs, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);

    // ui_Checkbox6 removed - only 5 checkboxes exist

    lv_obj_set_style_text_color(ui_lblPnlGrup1Sicaklik1, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui_lblPnlGrup1Sicaklik2, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui_lblGrup1Oran1, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui_lblGrup1Oran2, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui_lblGrup1, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui_lblGrup2, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);




    lv_obj_set_style_bg_color(ui_pnlSensors, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui_pnlOutputs, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);

    lv_obj_set_style_text_color(ui_lblVangoText, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);

    lv_obj_set_style_text_color(ui_lblSelectTheme, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui_lblWallpaper, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);

    lv_obj_set_style_text_color(ui_lblWeather, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui_lblDateAndTime, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui_lblSettingsB, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
    
    // Add missing widgets for black theme
    lv_obj_set_style_text_color(ui_lblPnlGrup1SicaklikDeger1, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui_lblPnlGrup1SicaklikDeger2, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui_Label8, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui_Label9, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);

    
    // Dim and sensor widgets
    lv_obj_set_style_text_color(ui_lblDim1, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui_lblDim2, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui_lblDim3, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui_lblDim4, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui_lblTemperature1, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui_lblTemperature2, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui_lblUnderArcTemperature1, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui_lblUnderArcTemperature2, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui_lblWater1, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui_lblWater2, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui_lblUnderArcWater1, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui_lblUnderArcWater2, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);

    lv_obj_set_style_text_color(ui_lblDimmableOutputsB, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui_lblPanelSettingsB, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui_lblThemeSettingsB, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui_lblConnectionSettingsB, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui_lblSensorsB, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui_lblWaterLevelsB, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui_lblRGBsB, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui_lblDimLevel1, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui_lblDimLevel2, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui_lblDimLevel3, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui_lblDimLevel4, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);


    for (int i = 0; i < numOfDims && i < 4; i++) {
        lv_obj_set_style_text_color(lblDims[i], lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
    }
}



// Function to get the image name based on the button name
const void* get_image_for_button(int outputBufferIndex) {
    if (outputBufferIndex < 0 || outputBufferIndex >= 19) {
        return NULL; // Invalid index
    }

    const char* btnName = lblBtnNames[outputBufferIndex];

    if (strcmp(btnName, "KITCHEN") == 0 || strcmp(btnName, "LAMP") == 0 || strcmp(btnName, "EX.LIGHT") == 0 || strcmp(btnName, "SPOT") == 0 || strcmp(btnName, "BEDROOM") == 0 || strcmp(btnName, "CORRIDOR") == 0 || strcmp(btnName, "STEP") == 0) {
        return &ui_img_lamp_png;
    } else if (strcmp(btnName, "TOILET") == 0) {
        return &ui_img_toilet_png;
    } else if (strcmp(btnName, "OUTLET") == 0 || strcmp(btnName, "EX.OUTLET") == 0 || strcmp(btnName, "HEATER") == 0) {
        return &ui_img_outlet_png;
    } else if (strcmp(btnName, "USB") == 0) {
        return &ui_img_usb_png;
    } else if (strcmp(btnName, "REGRIGE.") == 0) {
        return &ui_img_refrigerator_png;
    } else if (strcmp(btnName, "WATER P.") == 0) {
        return &ui_img_water_png;
    } else if (strcmp(btnName, "OVEN") == 0) {
        return &ui_img_oven_png;
    } else if (strcmp(btnName, "TV") == 0) {
        return &ui_img_tv_png;
    } else if (strcmp(btnName, "AC") == 0) {
        return &ui_img_ac_png;
    } else if (strcmp(btnName, "HEATER") == 0) {
        return &ui_img_heater_png;
    } else if (strcmp(btnName, "READING L.") == 0) {
        return &ui_img_readinglamp_png;
    } else {
        return NULL; // No matching image found
    }
}



// Function to toggle button color based on regs_data
void button_events(lv_event_t* e) {

    int8_t val = 0;
    uint8_t can_data[8] = {0}; // CAN verisi için buffer
    lv_obj_t* btn = lv_event_get_target(e);
    btn_index = (int)lv_event_get_user_data(e);
    int16_t outpts = get_outputs();

    // Check the bit at btn_index
    if (((outpts >> btn_index) & 0x01) == 1) {
        val = 0; // If bit is 1, set val to 0
    } else {
        val = 1; // If bit is 0, set val to 1
    }
    
    can_data[0] = btn_index;  // İlk byte veri
    can_data[1] = val;  // İlk byte veri
    send_can_frame(0x720, can_data);  // Output için CAN ID: 0x720
    ESP_LOGI(TAG, "Button index: %d val: %d", btn_index, val);
    panelWallpaperEnableCounter = 0;
}

// Function to toggle dimmer value based on current value
void dim_events(lv_event_t* e) {
    uint8_t can_data[8] = {0}; // CAN data buffer
    lv_obj_t* slider = lv_event_get_target(e);
    int dim_index = (int)lv_event_get_user_data(e);

    // Get the new value from the slider
    int16_t dim_value = lv_slider_get_value(slider);

    // Update the corresponding bar on main screen (sldDims)
    if (dim_index < numOfDims && sldDims[dim_index] != NULL) {
        lv_bar_set_value(sldDims[dim_index], dim_value, LV_ANIM_OFF);
    }

    // Prepare CAN data: [index, value]
    can_data[0] = dim_index;
    can_data[1] = dim_value;

    // Send CAN frame for dimmer (example CAN ID: 0x730)
    send_can_frame(0x730, can_data);

    ESP_LOGI(TAG, "Dimmer index: %d, value: %d", dim_index, dim_value);
}
// Function to create the UI dynamically based on numOfOutputs
void create_dynamic_ui(lv_obj_t* parent) {

    int btn_width = 100;
    int btn_height = 150;
    int btn_x_offset = 106; // btn_width + 1 for spacing
    int btn_y_offset = 191; // btn_height + 1 for spacing
    int x_start = 14;
    int y_start = -102;

    // Apply numOfOutputs and outputsBuffer to swO1-swO16 and cbxO1-cbxO16
    lv_obj_t* switches[16] = {ui_swO1, ui_swO2, ui_swO3, ui_swO4, ui_swO5, ui_swO6, ui_swO7, ui_swO8, ui_swO9, ui_swO10, ui_swO11, ui_swO12, ui_swO13, ui_swO14, ui_swO15, ui_swO16};
    lv_obj_t* dropdowns[16] = {ui_cbxO1, ui_cbxO2, ui_cbxO3, ui_cbxO4, ui_cbxO5, ui_cbxO6, ui_cbxO7, ui_cbxO8, ui_cbxO9, ui_cbxO10, ui_cbxO11, ui_cbxO12, ui_cbxO13, ui_cbxO14, ui_cbxO15, ui_cbxO16};
    lv_obj_t* checkboxes[5] = {ui_Checkbox1, ui_Checkbox2, ui_Checkbox3, ui_Checkbox4, ui_Checkbox5};

    lv_obj_t* dimcheckboxes[4] = {ui_swDim1, ui_swDim2, ui_swDim3, ui_swDim4};
    lv_obj_t* dimdropdowns[4] = {ui_cbxDim1, ui_cbxDim2, ui_cbxDim3, ui_cbxDim4};

    // Adjust button size and spacing if numOfOutputs is greater than 8
    if (numOfOutputs > 8) {
        btn_width = 100;
        btn_height = 90;
        btn_x_offset = 106; // btn_width + 1 for spacing
        btn_y_offset = 96; // btn_height + 1 for spacing
        y_start = -150;
    }

    for (int i = 0; i < numOfOutputs; i++) {
        int row = i / 4;
        int col = i % 4;


        btnIO[i] = lv_btn_create(parent);
        lv_obj_set_width(btnIO[i], btn_width);
        lv_obj_set_height(btnIO[i], btn_height);
        lv_obj_set_x(btnIO[i], x_start + col * btn_x_offset);
        lv_obj_set_y(btnIO[i], y_start + row * btn_y_offset);
        lv_obj_set_align(btnIO[i], LV_ALIGN_CENTER);
        lv_obj_add_flag(btnIO[i], LV_OBJ_FLAG_SCROLL_ON_FOCUS);     /// Flags
        lv_obj_clear_flag(btnIO[i], LV_OBJ_FLAG_SCROLLABLE);      /// Flags
        lv_obj_set_style_radius(btnIO[i], 10, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_bg_color(btnIO[i], lv_color_hex(0x5A5A5A), LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_bg_opa(btnIO[i], 255, LV_PART_MAIN | LV_STATE_DEFAULT);

        lblIO[i] = lv_label_create(btnIO[i]);
        lv_obj_set_width(lblIO[i], LV_SIZE_CONTENT);   /// 1
        lv_obj_set_height(lblIO[i], LV_SIZE_CONTENT);    /// 1
        lv_obj_set_x(lblIO[i], 0);

        if (numOfOutputs > 8) {
            lv_obj_set_align(lblIO[i], LV_ALIGN_BOTTOM_MID);
            lv_obj_set_y(lblIO[i], btn_height / 2 - 35); // Adjust y position to align at the bottom mid
        }
        else {

            lv_obj_set_y(lblIO[i], 0); // Adjust y position to align at the bottom mid
            lv_obj_set_align(lblIO[i], LV_ALIGN_CENTER);
        }
        lv_obj_set_align(lblIO[i], LV_ALIGN_BOTTOM_MID);
        lv_label_set_text(lblIO[i], lblBtnNames[outputsBuffer[i] - 1]);
        lv_obj_set_style_text_color(lblIO[i], lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_text_opa(lblIO[i], 255, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_text_font(lblIO[i], &lv_font_montserrat_14, LV_PART_MAIN | LV_STATE_DEFAULT);

        imgIO[i] = lv_img_create(btnIO[i]);
        lv_img_set_src(imgIO[i], get_image_for_button(outputsBuffer[i] - 1));
        lv_obj_set_width(imgIO[i], LV_SIZE_CONTENT);   /// 1
        lv_obj_set_height(imgIO[i], LV_SIZE_CONTENT);    /// 1
        lv_obj_set_x(imgIO[i], 0);
        lv_obj_set_y(imgIO[i], 5);
        if(numOfOutputs > 8) {
            lv_obj_set_align(imgIO[i], LV_ALIGN_TOP_MID);
        }
        else {
            lv_obj_set_align(imgIO[i], LV_ALIGN_CENTER);
        }
        
        lv_obj_add_flag(imgIO[i], LV_OBJ_FLAG_ADV_HITTEST);     /// Flags
        lv_obj_clear_flag(imgIO[i], LV_OBJ_FLAG_SCROLLABLE);      /// Flags
        // Add event callback for button click
        lv_obj_add_event_cb(btnIO[i], button_events, LV_EVENT_CLICKED, (void*)i);
    }


        for (int i = 0; i < numOfDims && i < 4; i++) {
        // Create the bar
        sldDims[i] = lv_bar_create(ui_pnlSensors);
        lv_bar_set_value(sldDims[i], 0, LV_ANIM_OFF);
        lv_obj_set_width(sldDims[i], 305);
        lv_obj_set_height(sldDims[i], 14);
        lv_obj_set_x(sldDims[i], -6);
        lv_obj_set_y(sldDims[i], -56 + i * 40); // Adjust y position dynamically
        lv_obj_set_align(sldDims[i], LV_ALIGN_CENTER);

        // Make bar non-clickable
        lv_obj_clear_flag(sldDims[i], LV_OBJ_FLAG_CLICKABLE);

        lv_obj_add_event_cb(sldDims[i], dim_events, LV_EVENT_RELEASED, (void*)i);

        // Create the label
        lblDims[i] = lv_label_create(ui_pnlSensors);
        lv_obj_set_width(lblDims[i], LV_SIZE_CONTENT);
        lv_obj_set_height(lblDims[i], LV_SIZE_CONTENT);
        lv_obj_set_x(lblDims[i], -135);
        lv_obj_set_y(lblDims[i], -77 + i * 40); // Adjust y position dynamically
        lv_obj_set_align(lblDims[i], LV_ALIGN_RIGHT_MID);
        lv_label_set_text_fmt(lblDims[i], "%s:", lblDimBtnNames[dimsBuffer[i] - 1]);
    }


    for (int i = 0; i < 16; i++) {
        if (i < numOfOutputs) {
            lv_obj_add_state(switches[i], LV_STATE_CHECKED); // Check the switch
            lv_dropdown_set_selected(dropdowns[i], outputsBuffer[i] - 1); // Set the dropdown value
            lv_obj_add_flag(dropdowns[i], LV_OBJ_FLAG_CLICKABLE); // Make the dropdown clickable
        } else{
            lv_obj_clear_state(switches[i], LV_STATE_CHECKED); // Uncheck the switch
            lv_obj_clear_flag(switches[i], LV_OBJ_FLAG_CLICKABLE); // Make the switch non-clickable
            lv_dropdown_set_selected(dropdowns[i], 0); // Reset the dropdown value
            lv_obj_clear_flag(dropdowns[i], LV_OBJ_FLAG_CLICKABLE); // Make the dropdown non-clickable
        }
    }
    
    // Ensure only the last checkbox is clickable
    for (int i = 0; i < 16; i++) {
        if (i == numOfOutputs - 1) {
            lv_obj_add_flag(switches[i], LV_OBJ_FLAG_CLICKABLE); // Make the last switch clickable
            lv_obj_add_flag(switches[i + 1], LV_OBJ_FLAG_CLICKABLE);
            break;
        } else {
            lv_obj_clear_flag(switches[i], LV_OBJ_FLAG_CLICKABLE);
        }
    }



    //Dims buffer apply to panel settings
    for (int i = 0; i < 4; i++) {
        if (i < numOfDims) {
            lv_obj_add_state(dimcheckboxes[i], LV_STATE_CHECKED); // Check the switch
            lv_dropdown_set_selected(dimdropdowns[i], dimsBuffer[i] - 1); // Set the dropdown value
            lv_obj_add_flag(dimdropdowns[i], LV_OBJ_FLAG_CLICKABLE); // Make the dropdown clickable
        } else {
            lv_obj_clear_state(dimcheckboxes[i], LV_STATE_CHECKED); // Uncheck the switch
            lv_obj_clear_flag(dimcheckboxes[i], LV_OBJ_FLAG_CLICKABLE); // Make the switch non-clickable
            lv_dropdown_set_selected(dimdropdowns[i], 0); // Reset the dropdown value
            lv_obj_clear_flag(dimdropdowns[i], LV_OBJ_FLAG_CLICKABLE); // Make the dropdown non-clickable
        }
    }
    
    // Ensure only the last checkbox is clickable
    for (int i = 0; i < 4; i++) {
        if (i == numOfDims - 1) {
            lv_obj_add_flag(dimcheckboxes[i], LV_OBJ_FLAG_CLICKABLE); // Make the last switch clickable
            lv_obj_add_flag(dimcheckboxes[i + 1], LV_OBJ_FLAG_CLICKABLE);
            break;
        } else {
            lv_obj_clear_flag(dimcheckboxes[i], LV_OBJ_FLAG_CLICKABLE);
        }
    }

    if (numOfDims == 0) {
        lv_obj_add_flag(dimcheckboxes[0], LV_OBJ_FLAG_CLICKABLE);
    }
    if (numOfOutputs == 0) {
        lv_obj_add_flag(switches[0], LV_OBJ_FLAG_CLICKABLE);
    }

    {
        const int saved_theme = panelThemeType;
        if (vango_theme_dark()) {
            my_btnBlackThemeFunc();
        } else {
            my_btnThemeWhiteFunc();
        }
        panelThemeType = saved_theme; /* black/white helpers overwrite type */
    }
    apply_theme_settings();
    apply_language_settings();
    lv_obj_set_style_transform_zoom(ui_lblScreenClock, 1024, 0); // 256 = 1x, 512 = 2x
    lv_dropdown_set_selected(ui_dbLanguage, 1 - panelLanguageType);


}


static void connectionLostPopupTimerr(lv_timer_t * timer) {
    lv_obj_add_flag(ui_pnlConnectionLost, LV_OBJ_FLAG_HIDDEN);     /// Flags
}


 lv_timer_t * ConnectionLostPopupTimer = NULL;
// Function to set the image source based on connection status
void set_device_image(bool connected) {
    static bool condition = false;
    if (connected) {
        lv_obj_clear_flag(ui_imgsconnected, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(ui_imgsnotconnected, LV_OBJ_FLAG_HIDDEN);

        condition = true;


    } else {
        lv_obj_clear_flag(ui_imgsnotconnected, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(ui_imgsconnected, LV_OBJ_FLAG_HIDDEN);
        if (condition && !connected)
        {
            condition = false;
            lv_obj_clear_flag(ui_pnlConnectionLost, LV_OBJ_FLAG_HIDDEN);     /// Flags
            lv_obj_move_foreground(ui_pnlConnectionLost);
            lv_label_set_text(ui_Label1, "IO Module Connection Lost!");
            ConnectionLostPopupTimer = lv_timer_create(connectionLostPopupTimerr, 60000, NULL);
        }
        
    }  
}



// Function to set the image source based on connection status
void set_bluetooth_icon(bool connected) {
    static bool bcondition = false;
    if (connected) {
        lv_obj_add_flag(ui_imgBluetoothNotConnected, LV_OBJ_FLAG_HIDDEN);
        lv_obj_clear_flag(ui_imgBluetoothConnected, LV_OBJ_FLAG_HIDDEN);  
        bcondition = true;
    } else {
        lv_obj_clear_flag(ui_imgBluetoothNotConnected, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(ui_imgBluetoothConnected, LV_OBJ_FLAG_HIDDEN);
        if (bcondition && !connected)
        {
            bcondition = false;
            lv_obj_clear_flag(ui_pnlConnectionLost, LV_OBJ_FLAG_HIDDEN);     /// Flags
            lv_obj_move_foreground(ui_pnlConnectionLost);
            lv_label_set_text(ui_Label1, "Bluetooth Connection Lost!");
            ESP_LOGI(TAG, "Bluetooth Connection Lost Popup Shown");
            ConnectionLostPopupTimer = lv_timer_create(connectionLostPopupTimerr, 60000, NULL);
        }
    }
}


void set_RGBTurnONOFF(int val)
{
    lv_color_t new_color = lv_color_make(0x5A, 0x5A, 0x5A);
    if(val == 1) {
        rgbEna = 1;
    }
    else if(val == 0) {
        if (ui_btnRGBColor) {
            lv_obj_set_style_bg_color(ui_btnRGBColor, new_color, LV_PART_MAIN | LV_STATE_DEFAULT);
        }
        rgbEna = 0;
    }
}


int initBarCounter = 0;
int initCounter = 0;
int scrMode = 0;
static void init_timer(lv_timer_t * timer) {
    // Check if ui_brInit is valid before using it
    if (ui_brInit != NULL && initBarCounter < 21) {
        lv_bar_set_value(ui_brInit, initBarCounter * 5, LV_ANIM_OFF);   
    }
    initBarCounter++;
}

static void sendMotorDataPeriodic_timer(lv_timer_t * timer) {
    /* Basılı tutma yenilemesi. Dur (0) bırakma anında ui_vango_dash motor_can_send(0) ile gider. */
    (void)timer;
    if (motorData != 0) {
        uint8_t can_data[8] = {0};
        can_data[0] = (uint8_t)motorData;
        send_can_frame(0x750, can_data);
    }
}


// Timer callback function
static void timer_updateTimer_callback(lv_timer_t * timer) {
    if (initCounter < 10) {
        if (scrMode == 0) {
            scrMode = 1;
        }
    }
    else {
        if (scrMode == 1) {
            /* Yeni koyu dashboard — legacy ui_scrMain arka planda kalir (IO rebuild) */
            if (vango_dash_get_screen() != NULL) {
                vango_dash_show(VANGO_PAGE_HOME);
            } else if (ui_scrMain != NULL) {
                lv_scr_load(ui_scrMain);
            }
            scrMode = 0;
        }
        initCounter = 11;
        // Your code here, e.g., update display with new data
        const uint16_t* regs_data = getSlavesRegsData();
        if (regs_data != NULL) {
            update_display_with_data((const uint8_t*)regs_data, 70);
        }
    }
    initCounter++;

}


void hexnet_wallpaper_note_activity(void)
{
    panelWallpaperEnableCounter = 0;
}

void hexnet_wallpaper_show(void)
{
    /* Legacy ui_init() skipped — create only the screensaver screen. */
    if (!ui_scrWallpaper) {
        ui_scrWallpaper_screen_init();
    }
    if (!ui_scrWallpaper) {
        return;
    }
    /* No BLE clock → logo; with clock text hide logo (same as legacy). */
    if (ui_lblScreenClock && ui_imgScreenLogo) {
        const char *clk = lv_label_get_text(ui_lblScreenClock);
        if (clk && clk[0]) {
            lv_obj_clear_flag(ui_lblScreenClock, LV_OBJ_FLAG_HIDDEN);
            lv_obj_add_flag(ui_imgScreenLogo, LV_OBJ_FLAG_HIDDEN);
        } else {
            lv_obj_add_flag(ui_lblScreenClock, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_flag(ui_imgScreenLogo, LV_OBJ_FLAG_HIDDEN);
        }
    }
    panelWallpaperEnableCounter = 0;
    lv_scr_load(ui_scrWallpaper);
}

void hexnet_wallpaper_dismiss(void)
{
    panelWallpaperEnableCounter = 0;
    if (vango_dash_get_screen() != NULL) {
        lv_scr_load(vango_dash_get_screen());
        return;
    }
    if (ui_scrMain) {
        lv_scr_load(ui_scrMain);
    }
}

static void wallpaper_update_timer_callback(lv_timer_t * timer) {
    (void)timer;
    if (!panelWallpaperEnable || panelWallpaperTime <= 0) {
        panelWallpaperEnableCounter = 0;
        return;
    }
    /* Already on screensaver — wait for touch dismiss. */
    if (ui_scrWallpaper && lv_scr_act() == ui_scrWallpaper) {
        return;
    }
    if (panelWallpaperEnableCounter >= panelWallpaperTime) {
        hexnet_wallpaper_show();
        return;
    }
    panelWallpaperEnableCounter++;
}

// Timer callback for communication animation
static void comm_animation_timer_callback(lv_timer_t * timer) {
    static bool comm_animation_started = false;
    static bool bt_animation_started = false;
    
    bool Deviceconnected = get_canbus_connection_status();
    bool btConnected = get_connection_status();
    
    // Handle device connection animation (ui_Panel9)
    if (Deviceconnected && !comm_animation_started) {
        // Start the repetitive animation when device connects
        commScreen_RepetitiveAnimation(ui_Panel9);
        comm_animation_started = true;
        // Set color to green when connected
        lv_obj_set_style_bg_color(ui_Panel9, lv_color_hex(0x00FF00), LV_PART_MAIN | LV_STATE_DEFAULT);
    } else if (!Deviceconnected && comm_animation_started) {
        // Stop the animation when device disconnects
        lv_anim_del(ui_Panel9, NULL);  // Stop all animations on this object
        comm_animation_started = false;
        // Set position to -100px and color to red when disconnected
        lv_obj_set_x(ui_Panel9, -100);
        lv_obj_set_style_bg_color(ui_Panel9, lv_color_hex(0xFF0000), LV_PART_MAIN | LV_STATE_DEFAULT);
    } else if (!Deviceconnected && !comm_animation_started) {
        // Ensure position and color are set correctly when disconnected
        lv_obj_set_x(ui_Panel9, -100);
        lv_obj_set_style_bg_color(ui_Panel9, lv_color_hex(0xFF0000), LV_PART_MAIN | LV_STATE_DEFAULT);
    }
    
    // Handle Bluetooth connection animation (ui_Panel1)
    if (btConnected && !bt_animation_started) {
        // Start the repetitive animation when Bluetooth connects
        btScreen_RepetitiveAnimation(ui_Panel1);
        bt_animation_started = true;
        // Set color to green when connected
        lv_obj_set_style_bg_color(ui_Panel1, lv_color_hex(0x00FF00), LV_PART_MAIN | LV_STATE_DEFAULT);
    } else if (!btConnected && bt_animation_started) {
        // Stop the animation when Bluetooth disconnects
        lv_anim_del(ui_Panel1, NULL);  // Stop all animations on this object
        bt_animation_started = false;
        // Set position to 110px and color to red when disconnected
        lv_obj_set_x(ui_Panel1, 110);
        lv_obj_set_style_bg_color(ui_Panel1, lv_color_hex(0xFF0000), LV_PART_MAIN | LV_STATE_DEFAULT);
    } else if (!btConnected && !bt_animation_started) {
        // Ensure position and color are set correctly when disconnected
        lv_obj_set_x(ui_Panel1, 110);
        lv_obj_set_style_bg_color(ui_Panel1, lv_color_hex(0xFF0000), LV_PART_MAIN | LV_STATE_DEFAULT);
    }
}

// Function to set the button color based on the value
void set_button_color(lv_obj_t *btn, uint16_t value, int connected) {
    if (connected == 0) {
        lv_obj_set_style_bg_color(btn, lv_color_hex(0x5F5F5F), LV_PART_MAIN | LV_STATE_DEFAULT); // Gray
        return;
    }
    if (value == 1) {
        lv_obj_set_style_bg_color(btn, lv_color_hex(0x37C600), LV_PART_MAIN | LV_STATE_DEFAULT); // Green
    } else if (value == 0) {
        lv_obj_set_style_bg_color(btn, lv_color_hex(0x5A5A5A), LV_PART_MAIN | LV_STATE_DEFAULT); // Gray
    } else if (value == 2) {
        lv_obj_set_style_bg_color(btn, lv_color_hex(0xC60000), LV_PART_MAIN | LV_STATE_DEFAULT); // Red
    }
}

void get_data_json_format(const uint16_t* regs_data, int txPacketType, char** json_str)  {

    // Call create_json_data_packet function
    *json_str = create_json_data_packet(regs_data, numOfOutputs, numOfDims, numOfSensors, slaveConnectionStatus, panelThemeType, numberOfNotifications, notifications);
    // Check if notifications is not NULL before deleting
    if (notifications != NULL) {
        cJSON_Delete(notifications);
        notifications = NULL; // Set to NULL after deletion to avoid dangling pointer
    }
} 





// Function to update display with new data
static bool s_io_profile_ui_rebuild_pending = false;

static void io_profile_ui_rebuild_timer_cb(lv_timer_t * timer)
{
    lv_timer_del(timer);
    hexnet_io_profile_sync_legacy_globals();

    /* Yeni dash aktifken legacy SquareLine rebuild reset/crash riski — atla */
    if (vango_dash_get_screen() != NULL || !ui_scrMain) {
        s_io_profile_ui_rebuild_pending = false;
        ESP_LOGW(TAG, "IO profile sync (dash/legacy skip rebuild): R=%d D=%d",
                 numOfOutputs, numOfDims);
        return;
    }

    /* Onceki buton/slider'lari sil (sayi azalinca eski widget kalmasin) */
    for (int i = 0; i < MAX_OUTPUTS; i++) {
        if (btnIO[i] != NULL) {
            lv_obj_del(btnIO[i]);
            btnIO[i] = NULL;
            lblIO[i] = NULL;
            imgIO[i] = NULL;
        }
    }
    for (int i = 0; i < 4; i++) {
        if (sldDims[i] != NULL) {
            lv_obj_del(sldDims[i]);
            sldDims[i] = NULL;
        }
        if (lblDims[i] != NULL) {
            lv_obj_del(lblDims[i]);
            lblDims[i] = NULL;
        }
    }

    create_dynamic_ui(ui_scrMain);
    s_io_profile_ui_rebuild_pending = false;
    ESP_LOGW(TAG, "IO profile UI rebuilt (no reset): R=%d D=%d", numOfOutputs, numOfDims);
}

void update_display_with_data(const uint8_t *data, int length) {
    const uint16_t* regs_data = (const uint16_t*)data;

    // Fetch data from registers — su >100 (kopuk sentinel) gosterilmesin
    uint16_t analog_input_1 = get_analog_input(0);
    uint16_t analog_input_2 = get_analog_input(1);
    uint16_t analog_input_3 = get_analog_input(2);
    uint16_t analog_input_4 = get_analog_input(3);
    uint16_t analog_input_5 = get_analog_input(4);
    if (analog_input_1 > 100) analog_input_1 = 0;
    if (analog_input_2 > 100) analog_input_2 = 0;
    if (analog_input_3 > 100) analog_input_3 = 0;
    batarya_volt = get_voltage() / 100.0;

    /* Eski SquareLine ekranı kurulmadıysa yalnız yeni dashboard güncellenir. */
    if (!ui_scrMain) {
        flush_pending_theme_language_from_io();
        vango_dash_set_battery_v(batarya_volt);
        {
            const int th_t = (int)get_sensorTemp();
            const int th_h = (int)get_sensorHum();
            const bool th_sample = (th_h >= 0 && th_h <= 100);
            if (th_sample) {
                vango_dash_set_temp_c(th_t);
            }
            if (get_canbus_connection_status() && th_sample) {
                vango_dash_set_th(th_t, th_h, true);
            } else if (get_canbus_connection_status()) {
                vango_dash_set_th(-1, -1, false);
            } else {
                vango_dash_set_th(-1, -1, false);
            }
        }
        if (get_canbus_connection_status()) {
            vango_dash_set_water((int)get_analog_input(0),
                                 (int)get_analog_input(1),
                                 (int)get_analog_input(2));
        } else {
            vango_dash_set_water(-1, -1, -1);
        }
        {
            const bool can_up = get_canbus_connection_status() != 0;
            vango_dash_set_link_status(can_up, get_io_ble_link() != 0, get_io_wifi_link() != 0);
            vango_dash_set_motor_link(can_up);
            vango_dash_set_dimmer_link(can_up);
        }
        vango_dash_sync_dimmers();
        vango_dash_sync_relays();
        /* BLE JSON: throttle — her 250ms cJSON IWDT/heap baskisi yapıyordu. */
        if (get_connection_status()) {
            static int64_t s_ble_json_last_us;
            static uint16_t s_ble_json_out;
            const uint16_t out_bits = get_outputs();
            const int64_t now_us = esp_timer_get_time();
            const bool changed = (out_bits != s_ble_json_out);
            const bool due = (now_us - s_ble_json_last_us) >= 2000000LL; /* 2s */
            if (changed || due || s_ble_json_last_us == 0) {
                char *converted_json_data = NULL;
                get_data_json_format(regs_data, 0, &converted_json_data);
                if (converted_json_data) {
                    set_converted_json_data(converted_json_data);
                    s_ble_json_last_us = now_us;
                    s_ble_json_out = out_bits;
                }
            }
        }
        return;
    }
    float amper = regs_data[CURRENT_INDIS] / 100.0;

    // Convert voltage to string with comma
    char batarya_volt_str[20];
    if (batarya_volt < 7.0 || batarya_volt > 16.0) {
        //ESP_LOGW(TAG, "Voltage out of range: %.2fV", batarya_volt);
        snprintf(batarya_volt_str, sizeof(batarya_volt_str), "-");
    } else {
        int before_comma_volt = (int)batarya_volt;
        int after_comma_volt = (int)((batarya_volt - before_comma_volt) * 100);
        if(panelLanguageType == 0) {
            // Turkish
            snprintf(batarya_volt_str, sizeof(batarya_volt_str), "Batarya: %d,%02dV", before_comma_volt, after_comma_volt);
        } else {
            // English  
        snprintf(batarya_volt_str, sizeof(batarya_volt_str), "Battery: %d,%02dV", before_comma_volt, after_comma_volt);
        }
    }

    // Convert current to string with comma
    char amper_str[10];
    if (amper < 0 || amper > 100.0) {
        //ESP_LOGW(TAG, "Current out of range: %.2fA", amper);
        snprintf(amper_str, sizeof(amper_str), "-");
    } else {
        int before_comma_amper = (int)amper;
        int after_comma_amper = (int)((amper - before_comma_amper) * 100);
        snprintf(amper_str, sizeof(amper_str), "%d,%02dA", before_comma_amper, after_comma_amper);
    }


    // Update the display labels with the fetched data
    lv_label_set_text_fmt(ui_lblPnlGrup1SicaklikDeger1, "%d°C", (uint16_t)get_sensorTemp());
    lv_label_set_text_fmt(ui_lblPnlGrup1SicaklikDeger2, "%d%%", (uint16_t)get_sensorHum());
    lv_label_set_text_fmt(ui_lblGrup1Oran1, "%d%%", analog_input_1);
    lv_label_set_text_fmt(ui_lblGrup1Oran2, "%d%%", analog_input_2);
    
    // Update temperature widgets with analog input values
    lv_arc_set_value(ui_arcTemperature1, get_sensorTemp());
    lv_arc_set_value(ui_arcTemperature2, get_sensorHum());
    lv_label_set_text_fmt(ui_lblTemperature1, "%d°C", get_sensorTemp());
    lv_label_set_text_fmt(ui_lblTemperature2, "%d%%", get_sensorHum());
    
    // Update water widgets with analog input values
    lv_arc_set_value(ui_arcWater1, analog_input_1);
    lv_arc_set_value(ui_arcWater2, analog_input_2);
    lv_label_set_text_fmt(ui_lblWater1, "%d%%", analog_input_1);
    lv_label_set_text_fmt(ui_lblWater2, "%d%%", analog_input_2);

    // Update the arcs with the fetched data
    lv_arc_set_value(ui_arcGrup1, analog_input_1);
    lv_arc_set_value(ui_arcGrup2, analog_input_2);

    /* Yeni dashboard status bar */
    flush_pending_theme_language_from_io();

    vango_dash_set_battery_v(batarya_volt);
    vango_dash_set_temp_c(get_sensorTemp());
    vango_dash_set_water((int)analog_input_1, (int)analog_input_2, 0);
    {
        const bool can_up = get_canbus_connection_status() != 0;
        vango_dash_set_link_status(can_up, get_io_ble_link() != 0, get_io_wifi_link() != 0);
    }

    // Check Modbus connection status and update device image
    //bool Deviceconnected = get_modbus_connection_status();
    bool Deviceconnected = get_canbus_connection_status();
    set_device_image(Deviceconnected);
    //if (Deviceconnected) apply_rgb_data_to_wheel(get_rgb_value(0), get_rgb_value(1), get_rgb_value(2));

    bool btConnected = get_connection_status();
    set_bluetooth_icon(btConnected);

    if(!btConnected){
        //If there is no connection, the time and date will not be updated. Only Battery voltage will be updated.
        lv_label_set_text(ui_lblDateAndTime, batarya_volt_str);
        lv_obj_add_flag(ui_imgWForecast, LV_OBJ_FLAG_HIDDEN);     /// Flags
        lv_obj_add_flag(ui_lblScreenClock, LV_OBJ_FLAG_HIDDEN);     /// Flags
        lv_obj_clear_flag(ui_imgScreenLogo, LV_OBJ_FLAG_HIDDEN);     /// Flags

    }
    else{
        char* converted_json_data;
        get_data_json_format(regs_data, 0, &converted_json_data);
        set_converted_json_data(converted_json_data);
        lv_obj_clear_flag(ui_lblScreenClock, LV_OBJ_FLAG_HIDDEN);     /// Flags
        lv_obj_add_flag(ui_imgScreenLogo, LV_OBJ_FLAG_HIDDEN);     /// Flags
    }

    if(rgbCalibration != 0){
        if(rgbCalibration == 1){
            set_RGB_color(255, 0, 0);
        }
        else if(rgbCalibration == 2){
            set_RGB_color(0, 255, 0);
        }
        else if(rgbCalibration == 3){
            set_RGB_color(0, 0, 255);
        }   
        rgbCalibration = 0;
    }



    // Ensure create_dynamic_ui is called only once at boot; later profile changes rebuild in-place
    static int ui_initialized = 0;
    /* Profile dirty → legacy UI rebuild. Labels dash sync'te debounce edilir
     * (burst ortasinda consume/NVS/paint CAN kaybettiriyordu). */
    if (ui_initialized >= 3 && !s_io_profile_ui_rebuild_pending &&
        hexnet_io_profile_consume_ui_dirty()) {
        s_io_profile_ui_rebuild_pending = true;
        ESP_LOGW(TAG, "IO profile CAN sync: rebuild UI (no reset) relays=%d", numOfOutputs);
        lv_timer_t *rst = lv_timer_create(io_profile_ui_rebuild_timer_cb, 50, NULL);
        if (rst) {
            lv_timer_set_repeat_count(rst, 1);
        } else {
            s_io_profile_ui_rebuild_pending = false;
        }
    }
    if (ui_initialized==2) {
        create_dynamic_ui(ui_scrMain);
        ui_initialized = 3;
    }
    if (ui_initialized > 3 && Deviceconnected && !s_io_profile_ui_rebuild_pending) {
        for (int i = 0; i < numOfOutputs; i++) {
            if (btnIO[i] != NULL) {
                set_button_color(btnIO[i], (get_outputs() >> i)&0x01, Deviceconnected);
            }
        }
        for (int i = 0; i < numOfDims && i < 8; i++) {
            uint8_t d = get_dimmable_output(i);
            if (d > 100) {
                d = 0;
            }
            if (i < 4 && sldDims[i] != NULL) {
                lv_bar_set_value(sldDims[i], d, LV_ANIM_OFF);
            }
        }
        /* Legacy SquareLine dim sayfasi (4 slider) — yeni dash 8 kanali ayri surer */
        lv_obj_t *dimSliders[4] = {ui_slDim1, ui_slDim2, ui_slDim3, ui_slDim4};
        lv_obj_t *dimLevels[4] = {ui_lblDimLevel1, ui_lblDimLevel2, ui_lblDimLevel3, ui_lblDimLevel4};
        for (int i = 0; i < 4; i++) {
            uint8_t d = get_dimmable_output(i);
            if (d > 100) {
                d = 0;
            }
            if (dimSliders[i] != NULL) {
                lv_slider_set_value(dimSliders[i], d, LV_ANIM_OFF);
            }
            if (dimLevels[i] != NULL) {
                lv_label_set_text_fmt(dimLevels[i], "%u", (unsigned)d);
            }
        }
    }
    ui_initialized++;
}


//######################################################################################################################




//###################################### JSON DATA PACKET FUNCTIONS ####################################################

// Example function to create JSON data packet as a C string
char* create_json_data_packet(const uint16_t* regs_data, int numOfOutputs, int numOfDims, int numOfSensors, bool slaveConnectionStatus, int themeType, int numberOfNotifications, cJSON* notifications) {
    // Create a JSON object
    cJSON *json = cJSON_CreateObject();

    // Add number of outputs, dims, sensors, slave connection status, and theme type to the JSON object
    cJSON_AddStringToObject(json, "slvConn", slaveConnectionStatus ? "Yes" : "No");
    cJSON_AddNumberToObject(json, "numOfO", numOfOutputs);
    cJSON_AddNumberToObject(json, "numOfD", numOfDims);
    cJSON_AddNumberToObject(json, "numOfS", numOfSensors);
    cJSON_AddStringToObject(json, "RGBE", "yes");
    cJSON_AddNumberToObject(json, "Th", themeType);
    cJSON_AddNumberToObject(json, "Lng", panelLanguageType);
    cJSON_AddNumberToObject(json, "volt", batarya_volt);


    // Add outputsBuffer to the JSON object
    cJSON *outputnames = cJSON_CreateIntArray(outputsBuffer, numOfOutputs);
    cJSON_AddItemToObject(json, "outNB", outputnames);

    // Add dimsBuffer to the JSON object
    cJSON *dimnames = cJSON_CreateIntArray(dimsBuffer, numOfDims);
    cJSON_AddItemToObject(json, "dimNB", dimnames);

    // Add sensorsBuffer to the JSON object
    cJSON *sensornames = cJSON_CreateIntArray(sensorsBuffer, 5);
    cJSON_AddItemToObject(json, "senNB", sensornames);


    // Fetch outputsBuffer from regs_data
    int buf[16];
    for (int i = 0; i < numOfOutputs; i++) {
        buf[i] = get_outputs() >> i & 0x01; // Get the output state from the bitmask
    }
    cJSON *outputs = cJSON_CreateIntArray(buf, numOfOutputs);
    cJSON_AddItemToObject(json, "outDB", outputs);

    // Fetch dimsBuffer from regs_data
    for (int i = 0; i < numOfDims; i++) {
        buf[i] = get_dimmable_output(i);
    }
    cJSON *dims = cJSON_CreateIntArray(buf, numOfDims);
    cJSON_AddItemToObject(json, "dDB", dims);



        buf[0] = get_analog_input(3);
        buf[1] = get_analog_input(4);
        buf[2] = get_analog_input(0);
        buf[3] = get_analog_input(2);
        buf[4] = get_analog_input(1);

    cJSON *sensors = cJSON_CreateIntArray(buf, numOfSensors);
    cJSON_AddItemToObject(json, "sDB", sensors);


    rgbBuffer[0] = get_rgb_value(0);
    rgbBuffer[1] = get_rgb_value(1);
    rgbBuffer[2] = get_rgb_value(2);
    rgbBuffer[3] = get_rgb_value(3);
    //ESP_LOGI(TAG, "RGB Values: %d, %d, %d, %d", get_rgb_value(0), get_rgb_value(1), get_rgb_value(2), get_rgb_value(3));
    // Add rgbBuffer to the JSON object
    cJSON *rgb = cJSON_CreateIntArray(rgbBuffer, 4);
    cJSON_AddItemToObject(json, "RGBDB", rgb);

    {
        uint8_t sta_mac[6] = {0};
        if (esp_read_mac(sta_mac, ESP_MAC_WIFI_STA) == ESP_OK) {
            char mac_str[24];
            snprintf(mac_str, sizeof(mac_str), "%02X:%02X:%02X:%02X:%02X:%02X",
                     sta_mac[0], sta_mac[1], sta_mac[2], sta_mac[3], sta_mac[4], sta_mac[5]);
            cJSON_AddStringToObject(json, "mac", mac_str);
        }
        const hexnet_io_profile_t *prof = hexnet_io_profile_get();
        if (prof) {
            cJSON_AddNumberToObject(json, "motorEn", prof->motor_enabled);
            cJSON_AddNumberToObject(json, "rgbEn", prof->rgb_enabled);
            cJSON_AddNumberToObject(json, "tankN", prof->num_tanks);
            cJSON_AddNumberToObject(json, "rMask", prof->relay_enable_mask);
            cJSON_AddNumberToObject(json, "dMask", prof->dim_enable_mask);
            cJSON_AddNumberToObject(json, "sMask", prof->sensor_enable_mask);
            cJSON_AddNumberToObject(json, "tMask", prof->tank_enable_mask);
            cJSON_AddNumberToObject(json, "ioLink", prof->link_flags);
        }
        hexnet_io_profile_append_to_json(json, "io");
    }

    // Convert JSON object to string
    char *json_str = cJSON_PrintUnformatted(json);
    //ESP_LOGI("JSON_DATA_PACKET", "%s", json_str);

    // Free the JSON object
    cJSON_Delete(json);

    return json_str; // Caller is responsible for freeing the returned string
}



void show_weather_icon(int index) {
    lv_obj_clear_flag(ui_imgWForecast, LV_OBJ_FLAG_HIDDEN);     /// Flags
    lv_img_set_offset_x(ui_imgWForecast, index );
}


void set_weather_icon(int weather) {
    switch (weather) {
        case WEATHER_SUNNY:
            show_weather_icon(ICON_SUNNY);
            break;
        case WEATHER_PARTLY_SUNNY:
            show_weather_icon(ICON_PARTLY_SUNNY);
            break;
        case WEATHER_THUNDER:
            show_weather_icon(ICON_THUNDER);
            break;
        case WEATHER_RAINY:
            show_weather_icon(ICON_RAINY);
            break;
        case WEATHER_SNOWY:
            show_weather_icon(ICON_SNOWY);
            break;
        case WEATHER_CLOUDY:
            show_weather_icon(ICON_CLOUDY);
            break;
        default:
            ESP_LOGI(TAG, "Unknown weather condition: %d", weather);
            break;
    }
}


// Function to parse BLE data and call the appropriate parsing function
void parse_ble_data(const char* json_data) {
    cJSON* json = cJSON_Parse(json_data);
    if (json == NULL) {
        //ESP_LOGE("PARSE_BLE_DATA", "Invalid JSON data");
        return;
    }

    cJSON* messageType = cJSON_GetObjectItem(json, "MessageType");
    if (messageType == NULL || !cJSON_IsString(messageType)) {
        ESP_LOGE("PARSE_BLE_DATA", "MessageType not found or invalid");
        cJSON_Delete(json);
        return;
    }

    if (strcmp(messageType->valuestring, "Read") == 0) {
        parse_read_data(json);
    } else if (strcmp(messageType->valuestring, "Write") == 0) {
        parse_write_data(json);
    } else if (strcmp(messageType->valuestring, "Configuration") == 0) {
        parse_configuration_data(json);
    } else if (strcmp(messageType->valuestring, "Rules") == 0) {
        parse_rules_data(json);
    } else {
        ESP_LOGE("PARSE_BLE_DATA", "Unknown MessageType: %s", messageType->valuestring);
    }

    cJSON_Delete(json);

}

// Function to check if string contains only English alphabet characters
bool is_english_alphabet(const char* str) {
    for (int i = 0; str[i] != '\0'; i++) {
        char c = str[i];
        // Check if character is not a letter (A-Z, a-z) and not a space
        if (!((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || c == ' ')) {
            return false;
        }
    }
    return true;
}


// Function to convert location name to English alphabet
void convert_location_to_english(const char* input, char* output, size_t output_size) {
    // Simple conversion mapping for common non-English characters
    const char* replacements[][2] = {
        {"ö", "o"}, {"Ö", "O"},
        {"ü", "u"}, {"Ü", "U"},
        {"ä", "a"}, {"Ä", "A"},
        {"å", "a"}, {"Å", "A"},
        {"ç", "c"}, {"Ç", "C"},
        {"ş", "s"}, {"Ş", "S"},
        {"ğ", "g"}, {"Ğ", "G"},
        {"ı", "i"}, {"İ", "I"},
        {"ñ", "n"}, {"Ñ", "N"},
        {"é", "e"}, {"É", "E"},
        {"è", "e"}, {"È", "E"},
        {"ê", "e"}, {"Ê", "E"},
        {"ë", "e"}, {"Ë", "E"},
        {"à", "a"}, {"À", "A"},
        {"á", "a"}, {"Á", "A"},
        {"â", "a"}, {"Â", "A"},
        {"ã", "a"}, {"Ã", "A"},
        {"ô", "o"}, {"Ô", "O"},
        {"ò", "o"}, {"Ò", "O"},
        {"ó", "o"}, {"Ó", "O"},
        {"õ", "o"}, {"Õ", "O"},
        {"ù", "u"}, {"Ù", "U"},
        {"ú", "u"}, {"Ú", "U"},
        {"û", "u"}, {"Û", "U"},
        {"ý", "y"}, {"Ý", "Y"},
        {"ÿ", "y"}, {"Ÿ", "Y"},
        {"ß", "ss"},
        {"æ", "ae"}, {"Æ", "AE"},
        {"œ", "oe"}, {"Œ", "OE"}
    };
    
    strncpy(output, input, output_size - 1);
    output[output_size - 1] = '\0';
    
    // Apply character replacements
    for (int i = 0; i < sizeof(replacements) / sizeof(replacements[0]); i++) {
        char* pos = strstr(output, replacements[i][0]);
        while (pos != NULL) {
            size_t len = strlen(replacements[i][0]);
            size_t replacement_len = strlen(replacements[i][1]);
            
            // Check if replacement fits
            if (strlen(output) - len + replacement_len < output_size) {
                memmove(pos + replacement_len, pos + len, strlen(pos + len) + 1);
                memcpy(pos, replacements[i][1], replacement_len);
                pos = strstr(pos + replacement_len, replacements[i][0]);
            } else {
                break;
            }
        }
    }
}



// Function to parse Read data
void parse_read_data(cJSON* json) {
    cJSON* time = cJSON_GetObjectItem(json, "Time");
    cJSON* date = cJSON_GetObjectItem(json, "Date");
    cJSON* weather = cJSON_GetObjectItem(json, "Weather");
    cJSON* location = cJSON_GetObjectItem(json, "Location");
    cJSON* temper = cJSON_GetObjectItem(json, "Temperature");
    ESP_LOGI("PARSE_READ_DATA", "Parsing Read Data...");

    if (time && cJSON_IsString(time)) {
        ESP_LOGI("PARSE_READ_DATA", "Time: %s", time->valuestring);
    }
    if (date && cJSON_IsString(date)) {
        ESP_LOGI("PARSE_READ_DATA", "Date: %s", date->valuestring);
    }
    if (weather && cJSON_IsString(weather)) {
        ESP_LOGI("PARSE_READ_DATA", "Weather: %s", weather->valuestring);
    }
    if (location && cJSON_IsString(location)) {
        ESP_LOGI("PARSE_READ_DATA", "Location: %s", location->valuestring);
    }

    if (temper && cJSON_IsString(temper)) {
        ESP_LOGI("PARSE_READ_DATA", "Temperature: %s", temper->valuestring);
    }

    // Merge time, date, and batarya_volt
    char merged_str[128];
    lv_label_set_text(ui_lblScreenClock, time->valuestring);
    if(panelLanguageType == 0) {
        // Turkish
        snprintf(merged_str, sizeof(merged_str), "Batarya:  %.2fV  Tarih:  %s   Saat:  %s",batarya_volt, date->valuestring, time->valuestring);
    } else {
        // English  
    snprintf(merged_str, sizeof(merged_str), "Battery:  %.2fV  Date:  %s   Time:  %s",batarya_volt, date->valuestring, time->valuestring);
    }

    // Set the combined value to ui_lblDateAndTime
    lv_label_set_text(ui_lblDateAndTime, merged_str);

    // Process location name - check if it's English alphabet, convert if needed
    char processed_location[64];
    if (location && cJSON_IsString(location)) {
        if (is_english_alphabet(location->valuestring)) {
            // Location is already in English alphabet
            strncpy(processed_location, location->valuestring, sizeof(processed_location) - 1);
            processed_location[sizeof(processed_location) - 1] = '\0';
            ESP_LOGI("PARSE_READ_DATA", "Location (English): %s", processed_location);
        } else {
            // Convert location to English alphabet
            convert_location_to_english(location->valuestring, processed_location, sizeof(processed_location));
            ESP_LOGI("PARSE_READ_DATA", "Location converted from '%s' to '%s'", location->valuestring, processed_location);
        }
    } else {
        strcpy(processed_location, "Unknown");
    }

    snprintf(merged_str, sizeof(merged_str), "%s %s°C", processed_location, temper->valuestring);
    merged_str[sizeof(merged_str) - 1] = '\0'; // Ensure null termination
    lv_label_set_text(ui_lblWeather, merged_str);

    if (weather && cJSON_IsString(weather)) {

        const char *w = weather->valuestring;

        /* ⛈ thunder */
        if (strcasestr(w, "thunder")) {
            set_weather_icon(WEATHER_THUNDER);
        }

        /* 🌧 rain */
        else if (strcasestr(w, "rain")) {
            set_weather_icon(WEATHER_RAINY);
        }

        /* ❄️ snow */
        else if (strcasestr(w, "snow")) {
            set_weather_icon(WEATHER_SNOWY);
        }

        /* ☀️ sun / clear */
        else if (strcasestr(w, "sun")) {
            set_weather_icon(WEATHER_SUNNY);
        }

        /* ☁️ cloud */
        else if (strcasestr(w, "cloud")) {
            set_weather_icon(WEATHER_CLOUDY);
        }

        else {
                    set_weather_icon(WEATHER_PARTLY_SUNNY);
        }
    }


}

typedef struct {
    uint8_t r;
    uint8_t g;
    uint8_t b;
    uint8_t ena;
} rgb_ui_data_t;

static void rgb_ui_update_cb(void *data)
{
    rgb_ui_data_t *d = (rgb_ui_data_t *)data;

    if(d->ena) {
        lv_color_t slc = lv_color_make(d->r, d->g, d->b);
        lv_obj_set_style_bg_color(ui_btnRGBColor, slc, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_add_state(ui_swRGBTurnON, LV_STATE_CHECKED);
        lv_obj_clear_flag(ui_Colorwheel1, LV_OBJ_FLAG_HIDDEN);
    } else {
        lv_color_t slc = lv_color_make(128, 128, 128);
        lv_obj_set_style_bg_color(ui_btnRGBColor, slc, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_clear_state(ui_swRGBTurnON, LV_STATE_CHECKED);
        lv_obj_add_flag(ui_Colorwheel1, LV_OBJ_FLAG_HIDDEN);
    }

    lv_mem_free(d);
}

// Function to parse Write data
void parse_write_data(cJSON* json) {
    cJSON* writeDataType = cJSON_GetObjectItem(json, "writeDataType");
    cJSON* writeNo = cJSON_GetObjectItem(json, "writeNo");
    cJSON* writeData = cJSON_GetObjectItem(json, "writeData");
    uint8_t can_data[8] = {0}; // CAN verisi için buffer

    if (writeDataType && cJSON_IsString(writeDataType)) {
        ESP_LOGI("PARSE_WRITE_DATA", "writeDataType: %s", writeDataType->valuestring);
    }
    if (writeNo && cJSON_IsNumber(writeNo)) {
        ESP_LOGI("PARSE_WRITE_DATA", "writeNo: %d", writeNo->valueint);
    }

    if (writeDataType && cJSON_IsString(writeDataType) && writeNo && cJSON_IsNumber(writeNo)) {
        if (strcmp(writeDataType->valuestring, "Output") == 0) {
            if (writeData && cJSON_IsNumber(writeData)) {
                ESP_LOGI("PARSE_WRITE_DATA", "Output Value: %d", writeData->valueint);
                can_data[0] = (uint8_t)writeNo->valueint;  // İlk byte veri
                can_data[1] = (uint8_t)writeData->valueint;  // İlk byte veri
                send_can_frame(0x720, can_data);  // Output için CAN ID: 0x720
            }
        } else if (strcmp(writeDataType->valuestring, "Dim") == 0) {
            if (writeData && cJSON_IsNumber(writeData)) {
                ESP_LOGI("PARSE_WRITE_DATA", "Dim Value: %d", writeData->valueint);
                can_data[0] = (uint8_t)writeNo->valueint;  // İlk byte veri
                can_data[1] = (uint8_t)writeData->valueint;  // İlk byte veri
                send_can_frame(0x730, can_data);  // Dim için CAN ID: 0x730
            }
        } else if (strcmp(writeDataType->valuestring, "RGB") == 0) {
            cJSON* rgbArray = cJSON_GetObjectItem(json, "writeData");
            if (rgbArray && cJSON_IsArray(rgbArray) && cJSON_GetArraySize(rgbArray) == 4) {
                can_data[selected_R] = (uint8_t)cJSON_GetArrayItem(rgbArray, 0)->valueint; // Red
                can_data[selected_G] = (uint8_t)cJSON_GetArrayItem(rgbArray, 1)->valueint; // Green
                can_data[selected_B] = (uint8_t)cJSON_GetArrayItem(rgbArray, 2)->valueint; // Blue
                can_data[3] = (uint8_t)cJSON_GetArrayItem(rgbArray, 3)->valueint; // Alpha
                ESP_LOGI("PARSE_WRITE_DATA", "RGB Values: R=%d, G=%d, B=%d, A=%d", can_data[0], can_data[1], can_data[2], can_data[3]);
                send_can_frame(0x740, can_data);  // RGB için CAN ID: 0x740
                rgbEna = can_data[3];

                rgb_ui_data_t *d = lv_mem_alloc(sizeof(rgb_ui_data_t));
                d->r = can_data[selected_R];
                d->g = can_data[selected_G];
                d->b = can_data[selected_B];
                d->ena = rgbEna;

                lv_async_call(rgb_ui_update_cb, d);

            } else {
                ESP_LOGE("PARSE_WRITE_DATA", "RGB writeData must be an array of 3 values.");
            }
        }else if (strcmp(writeDataType->valuestring, "Motor") == 0) {
            if (writeData && cJSON_IsNumber(writeData)) {
                ESP_LOGI("PARSE_WRITE_DATA", "Motor Value: %d", writeData->valueint);
                can_data[0] = (uint8_t)writeNo->valueint;  // İlk byte veri
                can_data[1] = (uint8_t)writeData->valueint;  // İlk byte veri
                send_can_frame(0x750, can_data);  // Motor için CAN ID: 0x750
            }
        }else {
            ESP_LOGI("PARSE_WRITE_DATA", "Unknown writeDataType: %s", writeDataType->valuestring);
        }
    }
}

void set_rgb_to_white() {
    uint8_t can_data[8] = {0}; // CAN verisi için buffer
    // Set RGB values to white
    can_data[0] = 255; // Red
    can_data[1] = 255; // Green
    can_data[2] = 255; // Blue
    can_data[3] = rgbEna;   // Unused byte
    send_can_frame(0x740, can_data);  // RGB için CAN ID: 0x740
}

// Timer callback for BLE-triggered restart
static void ble_restart_timer_callback(lv_timer_t * timer)
{
    ESP_LOGI("BLE_RESTART", "BLE restart timer callback called");
    save_panel_settings();
    ESP_LOGI("BLE_RESTART", "save_panel_settings() called");
    lv_timer_del(timer); // Delete the timer after use
}


// Function to parse Configuration data
void parse_configuration_data(cJSON* json) {
    cJSON* numOfOutputs = cJSON_GetObjectItem(json, "numOfOutputs");
    cJSON* outputsNameBuffer = cJSON_GetObjectItem(json, "OutputsNameBuffer");
    cJSON* numOfDims = cJSON_GetObjectItem(json, "numOfDims");
    cJSON* dimsNameBuffer = cJSON_GetObjectItem(json, "DimsNameBuffer");
    cJSON* numOfSensors = cJSON_GetObjectItem(json, "numOfSensors");
    cJSON* sensorsNameBuffer = cJSON_GetObjectItem(json, "SensorsNameBuffer");
    cJSON* rgbEnabled = cJSON_GetObjectItem(json, "RGBEnabled");

    int outputsBuf[16] = {0};
    int dimsBuf[4] = {0};
    int sensorsBuf[5] = {0};

    if (numOfOutputs && cJSON_IsNumber(numOfOutputs)) {
        ESP_LOGI("PARSE_CONFIGURATION_DATA", "numOfOutputs: %d", numOfOutputs->valueint);
    }
    if (outputsNameBuffer && cJSON_IsArray(outputsNameBuffer)) {
        int size = cJSON_GetArraySize(outputsNameBuffer);
        ESP_LOGI("PARSE_CONFIGURATION_DATA", "OutputsNameBuffer:");
        for (int i = 0; i < size && i < 16; i++) {
            cJSON* item = cJSON_GetArrayItem(outputsNameBuffer, i);
            if (cJSON_IsNumber(item)) {
                outputsBuf[i] = item->valueint;
                ESP_LOGI("PARSE_CONFIGURATION_DATA", "  %d", item->valueint);
            }
        }
    }
    if (numOfDims && cJSON_IsNumber(numOfDims)) {
        ESP_LOGI("PARSE_CONFIGURATION_DATA", "numOfDims: %d", numOfDims->valueint);
    }
    if (dimsNameBuffer && cJSON_IsArray(dimsNameBuffer)) {
        int size = cJSON_GetArraySize(dimsNameBuffer);
        ESP_LOGI("PARSE_CONFIGURATION_DATA", "DimsNameBuffer:");
        for (int i = 0; i < size && i < 4; i++) {
            cJSON* item = cJSON_GetArrayItem(dimsNameBuffer, i);
            if (cJSON_IsNumber(item)) {
                dimsBuf[i] = item->valueint;
                ESP_LOGI("PARSE_CONFIGURATION_DATA", "  %d", item->valueint);
            }
        }
    }
    if (numOfSensors && cJSON_IsNumber(numOfSensors)) {
        ESP_LOGI("PARSE_CONFIGURATION_DATA", "numOfSensors: %d", numOfSensors->valueint);
    }
    if (sensorsNameBuffer && cJSON_IsArray(sensorsNameBuffer)) {
        int size = cJSON_GetArraySize(sensorsNameBuffer);
        ESP_LOGI("PARSE_CONFIGURATION_DATA", "SensorsNameBuffer:");
        for (int i = 0; i < size && i < 5; i++) {
            cJSON* item = cJSON_GetArrayItem(sensorsNameBuffer, i);
            if (cJSON_IsNumber(item)) {
                sensorsBuf[i] = item->valueint;
                ESP_LOGI("PARSE_CONFIGURATION_DATA", "  %d", item->valueint);
            }
        }
    }
    if (rgbEnabled && cJSON_IsString(rgbEnabled)) {
        ESP_LOGI("PARSE_CONFIGURATION_DATA", "RGBEnables: %s", rgbEnabled->valuestring);
    }

    /* Theme/language: prefer numeric Th/Lng; string Theme/Language also accepted */
    {
        int theme = panelThemeType;
        int lang = panelLanguageType;
        bool have = false;
        cJSON *th = cJSON_GetObjectItem(json, "Th");
        cJSON *lng = cJSON_GetObjectItem(json, "Lng");
        cJSON *theme_s = cJSON_GetObjectItem(json, "Theme");
        cJSON *lang_s = cJSON_GetObjectItem(json, "Language");
        if (cJSON_IsNumber(th)) {
            theme = th->valueint;
            have = true;
        } else if (cJSON_IsString(theme_s) && theme_s->valuestring) {
            const char *s = theme_s->valuestring;
            if (s[0] == 'a' || s[0] == 'A' || s[0] == 's' || s[0] == 'S' || s[0] == '2') {
                theme = 2; /* auto / system */
                have = true;
            } else if (s[0] == 'd' || s[0] == 'D' || s[0] == 'b' || s[0] == 'B' || s[0] == '1') {
                theme = 1;
                have = true;
            } else if (s[0] == 'l' || s[0] == 'L' || s[0] == 'w' || s[0] == 'W' || s[0] == '0') {
                theme = 0;
                have = true;
            }
        }
        if (cJSON_IsNumber(lng)) {
            lang = lng->valueint;
            have = true;
        } else if (cJSON_IsString(lang_s) && lang_s->valuestring) {
            const char *s = lang_s->valuestring;
            if (s[0] == 'T' || s[0] == 't' || s[0] == '1') {
                lang = 1;
                have = true;
            } else if (s[0] == 'E' || s[0] == 'e' || s[0] == '0') {
                lang = 0;
                have = true;
            }
        }
        if (have) {
            apply_theme_language_from_io(theme, lang);
            push_theme_language_to_io(); /* mirror to IO SoT when phone talks to Display BLE */
        }
    }

    (void)hexnet_io_profile_apply_json(json, HEXNET_IO_LINK_DISPLAY);
    hexnet_io_profile_sync_legacy_globals();
    fromBleConfig = 1;
    ESP_LOGI("BLE_CONFIG", "Creating restart timer for BLE configuration");
    // Create a one-shot timer to show restart panel in LVGL task context
    lv_timer_t * restart_timer = lv_timer_create(ble_restart_timer_callback, 100, NULL);
    if (restart_timer == NULL) {
        ESP_LOGE("BLE_CONFIG", "Failed to create restart timer!");
    } else {
        lv_timer_set_repeat_count(restart_timer, 1); // Run only once
        ESP_LOGI("BLE_CONFIG", "Restart timer created successfully");
    }
}
// Function to parse Rules data
void parse_rules_data(cJSON* json) {
    cJSON* numOfRules = cJSON_GetObjectItem(json, "NumOfRules");
    cJSON* rules = cJSON_GetObjectItem(json, "Rules");
    cJSON* numOfNotifications = cJSON_GetObjectItem(json, "NumOfNotifications");
    cJSON* notifications = cJSON_GetObjectItem(json, "Notifications");

    if (numOfRules && cJSON_IsNumber(numOfRules)) {
        ESP_LOGI("PARSE_RULES_DATA", "NumOfRules: %d", numOfRules->valueint);
    }
    if (rules && cJSON_IsObject(rules)) {
        ESP_LOGI("PARSE_RULES_DATA", "Rules:");
        cJSON* rule;
        cJSON_ArrayForEach(rule, rules) {
            if (cJSON_IsString(rule)) {
                ESP_LOGI("PARSE_RULES_DATA", "  %s: %s", rule->string, rule->valuestring);
            }
        }
    }
    if (numOfNotifications && cJSON_IsNumber(numOfNotifications)) {
        ESP_LOGI("PARSE_RULES_DATA", "NumOfNotifications: %d", numOfNotifications->valueint);
    }
    if (notifications && cJSON_IsObject(notifications)) {
        ESP_LOGI("PARSE_RULES_DATA", "Notifications:");
        cJSON* notification;
        cJSON_ArrayForEach(notification, notifications) {
            if (cJSON_IsString(notification)) {
                ESP_LOGI("PARSE_RULES_DATA", "  %s: %s", notification->string, notification->valuestring);
            }
        }
    }
}

//######################################################################################################################


  
//############################ NVS FUNCTIONS #########################################################################
// Function to save configuration data to NVS
void save_panel_configuration_to_nvs(int totalOutps, int buffer1[16], int totalSensors, int buffer2[5], int totalDims, int buffer3[8]) {
    // Ensure the values do not exceed the maximum allowed sizes
    if (totalOutps > 16) {
        totalOutps = 16;
    }
    if (totalSensors > 5) {
        totalSensors = 5;
    }
    if (totalDims > 8) {
        totalDims = 8;
    }

    // Save totalOutps to NVS
    nvs_write_int("numOfOutputs", totalOutps);

    // Save buffer1 to NVS (0 = kanal pasif; 1..18 = tip)
    for (int i = 0; i < 16; i++) {
        if (buffer1[i] < 0 || buffer1[i] > 18) {
            buffer1[i] = 0;
        }
        char key[16];
        snprintf(key, sizeof(key), "outBuf%d", i);
        nvs_write_int(key, buffer1[i]);
    }

    // Save totalSensors to NVS
    nvs_write_int("numSens", totalSensors);

    // Save buffer2 to NVS
    for (int i = 0; i < 5; i++) {
        if (buffer2[i] < 0 || buffer2[i] > 1) {
            buffer2[i] = 0; // Set to default value if out of range
        }
        char key[16];
        snprintf(key, sizeof(key), "sensBuf%d", i);
        nvs_write_int(key, buffer2[i]);
    }

    // Save totalDims to NVS
    nvs_write_int("numDims", totalDims);

    // Save buffer3 to NVS
    for (int i = 0; i < 8; i++) {
        if (buffer3[i] < 0 || buffer3[i] > 8) {
            buffer3[i] = 0; // Set to default value if out of range
        }
        char key[16];
        snprintf(key, sizeof(key), "dimsBuf%d", i);
        nvs_write_int(key, buffer3[i]);
    }



}



void save_theme_configuration_to_nvs(int16_t* themeType, uint16_t* wallpaperEnabled, uint16_t* wallpaperTimeIndex, int16_t* lngType) {
    (void)themeType;
    (void)wallpaperEnabled;
    (void)wallpaperTimeIndex;
    (void)lngType;
    /* Tek open/commit — 5x nvs_write_int flash cache'i 5 kez kilitliyordu (IWDT). */
    nvs_handle_t h;
    esp_err_t err = nvs_open("storage", NVS_READWRITE, &h);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "theme NVS open: %s", esp_err_to_name(err));
        return;
    }
    (void)nvs_set_i32(h, "thmTyp", panelThemeType);
    (void)nvs_set_i32(h, "wallpEn", panelWallpaperEnable);
    (void)nvs_set_i32(h, "wllTimI", panelWallpaperTime);
    (void)nvs_set_i32(h, "lngTyp", panelLanguageType);
    (void)nvs_set_i32(h, "accIdx", panelAccentIdx);
    err = nvs_commit(h);
    nvs_close(h);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "theme NVS commit: %s", esp_err_to_name(err));
    }
}



// Debug function to print parameters to the screen
void debug_print_configuration(int totalOutpts, int buffer1[16], int totalSensors, int buffer2[5], int totalDims, int buffer3[8]) {
    ESP_LOGI("DEBUG", "totalOutpts: %d", totalOutpts);
    for (int i = 0; i < 16; i++) {
        ESP_LOGI("DEBUG", "buffer1[%d]: %d", i, buffer1[i]);
    }
    ESP_LOGI("DEBUG", "totalSensors: %d", totalSensors);
    for (int i = 0; i < 5; i++) {
        ESP_LOGI("DEBUG", "buffer2[%d]: %d", i, buffer2[i]);
    }
    ESP_LOGI("DEBUG", "totalDims: %d", totalDims);
    for (int i = 0; i < 8; i++) {
        ESP_LOGI("DEBUG", "buffer3[%d]: %d", i, buffer3[i]);
    }
}

// Function to read configuration data from NVS
void load_panel_configuration_from_nvs(int *totalOutpts, int buffer1[16], int *totalSensors, int buffer2[5], int *totalDims, int buffer3[8]) {
    // Read totalOutpts from NVS
    if (nvs_read_int("numOfOutputs", totalOutpts) != ESP_OK || *totalOutpts < 0 || *totalOutpts > 16) {
        *totalOutpts = 4; // Set to default value if out of range
    }

    // Read buffer1 from NVS
    for (int i = 0; i < 16; i++) {
        char key[16];
        snprintf(key, sizeof(key), "outBuf%d", i);
        if (nvs_read_int(key, &buffer1[i]) != ESP_OK || buffer1[i] < 1 || buffer1[i] > 18) {
            buffer1[i] = 1; // Set to default value if out of range
        }
    }

    // Read totalSensors from NVS
    if (nvs_read_int("numSens", totalSensors) != ESP_OK || *totalSensors < 0 || *totalSensors > 5) {
        *totalSensors = 1; // Set to default value if out of range
    }

    // Read buffer2 from NVS
    for (int i = 0; i < 5; i++) {
        char key[16];
        snprintf(key, sizeof(key), "sensBuf%d", i);
        if (nvs_read_int(key, &buffer2[i]) != ESP_OK || buffer2[i] < 0 || buffer2[i] > 1) {
            buffer2[i] = 1; // Set to default value if out of range
        }
    }

    // Read totalSensors from NVS
    if (nvs_read_int("numDims", totalDims) != ESP_OK || *totalDims < 0 || *totalDims > 8) {
        *totalDims = 1; // Set to default value if out of range
    }

    // Read buffer3 from NVS. Kanallar 5-8 kayitli degilse kapali kalir.
    for (int i = 0; i < 8; i++) {
        char key[16];
        snprintf(key, sizeof(key), "dimsBuf%d", i);
        if (nvs_read_int(key, &buffer3[i]) != ESP_OK || buffer3[i] < 0 || buffer3[i] > 8) {
            buffer3[i] = (i < 4) ? 1 : 0;
        }
    }



    // Read totalSensors from NVS
    if (nvs_read_int("selR", &selected_R) != ESP_OK || selected_R < 0 || selected_R > 2) {
        selected_R = 0; // Set to default value if out of range
    }

    // Read totalSensors from NVS
    if (nvs_read_int("selG", &selected_G) != ESP_OK || selected_G < 0 || selected_G > 2) {
        selected_G = 1; // Set to default value if out of range
    }
    // Read totalSensors from NVS
    if (nvs_read_int("selB", &selected_B) != ESP_OK || selected_B < 0 || selected_B > 2) {
        selected_B = 2; // Set to default value if out of range
    }
    ESP_LOGI(TAG, "-------------------Loaded Panel Configuration: R=%d, G=%d, B=%d",selected_R, selected_G, selected_B);
}



// Load the theme settings from NVS
void load_theme_configuration_from_nvs(int* themeType, int* wallpaperEnabled, int* wallpaperTimeIndex, int* panelLanguageType ) {

    // Read themeType from NVS
    if (nvs_read_int("thmTyp", themeType) != ESP_OK || *themeType < 0 || *themeType > 2) {
        *themeType = 2; /* default: system (follow IO SoT) */
    }


    // Read wallpaperEnabled from NVS
    if (nvs_read_int("wallpEn", wallpaperEnabled) != ESP_OK || *wallpaperEnabled < 0 || *wallpaperEnabled > 1) {
        *wallpaperEnabled = 0; // Set to default value if out of range
    }

    // Read wallpaper timeout (seconds) from NVS
    if (nvs_read_int("wllTimI", wallpaperTimeIndex) != ESP_OK || *wallpaperTimeIndex < 0 || *wallpaperTimeIndex > 600) {
        *wallpaperTimeIndex = 30; // Set to default value if out of range
    }

    // Read wallpaperEnabled from NVS
    if (nvs_read_int("lngTyp", panelLanguageType) != ESP_OK || *panelLanguageType < 0 || *panelLanguageType > 3) {
        *panelLanguageType = 2; /* default: system (follow IO SoT) */
    }

    {
        int acc = 0;
        if (nvs_read_int("accIdx", &acc) != ESP_OK || acc < 0 || acc >= VANGO_ACCENT_N) {
            acc = 0;
        }
        panelAccentIdx = acc;
    }

    // Log the loaded configuration
    ESP_LOGI(TAG, "Loaded Theme Configuration: Theme=%d, WallpaperEnabled=%d, WallpaperTimeIndex=%d, LanguageType=%d, Accent=%d",
             *themeType, *wallpaperEnabled, *wallpaperTimeIndex, *panelLanguageType, panelAccentIdx);

}


// Function to check switches and get corresponding dropdown values
void check_switches_and_get_dropdown_values() {
    // Reset the outputsBuffer
    memset(outputsBuffer, 0, sizeof(outputsBuffer));

    numOfOutputs = 0; // Initialize numOfOutputs
    // Apply numOfOutputs and outputsBuffer to swO1-swO16 and cbxO1-cbxO16
    lv_obj_t* switches[16] = {ui_swO1, ui_swO2, ui_swO3, ui_swO4, ui_swO5, ui_swO6, ui_swO7, ui_swO8, ui_swO9, ui_swO10, ui_swO11, ui_swO12, ui_swO13, ui_swO14, ui_swO15, ui_swO16};
    lv_obj_t* dropdowns[16] = {ui_cbxO1, ui_cbxO2, ui_cbxO3, ui_cbxO4, ui_cbxO5, ui_cbxO6, ui_cbxO7, ui_cbxO8, ui_cbxO9, ui_cbxO10, ui_cbxO11, ui_cbxO12, ui_cbxO13, ui_cbxO14, ui_cbxO15, ui_cbxO16};


    for (int i = 0; i < 16; i++) {
        if (lv_obj_has_state(switches[i], LV_STATE_CHECKED)) {
            outputsBuffer[i] = 1 + lv_dropdown_get_selected(dropdowns[i]);
            numOfOutputs++; // Increment numOfOutputs for each checked switch
            ESP_LOGI("SWITCH_CHECK", "Switch %d is checked. Dropdown value index: %d", i + 1, outputsBuffer[i]);
        } else {
            outputsBuffer[i] = 0; // Indicate that the switch is not checked
        }
    }

}


static bool has_duplicate(int a, int b, int c)
{
    return (a == b) || (a == c) || (b == c);
}

static const int default_values[3] = {0, 1, 2};
// Function to check switches and get corresponding dropdown values
void check_switches_and_get_dropdown_values_for_dims() {
    // Reset the outputsBuffer
    memset(dimsBuffer, 0, 4 * sizeof(dimsBuffer[0]));

    numOfDims = 0; // Initialize numOfOutputs
    // Apply numOfOutputs and outputsBuffer to swO1-swO16 and cbxO1-cbxO16
    lv_obj_t* dimcheckboxes[4] = {ui_swDim1, ui_swDim2, ui_swDim3, ui_swDim4};
    lv_obj_t* dimdropdowns[4] = {ui_cbxDim1, ui_cbxDim2, ui_cbxDim3, ui_cbxDim4};
    lv_obj_t* rgbdropdowns[3] = {ui_cbRSelect, ui_cbGSelect, ui_cbBSelect};

    for (int i = 0; i < 4; i++) {
        if (lv_obj_has_state(dimcheckboxes[i], LV_STATE_CHECKED)) {
            dimsBuffer[i] = 1 + lv_dropdown_get_selected(dimdropdowns[i]);
            numOfDims++; // Increment numOfOutputs for each checked switch
            ESP_LOGI("SWITCH_CHECK", "Switch %d is checked. Dropdown value index: %d", i + 1, dimsBuffer[i]);
        } else {
            dimsBuffer[i] = 0; // Indicate that the switch is not checked
        }
    }

    selected_R = lv_dropdown_get_selected(ui_cbRSelect);
    selected_G = lv_dropdown_get_selected(ui_cbGSelect);
    selected_B = lv_dropdown_get_selected(ui_cbBSelect);

    if(has_duplicate(selected_R, selected_G, selected_B))
    {
        selected_R = default_values[0];
        selected_G = default_values[1];
        selected_B = default_values[2];
    }

    nvs_write_int("selR", selected_R);
    nvs_write_int("selG", selected_G);
    nvs_write_int("selB", selected_B);
    ESP_LOGI(TAG, "-------------------2222Loaded Panel Configuration: R=%d, G=%d, B=%d",selected_R, selected_G, selected_B);
}


// Function to check the state of the first 5 switches and update sensorsBuffer
void check_sensors_and_update_buffer() {

    // Reset the sensorsBufferd
    memset(sensorsBuffer, 0, sizeof(sensorsBuffer));

    numOfSensors = 0; // Initialize numOfSensors

    // lv_obj_t* switches[5] = {ui_Checkbox1, ui_Checkbox2, ui_Checkbox3, ui_Checkbox4, ui_Checkbox5};

    // for (int i = 0; i < 5; i++) {
    //     if (lv_obj_has_state(switches[i], LV_STATE_CHECKED)) {
    //         sensorsBuffer[i] = 1; // Indicate that the switch is checked
    //         numOfSensors++; // Increment numOfSensors for each checked switch
    //     } else {
    //         sensorsBuffer[i] = 0; // Indicate that the switch is not checked
    //     }
    //     ESP_LOGI("SWITCH_CHECK", "Switch %d is checked. Value: %d", i + 1, sensorsBuffer[i]);
    // }

    // ESP_LOGI("SWITCH_CHECK", "Total number of checked switches: %d", numOfSensors);
}

int SaveConfigsCounter = 0; // Counter for save configs bar
lv_obj_t* current_progress_bar = NULL; // Global reference to the current progress bar
static void save_configsbar_timer(lv_timer_t * timer)
{
    if (current_progress_bar != NULL) {
        lv_bar_set_value(current_progress_bar, SaveConfigsCounter, LV_ANIM_OFF);  // 0 → 100 in steps of 3
    }
    SaveConfigsCounter += 3; // Increment the counter by 3
    if(SaveConfigsCounter >= 100) {
        check_switches_and_get_dropdown_values();
        check_sensors_and_update_buffer();
        check_switches_and_get_dropdown_values_for_dims();
        // Save the panel settings to NVS
        if(fromBleConfig == 1) {
            ESP_LOGI(TAG, "##### Saving Panel Settings from BLE Configuration #####");
            
            fromBleConfig = 0; // Reset the flag
        } else {
            ESP_LOGI(TAG, "##### Saving Panel Settings from UI Configuration #####");
            save_panel_configuration_to_nvs(numOfOutputs, outputsBuffer, numOfSensors, sensorsBuffer, numOfDims, dimsBuffer);
        }
        ESP_LOGI(TAG, "##### Panel Settings Saved Successfully! #####");
        esp_restart();
    }
}

void save_panel_settings()
{
    // Create restart panel as overlay on current screen
    lv_obj_t * current_screen = lv_scr_act();
    if (current_screen == NULL) {
        ESP_LOGE("SAVE_PANEL", "No active screen!");
        return;
    }
    
    ESP_LOGI("SAVE_PANEL", "Creating restart panel overlay on current screen");
    
    // Create the restart panel as a child of the current screen
    lv_obj_t * restart_panel = lv_obj_create(current_screen);
    lv_obj_set_width(restart_panel, 545);
    lv_obj_set_height(restart_panel, 125);
    lv_obj_set_align(restart_panel, LV_ALIGN_CENTER);
    lv_obj_clear_flag(restart_panel, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_color(restart_panel, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(restart_panel, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_color(restart_panel, lv_color_hex(0xFF0000), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_opa(restart_panel, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_color(restart_panel, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_opa(restart_panel, 50, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(restart_panel, 1, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_spread(restart_panel, 1, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_ofs_x(restart_panel, 5, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_ofs_y(restart_panel, 5, LV_PART_MAIN | LV_STATE_DEFAULT);
    
    // Create the text labels
    lv_obj_t * label1 = lv_label_create(restart_panel);
    lv_obj_set_align(label1, LV_ALIGN_TOP_MID);
    lv_obj_set_y(label1, -20);
    lv_label_set_text(label1, "Device will restart");
    lv_obj_set_style_text_color(label1, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(label1, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(label1, &lv_font_montserrat_22, LV_PART_MAIN | LV_STATE_DEFAULT);
    
    lv_obj_t * label2 = lv_label_create(restart_panel);
    lv_obj_set_align(label2, LV_ALIGN_CENTER);
    lv_label_set_text(label2, "Please wait...");
    lv_obj_set_style_text_color(label2, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(label2, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(label2, &lv_font_montserrat_22, LV_PART_MAIN | LV_STATE_DEFAULT);
    
    // Create progress bar
    lv_obj_t * progress_bar = lv_bar_create(restart_panel);
    lv_obj_set_width(progress_bar, 173);
    lv_obj_set_height(progress_bar, 10);
    lv_obj_set_align(progress_bar, LV_ALIGN_BOTTOM_MID);
    lv_obj_set_y(progress_bar, -10);
    lv_bar_set_range(progress_bar, 0, 100);
    lv_bar_set_value(progress_bar, 0, LV_ANIM_OFF);
    
    // Set the global progress bar reference for the timer
    current_progress_bar = progress_bar;
    
    // Reset counter for new restart
    SaveConfigsCounter = 0;
    
    // Move panel to foreground
    lv_obj_move_foreground(restart_panel);
    
    // Force LVGL to refresh the display
    lv_refr_now(NULL);
    
    // Create timer with the new progress bar
    lv_timer_t * initTim = lv_timer_create(save_configsbar_timer, 100, NULL);
    ESP_LOGI("SAVE_PANEL", "Restart panel overlay created and visible");
}

void save_theme_settings()
{

    uint16_t selected_index = 0;
    static char selected_text[32];  // Buffer to store text

    selected_index = lv_roller_get_selected(ui_rlrTime);
    // Get the selected item text
    lv_roller_get_selected_str(ui_rlrTime, selected_text, sizeof(selected_text));
    // Check if switch is enabled (ON)
    if (lv_obj_has_state(ui_swEnableWallpaper, LV_STATE_CHECKED)) {
        // Get the selected roller item index
        panelWallpaperEnable = 1;  // Enable wallpaper
        // Print the selected roller item
        ESP_LOGI(TAG, "Wallpaper Enabled, Selected Time: %s-----index = %d", selected_text, selected_index);
    } else {
        panelWallpaperEnable = 0;  // Disable wallpaper
        ESP_LOGI(TAG, "Wallpaper Disabled");
    }

    // Set panelWallpaperTime based on the selected index
    switch (selected_index) {
        case 0: { panelWallpaperTime = 30;  break;}
        case 1: { panelWallpaperTime = 60;  break;}
        case 2: { panelWallpaperTime = 120; break;}
        case 3: { panelWallpaperTime = 300; break;}
        case 4: { panelWallpaperTime = 600; break;}
        default:
            {
                ESP_LOGW(TAG, "Invalid roller index: %d, setting default to 30", selected_index);
                panelWallpaperTime = 30;  // Default if index is out of range
                break;
            }
    }


    lv_dropdown_get_selected_str(ui_dbLanguage, selected_text, sizeof(selected_text));

    if(strcmp(selected_text, "TURKCE") == 0)
    {
        panelLanguageType = 1;  // Turkish
        apply_language_settings();
    }
    else if(strcmp(selected_text, "ENGLISH") == 0)
    {
        panelLanguageType = 0;  // English
        apply_language_settings();
        
    }
    else
    {
        panelLanguageType = 0;  // Default to English
        apply_language_settings();
    }
    
    
    ESP_LOGI(TAG, "Wallpaper Enabled, Selected Time: %s-----index = %d panelThemeType =%d panelWallpaperEnable =%d panelWallpaperTime =%d panelLanguageType =%d", selected_text, selected_index,
             panelThemeType, panelWallpaperEnable, panelWallpaperTime, panelLanguageType);
    persist_theme_settings();
}

/*
 * NVS asla LVGL / esp_timer callback'inden yazilmamali
 * (S3: spi_flash cache assert / Interrupt WDT → reset).
 * Tema/accent seciminde UI "reset" gibi gorunuyordu.
 */
static QueueHandle_t s_theme_nvs_q;
static TaskHandle_t s_theme_nvs_task;

static void theme_nvs_task(void *arg)
{
    (void)arg;
    uint32_t token;
    for (;;) {
        if (xQueueReceive(s_theme_nvs_q, &token, portMAX_DELAY) != pdTRUE) {
            continue;
        }
        vTaskDelay(pdMS_TO_TICKS(80));
        while (xQueueReceive(s_theme_nvs_q, &token, 0) == pdTRUE) {
            /* coalesce */
        }
        save_theme_configuration_to_nvs((int16_t *)&panelThemeType,
                                        (uint16_t *)&panelWallpaperEnable,
                                        (uint16_t *)&panelWallpaperTime,
                                        (int16_t *)&panelLanguageType);
        ESP_LOGI(TAG, "Persisted theme/lang (task): theme=%d lang=%d wall=%d time=%d acc=%d",
                 panelThemeType, panelLanguageType, panelWallpaperEnable, panelWallpaperTime,
                 panelAccentIdx);
    }
}

void persist_theme_settings(void)
{
    if (!s_theme_nvs_q) {
        s_theme_nvs_q = xQueueCreate(1, sizeof(uint32_t));
    }
    if (!s_theme_nvs_task && s_theme_nvs_q) {
        BaseType_t ok = xTaskCreate(theme_nvs_task, "theme_nvs", 3072, NULL, 3, &s_theme_nvs_task);
        if (ok != pdPASS) {
            s_theme_nvs_task = NULL;
            ESP_LOGE(TAG, "theme NVS task create failed — sync fallback");
            save_theme_configuration_to_nvs((int16_t *)&panelThemeType,
                                            (uint16_t *)&panelWallpaperEnable,
                                            (uint16_t *)&panelWallpaperTime,
                                            (int16_t *)&panelLanguageType);
            return;
        }
    }
    if (s_theme_nvs_q) {
        uint32_t token = 1;
        (void)xQueueOverwrite(s_theme_nvs_q, &token);
    }
}

void sync_theme_widgets_from_globals(void)
{
    /* Dropdown options: "TURKCE\\nENGLISH" → index 0=TR, 1=EN */
    if (ui_dbLanguage) {
        lv_dropdown_set_selected(ui_dbLanguage, panelLanguageType == 1 ? 0 : 1);
    }
    apply_theme_settings();
}

static void push_theme_language_to_io(void)
{
    /* Only manual locks write IO SoT. 0xFF = no change for that field. */
    uint8_t can_data[8] = {0};
    const int push_th = (panelThemeType == 0 || panelThemeType == 1);
    const int push_lng = (panelLanguageType == 0 || panelLanguageType == 1);
    if (!push_th && !push_lng) {
        ESP_LOGI(TAG, "Skip 0x760 — theme/lang both system (follow IO)");
        return;
    }
    can_data[0] = push_th ? (uint8_t)panelThemeType : 0xFFu;
    can_data[1] = push_lng ? (uint8_t)panelLanguageType : 0xFFu;
    if (push_th) {
        io_sot_theme = panelThemeType;
    }
    if (push_lng) {
        io_sot_language = panelLanguageType;
    }
    ESP_LOGI(TAG, "TX 0x760 theme/lang Th=%d Lng=%d (mode Th=%d Lng=%d)",
             (int)can_data[0], (int)can_data[1], panelThemeType, panelLanguageType);
    send_can_frame(VANGO_CAN_FRAME_THEME_LANG, can_data);
}

/** Apply legacy screen colors without clobbering panelThemeType (esp. system=2). */
static void apply_legacy_theme_visual(void)
{
    const int saved = panelThemeType;
    if (vango_theme_dark()) {
        my_btnBlackThemeFunc();
    } else {
        my_btnThemeWhiteFunc();
    }
    panelThemeType = saved;
}

void commit_theme_language_settings(void)
{
    sync_theme_widgets_from_globals();
    apply_legacy_theme_visual();
    apply_language_settings();
    persist_theme_settings();
    push_theme_language_to_io();
}

void apply_theme_language_from_io(int theme, int language)
{
    /* CAN RX — update IO SoT cache; apply UI only when local mode is system */
    bool sot_changed = false;
    if (theme == 0 || theme == 1) {
        if (io_sot_theme != theme) {
            io_sot_theme = theme;
            sot_changed = true;
        }
    }
    if (language == 0 || language == 1) {
        if (io_sot_language != language) {
            io_sot_language = language;
            sot_changed = true;
        }
    }

    const bool apply_th = (panelThemeType == 2);
    const bool apply_lng = (panelLanguageType == 2);
    if (!apply_th && !apply_lng) {
        if (sot_changed) {
            ESP_LOGI(TAG, "IO SoT updated Th=%d Lng=%d (local locked — UI unchanged)",
                     io_sot_theme, io_sot_language);
        }
        return;
    }
    if (!sot_changed && !s_pending_io_prefs) {
        return;
    }
    s_pending_io_theme = io_sot_theme;
    s_pending_io_lang = io_sot_language;
    s_pending_io_prefs = true;
    ESP_LOGI(TAG, "Theme/lang queued from IO SoT: Th=%d Lng=%d (local system mode)",
             io_sot_theme, io_sot_language);
}

static void flush_pending_theme_language_from_io(void)
{
    if (!s_pending_io_prefs) {
        return;
    }
    s_pending_io_prefs = false;
    /* Modes stay system(2) or locked — only visuals/strings follow SoT */
    apply_legacy_theme_visual();
    sync_theme_widgets_from_globals();
    apply_language_settings();
    ESP_LOGI(TAG, "Theme/lang UI applied from IO SoT: eff Th=%d Lng=%d (mode Th=%d Lng=%d)",
             io_sot_theme, io_sot_language, panelThemeType, panelLanguageType);
}


void apply_theme_settings()
{
    if (!ui_swEnableWallpaper || !ui_rlrTime) {
        return;
    }

    // Apply theme enabled status to the switch
    if (panelWallpaperEnable) {
        lv_obj_add_state(ui_swEnableWallpaper, LV_STATE_CHECKED);
        lv_obj_clear_flag(ui_rlrTime, LV_OBJ_FLAG_HIDDEN);     /// Flags
    } else {
        lv_obj_clear_state(ui_swEnableWallpaper, LV_STATE_CHECKED);
        lv_obj_add_flag(ui_rlrTime, LV_OBJ_FLAG_HIDDEN);     /// Flags
    }

    // Map panelWallpaperTime to the roller index
    uint16_t roller_index = 0;

    switch (panelWallpaperTime) {
        case 30:  roller_index = 0; break;
        case 60:  roller_index = 1; break;
        case 120: roller_index = 2; break;
        case 300: roller_index = 3; break;
        case 600: roller_index = 4; break;
        default:
            ESP_LOGW(TAG, "Invalid panelWallpaperTime: %d, setting default to 30s", panelWallpaperTime);
            roller_index = 0;  // Default to first option
            break;
    }

    // Set roller selection
    lv_roller_set_selected(ui_rlrTime, roller_index, LV_ANIM_OFF);

    // Log applied settings
    ESP_LOGI(TAG, " ############################Applied Theme Settings:panelThemeType = %d panelWallpaperEnable=%d, WallpaperTimeIndex=%d",panelThemeType, panelWallpaperEnable, roller_index);

}


void apply_language_settings()
{
    if (!ui_scrMain) {
        vango_dash_on_prefs_changed();
        return;
    }
    /* Effective lang: 0=EN, 1=TR, 3=DE (system follows IO SoT). */
    const int lang = vango_lang();
    if (lang == 0) {
        lv_label_set_text(ui_lblPanelSettingsB, "PANEL\nSETTINGS");
        lv_label_set_text(ui_lblThemeSettingsB, "THEME\nSETTINGS");
        lv_label_set_text(ui_lblConnectionSettingsB, "CONNECTION\nSETTINGS");
        lv_label_set_text(ui_lblDimmableOutputsB, "DIM\nOUTPUTS");
        lv_label_set_text(ui_lblSensorsB, "SENSORS");
        lv_label_set_text(ui_lblWaterLevelsB, "WATER\nLEVELS");
        lv_label_set_text(ui_lblLock6, "Back");
        lv_label_set_text(ui_lblLock5, "Back");
        lv_label_set_text(ui_Label2, "Apply");
        lv_label_set_text(ui_lblLock2, "Back");
        lv_label_set_text(ui_lblLock4, "Back");
        lv_label_set_text(ui_lblLock8, "Back");
        lv_label_set_text(ui_lblLock3, "Back");
        lv_label_set_text(ui_lblSettings2, "Save");
        lv_label_set_text(ui_lblLock3, "Save");
        lv_label_set_text(ui_lblSelectTheme, "Select Theme: ");
        lv_label_set_text(ui_lblWallpaper, "Wallpaper: ");
        lv_label_set_text(ui_lblGrup1, "Clean Water");
        lv_label_set_text(ui_lblGrup2, "Dirty Water");
        lv_label_set_text(ui_lblPnlGrup1Sicaklik1, "\nTemperature");
        lv_label_set_text(ui_lblPnlGrup1Sicaklik2, "\nHumidity");
        lv_label_set_text(ui_Label12, "SAVING CONFIGURATIONS...\n   DEVICE WILL RESTART");
        lv_label_set_text(ui_lblPanelSettings, "PANEL SETTINGS");
        lv_label_set_text(ui_lblDimmableOutputs, "Dimmable Outputs");
        lv_label_set_text(ui_lblUnderArcTemperature1, "Temperature");
        lv_label_set_text(ui_lblUnderArcTemperature2, "Humidity");
        lv_label_set_text(ui_lblUnderArcWater1, "Clean Water");
        lv_label_set_text(ui_lblUnderArcWater2, "Dirty Water");

        for (int i = 0; i < numOfOutputs; i++) {
            lv_label_set_text(lblIO[i], lblBtnNames[outputsBuffer[i] - 1]);
        }
        for (int i = 0; i < numOfDims && i < 4; i++) {
            lv_label_set_text(lblDims[i], lblDimBtnNames[dimsBuffer[i] - 1]);
        }
        set_language_for_dropdowns();

    } else if (lang == 1) {
        lv_label_set_text(ui_lblPanelSettingsB, "PANEL\nAYARLARI");
        lv_label_set_text(ui_lblThemeSettingsB, "TEMA\nAYARLARI");
        lv_label_set_text(ui_lblConnectionSettingsB, "BAGLANTI\nAYARLARI");
        lv_label_set_text(ui_lblDimmableOutputsB, "DIM\nCIKISLARI");
        lv_label_set_text(ui_lblSensorsB, "SENSORLER");
        lv_label_set_text(ui_lblWaterLevelsB, "SU\nSEVIYELERI");
        lv_label_set_text(ui_lblLock6, "Geri");
        lv_label_set_text(ui_lblLock5, "Geri");
        lv_label_set_text(ui_Label2, "Uygula");
        lv_label_set_text(ui_lblLock2, "Geri");
        lv_label_set_text(ui_lblLock4, "Geri");
        lv_label_set_text(ui_lblLock8, "Geri");
        lv_label_set_text(ui_lblLock3, "Geri");
        lv_label_set_text(ui_lblSettings2, "Kaydet");
        lv_label_set_text(ui_lblLock3, "Kaydet");
        lv_label_set_text(ui_lblSelectTheme, "Tema Sec: ");
        lv_label_set_text(ui_lblWallpaper, "Duvar Kagidi: ");
        lv_label_set_text(ui_lblGrup1, "Temiz Su");
        lv_label_set_text(ui_lblGrup2, "Kirli Su");
        lv_label_set_text(ui_lblPnlGrup1Sicaklik1, "\n Sicaklik");
        lv_label_set_text(ui_lblPnlGrup1Sicaklik2, "\n   Nem");
        lv_label_set_text(ui_Label12, "AYARLAR KAYDEDILIYOR...\nCIHAZ YENIDEN BASLAYACAK");
        lv_label_set_text(ui_lblPanelSettings, "PANEL AYARLARI");
        lv_label_set_text(ui_lblDimmableOutputs, "Dim Cikislari");
        lv_label_set_text(ui_lblUnderArcTemperature1, "Sicaklik");
        lv_label_set_text(ui_lblUnderArcTemperature2, "Nem");
        lv_label_set_text(ui_lblUnderArcWater1, "Temiz Su");
        lv_label_set_text(ui_lblUnderArcWater2, "Kirli Su");

        for (int i = 0; i < numOfOutputs; i++) {
            lv_label_set_text(lblIO[i], lblBtnNames_TR[outputsBuffer[i] - 1]);
        }
        for (int i = 0; i < numOfDims && i < 4; i++) {
            lv_label_set_text(lblDims[i], lblDimBtnNames_TR[dimsBuffer[i] - 1]);
        }
        set_language_for_dropdowns_TR();

    } else if (lang == 3) {
        lv_label_set_text(ui_lblPanelSettingsB, "PANEL\nEINSTELL.");
        lv_label_set_text(ui_lblThemeSettingsB, "THEMA\nEINSTELL.");
        lv_label_set_text(ui_lblConnectionSettingsB, "VERBINDUNG\nEINSTELL.");
        lv_label_set_text(ui_lblDimmableOutputsB, "DIM\nAUSGAENGE");
        lv_label_set_text(ui_lblSensorsB, "SENSOREN");
        lv_label_set_text(ui_lblWaterLevelsB, "WASSER\nSTAND");
        lv_label_set_text(ui_lblLock6, "Zurueck");
        lv_label_set_text(ui_lblLock5, "Zurueck");
        lv_label_set_text(ui_Label2, "Anwenden");
        lv_label_set_text(ui_lblLock2, "Zurueck");
        lv_label_set_text(ui_lblLock4, "Zurueck");
        lv_label_set_text(ui_lblLock8, "Zurueck");
        lv_label_set_text(ui_lblLock3, "Zurueck");
        lv_label_set_text(ui_lblSettings2, "Speichern");
        lv_label_set_text(ui_lblLock3, "Speichern");
        lv_label_set_text(ui_lblSelectTheme, "Thema: ");
        lv_label_set_text(ui_lblWallpaper, "Bildschirm: ");
        lv_label_set_text(ui_lblGrup1, "Frischwasser");
        lv_label_set_text(ui_lblGrup2, "Abwasser");
        lv_label_set_text(ui_lblPnlGrup1Sicaklik1, "\nTemperatur");
        lv_label_set_text(ui_lblPnlGrup1Sicaklik2, "\nFeuchte");
        lv_label_set_text(ui_Label12, "EINSTELLUNGEN WERDEN\nGESPEICHERT... NEUSTART");
        lv_label_set_text(ui_lblPanelSettings, "PANEL EINSTELLUNGEN");
        lv_label_set_text(ui_lblDimmableOutputs, "Dim-Ausgaenge");
        lv_label_set_text(ui_lblUnderArcTemperature1, "Temperatur");
        lv_label_set_text(ui_lblUnderArcTemperature2, "Feuchte");
        lv_label_set_text(ui_lblUnderArcWater1, "Frischwasser");
        lv_label_set_text(ui_lblUnderArcWater2, "Abwasser");

        for (int i = 0; i < numOfOutputs; i++) {
            lv_label_set_text(lblIO[i], lblBtnNames_DE[outputsBuffer[i] - 1]);
        }
        for (int i = 0; i < numOfDims && i < 4; i++) {
            lv_label_set_text(lblDims[i], lblDimBtnNames_DE[dimsBuffer[i] - 1]);
        }
        set_language_for_dropdowns();
    }



    lv_obj_t* dropdowns[16] = {ui_cbxO1, ui_cbxO2, ui_cbxO3, ui_cbxO4, ui_cbxO5, ui_cbxO6, ui_cbxO7, ui_cbxO8, ui_cbxO9, ui_cbxO10, ui_cbxO11, ui_cbxO12, ui_cbxO13, ui_cbxO14, ui_cbxO15, ui_cbxO16};
    lv_obj_t* dimdropdowns[4] = {ui_cbxDim1, ui_cbxDim2, ui_cbxDim3, ui_cbxDim4};
    for (int i = 0; i < numOfOutputs; i++) {
        lv_dropdown_set_selected(dropdowns[i], outputsBuffer[i] - 1); // Set the dropdown value
        ESP_LOGI("LANGUAGE_APPLY", "Dropdown %d set to index %d", i + 1, outputsBuffer[i] - 1);
    }

    for(int i = 0; i < numOfDims && i < 4; i++) {
        lv_dropdown_set_selected(dimdropdowns[i], dimsBuffer[i] - 1); // Set the dropdown value
        ESP_LOGI("LANGUAGE_APPLY", "Dim Dropdown %d set to index %d", i + 1, dimsBuffer[i] - 1);
    }

        /* Conflict detected → restore defaults */
        lv_dropdown_set_selected(ui_cbRSelect, selected_R);
        lv_dropdown_set_selected(ui_cbGSelect, selected_G);
        lv_dropdown_set_selected(ui_cbBSelect, selected_B);

    /* Keep dash Settings / nav in sync with panel language */
    vango_dash_on_prefs_changed();
}


// Callback function for color changes
 void color_wheel_event_cb() {
    uint8_t can_data[8] = {0}; // CAN verisi için buffer
    lv_color_t selected_color = lv_colorwheel_get_rgb(ui_Colorwheel1); // Get selected color
    // Apply selected color to the panel background
    lv_obj_set_style_bg_color(ui_btnRGBColor, selected_color, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui_btnRGBApply, selected_color, LV_PART_MAIN | LV_STATE_DEFAULT);

    lv_color32_t color32;
    
    color32.full = lv_color_to32(selected_color);

    uint8_t r = color32.ch.red;
    uint8_t g = color32.ch.green;
    uint8_t b = color32.ch.blue;

    can_data[selected_R] = r;  // İlk byte veri
    can_data[selected_G] = g;  // İlk byte veri
    can_data[selected_B] = b;  // İlk byte veri
    can_data[3] = rgbEna;
    send_can_frame(0x740, can_data);  // RGB için CAN ID: 0x740

    
}

// Callback function for color changes
 void set_RGB_color(int rr, int gg, int bb) {
    uint8_t can_data[8] = {0}; // CAN verisi için buffer

    can_data[0] = rr;  // İlk byte veri
    can_data[1] = gg;  // İlk byte veri
    can_data[2] = bb;  // İlk byte veri
    can_data[3] = 1;
    ESP_LOGI(TAG, "SET RGB COLOR R:%d G:%d B:%d", rr, gg, bb);
    send_can_frame(0x740, can_data);  // RGB için CAN ID: 0x740
}


// Callback function for color changes
 void apply_rgb_data_to_wheel(uint8_t r, uint8_t g, uint8_t b) {

    lv_color_t new_color = lv_color_make(r, g, b);
    // Apply selected color to the panel background
    lv_obj_set_style_bg_color(ui_btnRGBColor, new_color, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui_btnRGBApply, new_color, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_colorwheel_set_rgb(ui_Colorwheel1, new_color);

    //lv_colorwheel_set_rgb(ui_Colorwheel1, lv_color_make(255, 0, 0)); // Set to red

    
}
//######################################################################################################################




// Function to initialize dim widgets visibility based on numOfDims
void initialize_dim_widgets_visibility(void)
{
    // Check if UI objects are valid before accessing them
    if (ui_slDim1 == NULL || ui_slDim2 == NULL || ui_slDim3 == NULL || ui_slDim4 == NULL ||
        ui_lblDim1 == NULL || ui_lblDim2 == NULL || ui_lblDim3 == NULL || ui_lblDim4 == NULL) {
        ESP_LOGW(TAG, "Dim widgets not yet initialized, skipping visibility setup");
        return;
    }
    
    // Hide all dim widgets first
    lv_obj_add_flag(ui_slDim1, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(ui_slDim2, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(ui_slDim3, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(ui_slDim4, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(ui_lblDim1, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(ui_lblDim2, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(ui_lblDim3, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(ui_lblDim4, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(ui_lblDimLevel1, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(ui_lblDimLevel2, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(ui_lblDimLevel3, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(ui_lblDimLevel4, LV_OBJ_FLAG_HIDDEN);

    
    // Unhide only the number of dims specified by numOfDims and add event callbacks
    if (numOfDims >= 1) {
        lv_obj_clear_flag(ui_slDim1, LV_OBJ_FLAG_HIDDEN);
        lv_obj_clear_flag(ui_lblDim1, LV_OBJ_FLAG_HIDDEN);
        lv_obj_clear_flag(ui_lblDimLevel1, LV_OBJ_FLAG_HIDDEN);
        if(panelLanguageType == 0)
        {
            lv_label_set_text_fmt(ui_lblDim1, "%s:", lblDimBtnNames[dimsBuffer[0] - 1]);
        }
        else
        {
            lv_label_set_text_fmt(ui_lblDim1, "%s:", lblDimBtnNames_TR[dimsBuffer[0] - 1]);
        }    
        lv_obj_add_event_cb(ui_slDim1, dim_events, LV_EVENT_RELEASED, (void*)0);
    }
    if (numOfDims >= 2) {
        lv_obj_clear_flag(ui_slDim2, LV_OBJ_FLAG_HIDDEN);
        lv_obj_clear_flag(ui_lblDim2, LV_OBJ_FLAG_HIDDEN);
        lv_obj_clear_flag(ui_lblDimLevel2, LV_OBJ_FLAG_HIDDEN);
        if(panelLanguageType == 0)
        {
            lv_label_set_text_fmt(ui_lblDim2, "%s:", lblDimBtnNames[dimsBuffer[1] - 1]);
        }
        else
        {
            lv_label_set_text_fmt(ui_lblDim2, "%s:", lblDimBtnNames_TR[dimsBuffer[1] - 1]);
        }  
        lv_obj_add_event_cb(ui_slDim2, dim_events, LV_EVENT_RELEASED, (void*)1);
    }
    if (numOfDims >= 3) {
        lv_obj_clear_flag(ui_slDim3, LV_OBJ_FLAG_HIDDEN);
        lv_obj_clear_flag(ui_lblDim3, LV_OBJ_FLAG_HIDDEN);
        lv_obj_clear_flag(ui_lblDimLevel3, LV_OBJ_FLAG_HIDDEN);
        if(panelLanguageType == 0)
        {
            lv_label_set_text_fmt(ui_lblDim3, "%s:", lblDimBtnNames[dimsBuffer[2] - 1]);
        }
        else
        {
            lv_label_set_text_fmt(ui_lblDim3, "%s:", lblDimBtnNames_TR[dimsBuffer[2] - 1]);
        }  
        lv_obj_add_event_cb(ui_slDim3, dim_events, LV_EVENT_RELEASED, (void*)2);
    }
    if (numOfDims >= 4) {
        lv_obj_clear_flag(ui_slDim4, LV_OBJ_FLAG_HIDDEN);
        lv_obj_clear_flag(ui_lblDim4, LV_OBJ_FLAG_HIDDEN);
        lv_obj_clear_flag(ui_lblDimLevel4, LV_OBJ_FLAG_HIDDEN);
        if(panelLanguageType == 0)
        {
            lv_label_set_text_fmt(ui_lblDim4, "%s:", lblDimBtnNames[dimsBuffer[3] - 1]);
        }
        else
        {
            lv_label_set_text_fmt(ui_lblDim4, "%s:", lblDimBtnNames_TR[dimsBuffer[3] - 1]);
        }  
        lv_obj_add_event_cb(ui_slDim4, dim_events, LV_EVENT_RELEASED, (void*)3);
    }
    
    ESP_LOGI(TAG, "Initialized %d dim widgets visibility", numOfDims);
}


// Function to create output buttons on main screen
void create_output_buttons_on_screen(void)
{
    // Clear any existing output buttons first (full slot range — count may shrink)
    for (int i = 0; i < MAX_OUTPUTS; i++) {
        if (btnIO[i] != NULL) {
            lv_obj_del(btnIO[i]);
            btnIO[i] = NULL;
        }
        if (lblIO[i] != NULL) {
            lblIO[i] = NULL;
        }
        if (imgIO[i] != NULL) {
            imgIO[i] = NULL;
        }
    }
    
    // Create buttons on scrMain screen
    create_buttons_for_screen(ui_scrMain, "main");
}

// Function to create buttons for main screen
void create_buttons_for_screen(lv_obj_t* parent, const char* screen_type)
{
    // Use exact design for main screen
    int btn_width = 100;
    int btn_height = 90;
    int btn_x_offset = 106; // btn_width + 6 for spacing
    int btn_y_offset = 96;  // btn_height + 6 for spacing
    int x_start = 53;       // Start position for first column (right side)
    int y_start = -150;     // Start position for first row
    
    // Adjust button size and spacing if numOfOutputs is greater than 8
    if (numOfOutputs > 8) {
        btn_width = 105;
        btn_height = 95;
        btn_x_offset = 106; // btn_width + 1 for spacing
        btn_y_offset = 96; // btn_height + 1 for spacing
        y_start = -150;
    }

    for (int i = 0; i < numOfOutputs; i++) {
        int row = i / 4;
        int col = i % 4;

        btnIO[i] = lv_btn_create(parent);
        lv_obj_set_width(btnIO[i], btn_width);
        lv_obj_set_height(btnIO[i], btn_height);
        lv_obj_set_x(btnIO[i], x_start + col * btn_x_offset);
        lv_obj_set_y(btnIO[i], y_start + row * btn_y_offset);
        lv_obj_set_align(btnIO[i], LV_ALIGN_CENTER);
        lv_obj_add_flag(btnIO[i], LV_OBJ_FLAG_SCROLL_ON_FOCUS);     /// Flags
        lv_obj_clear_flag(btnIO[i], LV_OBJ_FLAG_SCROLLABLE);      /// Flags
        lv_obj_set_style_radius(btnIO[i], 0, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_bg_color(btnIO[i], lv_color_hex(0x5A5A5A), LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_bg_opa(btnIO[i], 255, LV_PART_MAIN | LV_STATE_DEFAULT);

        lblIO[i] = lv_label_create(btnIO[i]);
        lv_obj_set_width(lblIO[i], LV_SIZE_CONTENT);   /// 1
        lv_obj_set_height(lblIO[i], LV_SIZE_CONTENT);    /// 1
        lv_obj_set_x(lblIO[i], 0);

        if (numOfOutputs > 8) {
            lv_obj_set_align(lblIO[i], LV_ALIGN_BOTTOM_MID);
            lv_obj_set_y(lblIO[i], btn_height / 2 - 35); // Adjust y position to align at the bottom mid
        }
        else {
            lv_obj_set_y(lblIO[i], 0); // Adjust y position to align at the bottom mid
            lv_obj_set_align(lblIO[i], LV_ALIGN_CENTER);
        }
        lv_obj_set_align(lblIO[i], LV_ALIGN_BOTTOM_MID);
        lv_label_set_text(lblIO[i], lblBtnNames[outputsBuffer[i] - 1]);
        lv_obj_set_style_text_color(lblIO[i], lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_text_opa(lblIO[i], 255, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_text_font(lblIO[i], &lv_font_montserrat_18, LV_PART_MAIN | LV_STATE_DEFAULT);

        imgIO[i] = lv_img_create(btnIO[i]);
        lv_img_set_src(imgIO[i], get_image_for_button(outputsBuffer[i] - 1));
        lv_obj_set_width(imgIO[i], LV_SIZE_CONTENT);   /// 1
        lv_obj_set_height(imgIO[i], LV_SIZE_CONTENT);    /// 1
        lv_obj_set_x(imgIO[i], 0);
        lv_obj_set_y(imgIO[i], 5);
        if(numOfOutputs > 8) {
            lv_obj_set_align(imgIO[i], LV_ALIGN_TOP_MID);
        }
        else {
            lv_obj_set_align(imgIO[i], LV_ALIGN_CENTER);
        }
        
        lv_obj_add_flag(imgIO[i], LV_OBJ_FLAG_ADV_HITTEST);     /// Flags
        lv_obj_clear_flag(imgIO[i], LV_OBJ_FLAG_SCROLLABLE);      /// Flags
        
        // Add event callback for button click
        lv_obj_add_event_cb(btnIO[i], button_events, LV_EVENT_CLICKED, (void*)i);
    }
}


// Function to start the communication animation timer
void start_comm_animation_timer(void) {
    if (comm_animation_timer != NULL) {
        lv_timer_resume(comm_animation_timer);
    }
}

// Function to stop the communication animation timer
void stop_comm_animation_timer(void) {
    if (comm_animation_timer != NULL) {
        lv_timer_pause(comm_animation_timer);
    }
}

//###################################### Display Manager ################################################################

void display_manager_init() {
  
    waveshare_esp32_s3_rgb_lcd_init(); // Initialize the Waveshare ESP32-S3 RGB LCD 
     wavesahre_rgb_lcd_bl_on();  //Turn on the screen backlight 
    // wavesahre_rgb_lcd_bl_off(); //Turn off the screen backlight 
    
    lv_timer_t * updateScreentimer = lv_timer_create(timer_updateTimer_callback, 250, NULL);
    lv_timer_t * wallpaperTimer = lv_timer_create(wallpaper_update_timer_callback, 1000, NULL);
    (void)updateScreentimer;
    (void)wallpaperTimer;
    lv_timer_t * sendMotorDataTimer = lv_timer_create(sendMotorDataPeriodic_timer, 200, NULL);
    (void)sendMotorDataTimer;
    /* Eski iletişim animasyonu ui_Panel9'a dokunur; o ekran kurulmuyor. */

     hexnet_io_profile_sync_legacy_globals();
    sensorsBuffer[0] = 1;//temperature 1
    sensorsBuffer[1] = 1;//temperature 2
    sensorsBuffer[2] = 1;
    sensorsBuffer[3] = 0;
    sensorsBuffer[4] = 1;
    numOfSensors = 5;

     load_theme_configuration_from_nvs(&panelThemeType, &panelWallpaperEnable, &panelWallpaperTime, &panelLanguageType);

            // Debug log the loaded configuration
    ESP_LOGW(TAG, "Loaded Panel Configuration:");
    ESP_LOGW(TAG, "numOfOutputs: %d", numOfOutputs);
    for (int i = 0; i < 16; i++) {
        ESP_LOGW(TAG, "outputsBuffer[%d]: %d", i, outputsBuffer[i]);
    }
    ESP_LOGW(TAG, "numOfSensors: %d", numOfSensors);
    for (int i = 0; i < 5; i++) {
        ESP_LOGW(TAG, "sensorsBuffer[%d]: %d", i, sensorsBuffer[i]);
    }
    ESP_LOGW(TAG, "numOfDims: %d", numOfDims);
    for (int i = 0; i < 8; i++) {
        ESP_LOGW(TAG, "dimsBuffer[%d]: %d", i, dimsBuffer[i]);
    }
    ESP_LOGW(TAG, "panelThemeType: %d", panelThemeType);
    ESP_LOGW(TAG, "panelWallpaperEnable: %d", panelWallpaperEnable);
    ESP_LOGW(TAG, "panelWallpaperTime: %d", panelWallpaperTime);

   


    ESP_LOGI(TAG, "Legacy SquareLine UI init skipped");
    /* ui_init() / ui_scrInit / panel settings ekranı bilinçli olarak kurulmuyor.
       Kaynak dosyalar duruyor; çalışan arayüz vango_dash. */
}

void display_manager_start_dash(void)
{
    vango_shot_start();
    if (lvgl_port_lock(-1)) {
        if (vango_dash_get_screen() == NULL) {
            vango_dash_init();
            ESP_LOGI(TAG, "VANGO dash started after radio init");
        }
        vango_dash_show(VANGO_PAGE_HOME);
        lvgl_port_unlock();
    }
    vango_shot_set_ready();
    /* BLE advertise only after dash — prevents connect-time DRAM OOM reset loop */
    ble_start_advertising();
}