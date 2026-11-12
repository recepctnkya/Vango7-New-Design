// display_manager.h

#ifndef DISPLAY_MANAGER_H
#define DISPLAY_MANAGER_H

#include <stdint.h>
#include "lvgl_port.h"

void update_display_with_data(const uint8_t *data, int length);
void display_manager_init();
/** Create VANGO dash after Wi-Fi/BLE so controller gets internal DRAM first */
void display_manager_start_dash(void);
/** Preset index 0..17 → LVGL image (lblBtnNames). */
const void *get_image_for_button(int outputBufferIndex);
void apply_screen_temperature_textbox(int sicaklik1_value, int sicaklik2_value);

void my_btnThemeWhiteFunc(void);
void my_btnBlackThemeFunc(void);
void update_regs_data(uint16_t index, uint16_t value);
void save_panel_settings();
void save_theme_settings();
/** Persist current theme/lang/wallpaper globals to NVS (does not read legacy widgets). */
void persist_theme_settings(void);
/** Push globals → legacy theme/lang widgets (dropdown, roller, switch). */
void sync_theme_widgets_from_globals(void);
/** Apply theme+lang everywhere and persist (for dash Settings). Also pushes to IO SoT via CAN 0x760. */
void commit_theme_language_settings(void);
/** Legacy SquareLine screensaver (black + logo / clock). */
void hexnet_wallpaper_show(void);
void hexnet_wallpaper_dismiss(void);
/** Touch / page activity — idle timer starts only after interaction stops. */
void hexnet_wallpaper_note_activity(void);
/** IO Module CAN/BLE SoT → Display cache (no push back to IO). */
void apply_theme_language_from_io(int theme, int language);
void apply_theme_settings();
void apply_language_settings();
void color_wheel_event_cb();
void save_panel_configuration_to_nvs(int totalOutps, int buffer1[16], int totalSensors, int buffer2[5], int totalDims, int buffer3[8]);
void load_panel_configuration_from_nvs(int *totalOutpts, int buffer1[16], int *totalSensors, int buffer2[5], int *totalDims, int buffer3[8]);
void apply_rgb_data_to_wheel(uint8_t r, uint8_t g, uint8_t b);
void set_rgb_to_white();
void set_RGBTurnONOFF(int val);
void set_RGB_color(int rr, int gg, int bb); 

// Output buttons creation function
void create_output_buttons_on_screen(void);
void create_buttons_for_screen(lv_obj_t* parent, const char* screen_type);
void move_buttons_to_main_screen(void);

// Dim widgets visibility initialization
void initialize_dim_widgets_visibility(void);

// Communication animation timer control functions
void start_comm_animation_timer(void);
void stop_comm_animation_timer(void);

#endif // DISPLAY_MANAGER_H