#include "pms7003.h"
__attribute__((unused)) static const char *TAG = "PMS7003";

esp_err_t pms7003_initUart(uart_config_t *pms_uart_config){
    /** 
     * @note: 3rd parameter of uart_driver_install() function - rx_buffer_size never less than 128 byte.
     *        if rx_buffer_size <= 128, program print error code "uart rx buffer length error".
     */
    esp_err_t error1 = uart_driver_install(CONFIG_PMS_UART_PORT, (RX_BUFFER_SIZE*2), 0, 0, NULL,0);
    ESP_ERROR_CHECK_WITHOUT_ABORT(error1);
    esp_err_t error2 = uart_param_config(CONFIG_PMS_UART_PORT, pms_uart_config);
    ESP_ERROR_CHECK_WITHOUT_ABORT(error2);
    esp_err_t error3 = uart_set_pin(CONFIG_PMS_UART_PORT, CONFIG_PMS_PIN_TX, CONFIG_PMS_PIN_RX, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
    ESP_ERROR_CHECK_WITHOUT_ABORT(error3);

    if(error1 != ESP_OK){
        ESP_LOGI(__func__, "PMS7003 install driver UART failed.");
        return ESP_ERROR_PMS7003_INSTALL_DRIVER_UART_FAILED;
    }
    if (error2 != ESP_OK){
        ESP_LOGI(__func__, "PMS7003 config param UART failed.");
        return ESP_ERROR_PMS7003_CONFIG_PARAM_UART_FAILED;
    }
    if(error3 != ESP_OK){
        ESP_LOGI(__func__, "PMS7003 set pin UART failed.");
        return ESP_ERROR_PMS7003_SET_PIN_UART_FAILED;
    }

    ESP_LOGI(__func__, "PMS7003 UART port initialize successful.");
    return ESP_OK;

}

esp_err_t pms7003_setActiveMode(void)
{
    esp_err_t errorReturn = uart_write_bytes(CONFIG_PMS_UART_PORT, pms7003_commandActiveMode, sizeof(pms7003_commandActiveMode));
    if (errorReturn == -1)
    {
        ESP_LOGE(__func__, "Set active mode failed.");
        return ESP_ERROR_PMS7003_SET_ACTIVE_MODE_FAILED;
    } else {
        ESP_LOGI(__func__, "Set active mode successfuly.");
        return ESP_OK;
    }
}

esp_err_t pms7003_readData(const int pms_modeAmbience, uint32_t *pm1_0, uint32_t *pm2_5, uint32_t *pm10)
{
    uint8_t rawData[128];
    bool check = false;
    SemaphoreHandle_t print_muxtex = NULL;
    print_muxtex = xSemaphoreCreateMutex();

    // Read raw data array
    int lengthSensorDataArray;
    lengthSensorDataArray = uart_read_bytes(CONFIG_PMS_UART_PORT, rawData, RX_BUFFER_SIZE, 100 / portTICK_PERIOD_MS);
    xSemaphoreTake(print_muxtex, portMAX_DELAY);

    for (size_t i = 0; i < 104; i++)
    {
        if (rawData[i] == START_CHARACTER_1 &&      // Check first start byte in raw data array.
            rawData[i+1] == START_CHARACTER_2)        // Check second start byte in raw data array.
        {
            uint8_t startByte;
            startByte = i;
            startByte += (pms_modeAmbience == indoor) ? 4 : 10;          //atmospheric from 10th byte, standard from 4th in raw data array.
            
            *pm1_0 = ((rawData[startByte] << 8) + rawData[startByte + 1]);
            *pm2_5 = ((rawData[startByte + 2] << 8) + rawData[startByte + 3]);
            *pm10  = ((rawData[startByte + 4] << 8) + rawData[startByte + 5]);

            ESP_LOGI(__func__, "PMS7003 sensor read data successful.");
            ESP_LOGI(__func__, "PM1.0: %dug/m3\tPM2.5: %dug/m3\tPM10: %dug/m3.\r", *pm1_0, *pm2_5, *pm10);
            xSemaphoreGive(print_muxtex);
            vSemaphoreDelete(print_muxtex);
            check = true;
            break;
        }
    }

    if(check == false) {
        *pm1_0 = PMS_ERROR_INVALID_VALUE;       // Return invalid value of sensor.
        *pm2_5 = PMS_ERROR_INVALID_VALUE;       // Return invalid value of sensor.
        *pm10  = PMS_ERROR_INVALID_VALUE;       // Return invalid value of sensor.
        ESP_LOGE(__func__, "PMS7003 sensor read data failed.");
        xSemaphoreGive(print_muxtex);
        vSemaphoreDelete(print_muxtex);
        return ESP_ERROR_PMS7003_READ_DATA_FAILED;
    }
    return ESP_OK;
}
