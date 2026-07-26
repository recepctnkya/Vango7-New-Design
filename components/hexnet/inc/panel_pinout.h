/**
 * @file panel_pinout.h
 * @brief VanGO Display — ESP32-S3 pin ve panel profili (4.3" / 7")
 *
 * Ortak: RGB 800x480, I2C SCL=9 SDA=8, RGB bus (waveshare_rgb_lcd_port.h).
 * Fark: CAN TX/RX ve CH422G backlight register (menuconfig veya manuel define).
 *
 * menuconfig: Example Configuration → VanGO panel size
 * Dokümantasyon: Documents/PINOUT.html
 */

#ifndef PANEL_PINOUT_H
#define PANEL_PINOUT_H

#include "sdkconfig.h"
#include "driver/gpio.h"

#ifdef __cplusplus
extern "C" {
#endif

/* -------------------------------------------------------------------------- */
/* Panel profili (menuconfig)                                                  */
/* -------------------------------------------------------------------------- */

#if CONFIG_VANGO_PANEL_SIZE_4_3_INCH
#define VANGO_PANEL_IS_4_3_INCH  1
#define VANGO_PANEL_IS_7_INCH    0
#define VANGO_PANEL_SIZE_LABEL   "4.3\""
#elif CONFIG_VANGO_PANEL_SIZE_7_INCH
#define VANGO_PANEL_IS_4_3_INCH  0
#define VANGO_PANEL_IS_7_INCH    1
#define VANGO_PANEL_SIZE_LABEL   "7\""
#else
/* Varsayılan: 7" (mevcut üretim firmware) */
#define VANGO_PANEL_IS_4_3_INCH  0
#define VANGO_PANEL_IS_7_INCH    1
#define VANGO_PANEL_SIZE_LABEL   "7\""
#endif

/* -------------------------------------------------------------------------- */
/* LVGL / LCD çözünürlük (her iki panel — firmware)                            */
/* -------------------------------------------------------------------------- */

#define VANGO_LCD_H_RES              800
#define VANGO_LCD_V_RES              480

/* -------------------------------------------------------------------------- */
/* I2C — touch (GT911) + CH422G backlight (ortak)                              */
/* -------------------------------------------------------------------------- */

#define VANGO_I2C_MASTER_NUM         0
#define VANGO_I2C_SCL_GPIO           9
#define VANGO_I2C_SDA_GPIO           8
#define VANGO_I2C_FREQ_HZ            400000

#define VANGO_CH422G_I2C_ADDR_CFG    0x24
#define VANGO_CH422G_I2C_ADDR_BL     0x38
#define VANGO_CH422G_CFG_OUTPUT_MODE 0x01

/* -------------------------------------------------------------------------- */
/* Panel kontrol GPIO (ortak)                                                    */
/* -------------------------------------------------------------------------- */

#define VANGO_GPIO_PANEL_CTRL        4

/* -------------------------------------------------------------------------- */
/* CAN / TWAI — panel boyutuna göre değişir                                    */
/* -------------------------------------------------------------------------- */

#if VANGO_PANEL_IS_4_3_INCH
#define VANGO_CAN_TX_GPIO            15
#define VANGO_CAN_RX_GPIO            16
#define VANGO_CH422G_BL_ON_REG       0x1E
#define VANGO_CH422G_BL_OFF_REG      0x1A
#else
#define VANGO_CAN_TX_GPIO            20
#define VANGO_CAN_RX_GPIO            19
#define VANGO_CH422G_BL_ON_REG       0x3E
#define VANGO_CH422G_BL_OFF_REG      0x3A
#endif

/* -------------------------------------------------------------------------- */
/* CAN protokol (ortak)                                                          */
/* -------------------------------------------------------------------------- */

#define VANGO_CAN_BITRATE_KBPS       500
#define VANGO_CAN_FRAME_VOLTAGE_IO   0x100
#define VANGO_CAN_FRAME_IO_PROFILE   0x110 /* IO flash SoT: relay/dim/sensor masks */
#define VANGO_CAN_FRAME_ANALOG_DIM   0x200
#define VANGO_CAN_FRAME_RGB_SENSORS  0x300

/* -------------------------------------------------------------------------- */
/* RGB LCD — senkron ve clock (ortak, waveshare_rgb_lcd_port.h ile aynı)       */
/* -------------------------------------------------------------------------- */

#define VANGO_RGB_VSYNC_GPIO         GPIO_NUM_3
#define VANGO_RGB_HSYNC_GPIO         GPIO_NUM_46
#define VANGO_RGB_DE_GPIO            GPIO_NUM_5
#define VANGO_RGB_PCLK_GPIO          GPIO_NUM_7
#define VANGO_RGB_PIXEL_CLOCK_HZ     (16 * 1000 * 1000)

#define VANGO_RGB_DATA0_GPIO         GPIO_NUM_14
#define VANGO_RGB_DATA1_GPIO         GPIO_NUM_38
#define VANGO_RGB_DATA2_GPIO         GPIO_NUM_18
#define VANGO_RGB_DATA3_GPIO         GPIO_NUM_17
#define VANGO_RGB_DATA4_GPIO         GPIO_NUM_10
#define VANGO_RGB_DATA5_GPIO         GPIO_NUM_39
#define VANGO_RGB_DATA6_GPIO         GPIO_NUM_0
#define VANGO_RGB_DATA7_GPIO         GPIO_NUM_45
#define VANGO_RGB_DATA8_GPIO         GPIO_NUM_48
#define VANGO_RGB_DATA9_GPIO         GPIO_NUM_47
#define VANGO_RGB_DATA10_GPIO        GPIO_NUM_21
#define VANGO_RGB_DATA11_GPIO        GPIO_NUM_1
#define VANGO_RGB_DATA12_GPIO        GPIO_NUM_2
#define VANGO_RGB_DATA13_GPIO        GPIO_NUM_42
#define VANGO_RGB_DATA14_GPIO        GPIO_NUM_41
#define VANGO_RGB_DATA15_GPIO        GPIO_NUM_40

#ifdef __cplusplus
}
#endif

#endif /* PANEL_PINOUT_H */
