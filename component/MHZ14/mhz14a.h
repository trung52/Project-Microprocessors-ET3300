/**
 * @file mhz14a.h
 * @author Vu Thanh Trung (20203623)
 * @brief Library for MHZ14A
 * @date 2023-06-16
 * @copyright Copyright (c) 2023
 */
#ifndef __MHZ14_H__
#define __MHZ14_H__

#include <stdio.h>
#include "esp_err.h"
#include "esp_log.h"
#include "driver/gpio.h"
#include "soc/rtc.h"
#include "../../build/config/sdkconfig.h"
#include "esp_intr_alloc.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "../../../esp-idf-v4.4.3/components/freertos/include/freertos/queue.h"
#include "esp_timer.h"
#include "driver/mcpwm.h"

#define ID_MHZ14A 0x03
#define ESP_ERROR_MHZ14A_READ_DATA_FAILED ((ID_MHZ14A<<12)|0X00)
#define ESP_ERROR_MHZ14A_CONFIG_GPIO_FAILED ((ID_MHZ14A<<12)|0x01)
#define ESP_ERROR_MHZ14A_INIT_INTR_FAILED ((ID_MHZ14A<<12)|0x02)

#define GPIO_HIGH_LEVEL 1
#define GPIO_LOW_LEVEL 0

#define TIME_TO_WARM_UP 10000000 // Time in microseconds

#define MHZ14A_RANGE 2000

#define MHZ14A_PWM_PIN_CONFIG_DEFAULT() { .pin_bit_mask = BIT(CONFIG_MHZ14A_PWM_PIN),  \
                                          .mode = GPIO_MODE_INPUT,                     \
                                          .pull_up_en = GPIO_PULLUP_ENABLE,            \
                                          .pull_down_en = GPIO_PULLDOWN_DISABLE,       \
                                          .intr_type = GPIO_INTR_ANYEDGE,              \
}

/**
 * @brief Initialize MHZ14A sensor with PWM output
 * @param mhz14a_pwm_pin_config
 * @author Vu Thanh Trung (20203623)
 * @return esp_err_t
*/
esp_err_t mhz14a_init(const gpio_config_t *mhz14a_pwm_pin_config);

/**
 * @brief Read data by PWM way
 * @author Vu Thanh Trung (20203623)
 * @param [out] cos_ppm
*/
esp_err_t mhz14a_readData(uint32_t *co2_ppm);


#endif
