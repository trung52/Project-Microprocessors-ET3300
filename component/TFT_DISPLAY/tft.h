/**
 * @file tftdisplay.h
 * @author Vu Thanh Trung   (20203623)
 * @brief Library for SPI TFT display module (ILI9341)
 * @note Before use this file and the lvgl_esp32_driver,
 * run idf.py menuconfig, go to Component config then
 * LVGL TFT configuration and LVGL TFT Display configuration
 * to configure lvgl_esp32_drivers.
*/

#ifndef __TFT_H__
#define __TFT_H__

#include <stdio.h>
#include "esp_err.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "../lvgl/lvgl.h"
#include "../../build/config/sdkconfig.h"
#include "driver/gpio.h"
#include "driver/spi_common.h"
#include "../lvgl_esp32_drivers/lvgl_helpers.h"
#include "../lvgl_esp32_drivers/lvgl_spi_conf.h"
#include "../lvgl_esp32_drivers/lvgl_tft/disp_spi.h"
#include "../lvgl_esp32_drivers/lvgl_tft/disp_driver.h"
#include "../datamanager/datamanager.h"
#define LV_TICK_PERIOD_MS 1

#define ID_TFT 0x03

#define ESP_ERROR_TFT_INIT_FAILED ((ID_TFT << 12)|0x00)

struct label_st
{
    lv_obj_t *dateTime;
    lv_obj_t *groupName;

    lv_obj_t *temp_label;
    lv_obj_t *temp_val;

    lv_obj_t *humi_label;
    lv_obj_t *humi_val;

    lv_obj_t *press_label;
    lv_obj_t *press_val;

    lv_obj_t *co2_label;
    lv_obj_t *co2_val;
    
    lv_obj_t *pm10_label;
    lv_obj_t *pm10_val;
    
    lv_obj_t *pm2_5_label;
    lv_obj_t *pm2_5_val;

    lv_obj_t *pm1_label;
    lv_obj_t *pm1_val;

};

extern struct label_st label_to_display;

/**
 * @brief Initialize SPI and driver for TFT display
 * @return esp_err_t
*/
esp_err_t tft_initialize();

/**
 * @brief Initialize screen display
*/
void tft_initScreen();

/**
 * @brief Update data and time to screen
 * @param data_sensor
 * @return esp_err_t
*/
esp_err_t tft_updateScreen(struct dataSensor_st *data_sensor, const char*timestring);

#endif