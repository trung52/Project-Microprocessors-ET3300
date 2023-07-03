#include "mhz14a.h"

static uint32_t cap_val_begin_of_sample = 0;
static uint32_t cap_val_end_of_sample = 0;

static xQueueHandle cap_queue;

bool is_new_mhz14a_data = false;

int64_t pulseTimeBegin = 0;
int64_t pulseTimeEnd = 0;
void IRAM_ATTR interrup_handle(){
    if(is_new_mhz14a_data == false){
        if(gpio_get_level(CONFIG_MHZ14A_PWM_PIN) == GPIO_HIGH_LEVEL){
            pulseTimeBegin = esp_timer_get_time();
        }
        else{
            pulseTimeEnd = esp_timer_get_time();
            is_new_mhz14a_data = true;
        }
    }
}

static bool mhz_isr_handler(mcpwm_unit_t mcpwm, mcpwm_capture_channel_id_t cap_sig, const cap_event_data_t *edata,
                                  void *arg) {
    //calculate the interval in the ISR,
    //so that the interval will be always correct even when cap_queue is not handled in time and overflow.
    BaseType_t high_task_wakeup = pdFALSE;
    if (edata->cap_edge == MCPWM_POS_EDGE) {
        // store the timestamp when pos edge is detected
        cap_val_begin_of_sample = edata->cap_value;
        cap_val_end_of_sample = cap_val_begin_of_sample;
    } else {
        cap_val_end_of_sample = edata->cap_value;
        uint32_t pulse_count = cap_val_end_of_sample - cap_val_begin_of_sample;
        // send measurement back though queue
        xQueueSendFromISR(cap_queue, &pulse_count, &high_task_wakeup);
    }
    return high_task_wakeup == pdTRUE;
}

esp_err_t mhz14a_init(const gpio_config_t *mhz14a_pwm_pin_config)
{
    // esp_err_t error_config_gpio = gpio_config(mhz14a_pwm_pin_config);
    // ESP_ERROR_CHECK_WITHOUT_ABORT (error_config_gpio);
    // if(error_config_gpio == ESP_OK){
    //     ESP_LOGI(__func__,"MHZ14A config GPIO successfully.");
    // }
    // else
    // {
    //     ESP_LOGE(__func__,"MHZ14A config GPIO failed.");
    //     return ESP_ERROR_MHZ14A_CONFIG_GPIO_FAILED;
    // }

    // esp_err_t error_install_isr = gpio_install_isr_service(ESP_INTR_FLAG_EDGE);
    // ESP_ERROR_CHECK_WITHOUT_ABORT(error_install_isr);
    // esp_err_t error_add_isr = gpio_isr_handler_add(CONFIG_MHZ14A_PWM_PIN, interrup_handle, NULL);
    // ESP_ERROR_CHECK_WITHOUT_ABORT(error_add_isr);

    // if(error_install_isr != ESP_OK ||
    //    error_add_isr != ESP_OK){
    //     ESP_LOGE(__func__,"MHZ14A initialize interrupt failed.");
    //     return ESP_ERROR_MHZ14A_INIT_INTR_FAILED;
    // }
    
    cap_queue = xQueueCreate(1, sizeof(uint32_t));
    if (cap_queue == NULL) {
        ESP_LOGE(__func__, "failed to alloc cap_queue");
        return ESP_FAIL;
    }

    // set CAP_0 on GPIO
    ESP_ERROR_CHECK(mcpwm_gpio_init(MCPWM_UNIT_0, MCPWM_CAP_0, CONFIG_MHZ14A_PWM_PIN));
    // enable pull down CAP0, to reduce noise
    ESP_ERROR_CHECK(gpio_pulldown_en(CONFIG_MHZ14A_PWM_PIN));
    // enable both edge capture on CAP0
    mcpwm_capture_config_t conf = {
        .cap_edge = MCPWM_BOTH_EDGE,
        .cap_prescale = 1,
        .capture_cb = mhz_isr_handler,
        .user_data = NULL
    };
    ESP_ERROR_CHECK(mcpwm_capture_enable_channel(MCPWM_UNIT_0, MCPWM_SELECT_CAP0, &conf));
    ESP_LOGI(__func__, "MHZ14A PWM pin configured");

    //Warming up
    int64_t time_start_warm_up = esp_timer_get_time();
    while (esp_timer_get_time() - time_start_warm_up <TIME_TO_WARM_UP)
    {
        ESP_LOGI(__func__,"MHZ14A warming up...");
        vTaskDelay(1000/portTICK_RATE_MS);
    }
    

    ESP_LOGI(__func__,"MHZ14A initialize success.");
    return ESP_OK;
}

esp_err_t mhz14a_readData(uint32_t *co2_ppm)
{
    //Write by interrupt
    // if(is_new_mhz14a_data == true){
    //     ESP_LOGE(__func__,"timeend: %lld timebegin: %lld", pulseTimeEnd, pulseTimeBegin);
    //     uint32_t timeHigh, timeLow, ppm_pwm;
    //     timeHigh =(uint32_t)((pulseTimeEnd - pulseTimeBegin)/1000) ; //change to milliseconds
    //     timeLow = 1024-timeHigh;
    //     ppm_pwm = MHZ14A_RANGE * (timeHigh-2)/(timeHigh+timeLow-4);
    //     if(ppm_pwm >2000)
    //         ppm_pwm = 2000;
    //     *co2_ppm = ppm_pwm;
    //     ESP_LOGI(__func__,"MHZ14A get data successfully: %d ppm", ppm_pwm);
    //     is_new_mhz14a_data = false;
    //     return ESP_OK;
    // }
    // else {
    //     *co2_ppm = -1;
    //     ESP_LOGE(__func__,"MHZ14A get data failed.");
    //     return ESP_ERROR_MHZ14A_READ_DATA_FAILED;
    // }

    //Write by while loop - very stupid
    // while (gpio_get_level(CONFIG_MHZ14A_PWM_PIN) == GPIO_LOW_LEVEL) {};
    // int64_t t0 = esp_timer_get_time();
    // while (gpio_get_level(CONFIG_MHZ14A_PWM_PIN) == GPIO_HIGH_LEVEL) {};
    // int64_t t1 = esp_timer_get_time();
    // while (gpio_get_level(CONFIG_MHZ14A_PWM_PIN) == GPIO_LOW_LEVEL) {};
    // int64_t t2 = esp_timer_get_time();
    // int32_t th = (uint32_t)((t1-t0)/1000);
    // int32_t tl = (uint32_t)((t2-t1)/1000);
    // uint32_t ppm = 2000 * (th - 2) / (th + tl - 4);
    // while (gpio_get_level(CONFIG_MHZ14A_PWM_PIN) == GPIO_HIGH_LEVEL) {};
    // *co2_ppm = ppm;
    // ESP_LOGI(__func__,"MHZ14A get data successfully: %d ppm", ppm);
    // vTaskDelay(10/portTICK_RATE_MS);
    // return ESP_OK;

    //Wirte by PWM capture
    uint32_t pulse_count, time_high, time_low, pwm_ppm;
        // block and wait for new measurement
        xQueueReceive(cap_queue, &pulse_count, portMAX_DELAY);
        uint32_t pulse_width_us = pulse_count * (1000000.0 / rtc_clk_apb_freq_get());
        time_high = pulse_width_us/1000;
        time_low = 1024 - time_high;
        pwm_ppm = MHZ14A_RANGE*(time_high-2)/(time_high+time_low - 4);
        *co2_ppm = pwm_ppm;
        ESP_LOGI(__func__, "Pulse width: %d, Co2 concentration: %dppm", pulse_width_us, pwm_ppm);
        return ESP_OK;
}

