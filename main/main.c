/**
 * @file main.c
 * @author Vu Thanh Trung (20203623)
 * @brief Main file of project VXL
 * @date 2023-07-06
*/

/*---------------------------------- INCLUDE LIBRARY ----------------------------------*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>

#include "../build/config/sdkconfig.h"
#include "pms7003.h"
#include "../component/BME280/bme280.h"
#include "../component/Time/DS3231Time.h"
#include "../component/DeviceManager/DeviceManager.h"
#include "../component/datamanager/datamanager.h" 
#include "../component/FileManager/sdcard.h"
#include "../component/TFT_DISPLAY/tft.h"
#include "mhz14a.h"

/*---------------------------------- DEFINE ---------------------------------- */
#define PERIOD_GET_DATA_FROM_SENSOR           (TickType_t)(5000 / portTICK_RATE_MS)
#define PERIOD_SAVE_DATA_SENSOR_TO_SDCARD     (TickType_t)(2500 / portTICK_RATE_MS)
#define PERIOD_READ_DATA_SENSOR_FROM_SDCARD   (TickType_t)(120000/portTICK_RATE_MS)
#define PERIOD_UPDATE_DATA_ON_SCREEN    (TickType_t)(1000 / portTICK_RATE_MS)

#define WAIT_10_TICK                                (TickType_t)(10 / portTICK_RATE_MS)

#define QUEUE_SIZE              10U
#define NAME_FILE_QUEUE_SIZE    5U

uart_config_t pms_uart_config = UART_CONFIG_DEFAULT;

gpio_config_t mhz14a_pwm_pin_config = MHZ14A_PWM_PIN_CONFIG_DEFAULT(); 

bmp280_t bme280_device;
bmp280_params_t bme280_params;

i2c_dev_t ds3231_device;

TaskHandle_t getDataFromSensorTask_handle = NULL;
TaskHandle_t saveDataSensorToSDcardTask_handle = NULL;
TaskHandle_t readDataSensorFromSDcardTask_handle = NULL;
TaskHandle_t updateScreenTask_handle = NULL;

SemaphoreHandle_t getDataFromSensor_semaphore = NULL;
SemaphoreHandle_t saveDataSensorToSDcard_semaphore = NULL;
SemaphoreHandle_t readDataSensorFromSDcard_semaphore = NULL;
SemaphoreHandle_t updateScreen_semaphore = NULL;

QueueHandle_t dataSensorSentToSD_queue;
QueueHandle_t dataSensorSentToScreen_queue;

static char nameFileSaveData[21];
static char timeDisplayOnScreen[30];
struct label_st label_to_display;

void getDataFromSensor_task(void *parameters){
    int64_t timestart, timeend;
    struct dataSensor_st dataSensorTemp;
    struct moduleError_st moduleErrorTemp;
    TickType_t task_lastWakeTime;
    task_lastWakeTime = xTaskGetTickCount();
    getDataFromSensor_semaphore = xSemaphoreCreateMutex();
    for(;;){
        if(xSemaphoreTake(getDataFromSensor_semaphore, portMAX_DELAY)){ //Take mutex
            timestart = esp_timer_get_time();
            moduleErrorTemp.ds3231Error = ds3231_getEpochTime(&ds3231_device, &(dataSensorTemp.timeStamp));

            moduleErrorTemp.pms7003Error = pms7003_readData(indoor, &(dataSensorTemp.pm1_0), 
                                                          &(dataSensorTemp.pm2_5), 
                                                          &(dataSensorTemp.pm10));
        
            moduleErrorTemp.bme280Error = bme280_readSensorData(&bme280_device,
                                                       &(dataSensorTemp.temperature),
                                                       &(dataSensorTemp.pressure), 
                                                       &(dataSensorTemp.humidity));
            moduleErrorTemp.mhz14aError = mhz14a_readData(&(dataSensorTemp.co2));
            xSemaphoreGive(getDataFromSensor_semaphore); //Give mutex

            printf("%s,%llu,%.2f,%.2f,%.2f,%d,%d,%d,%d\n", CONFIG_NAME_DEVICE,
                                                        dataSensorTemp.timeStamp,
                                                        dataSensorTemp.temperature,
                                                        dataSensorTemp.humidity,
                                                        dataSensorTemp.pressure,
                                                        dataSensorTemp.pm1_0,
                                                        dataSensorTemp.pm2_5,
                                                        dataSensorTemp.pm10,
                                                        dataSensorTemp.co2);
            ESP_ERROR_CHECK_WITHOUT_ABORT(moduleErrorTemp.ds3231Error);
            ESP_ERROR_CHECK_WITHOUT_ABORT(moduleErrorTemp.pms7003Error);
            ESP_ERROR_CHECK_WITHOUT_ABORT(moduleErrorTemp.bme280Error);
            ESP_ERROR_CHECK_WITHOUT_ABORT(moduleErrorTemp.mhz14aError);

            ESP_LOGI(__func__, "Read data from sensors completed");
            
            if(xQueueSendToBack(dataSensorSentToSD_queue, (void*)&dataSensorTemp, WAIT_10_TICK*5) == pdPASS){
                ESP_LOGI(__func__, "Success to post the data sensor to dataSensorSentToSD Queue.");
            }
            else{
                ESP_LOGE(__func__, "Failed to post the data sensor to dataSensorSentToSD Queue.");
            }
            if(xQueueSendToBack(dataSensorSentToScreen_queue, (void*)&dataSensorTemp, WAIT_10_TICK*5) == pdPASS){
                ESP_LOGI(__func__, "Success to post the data sensor to dataSensorSentToScreen Queue.");
            }
            else{
                ESP_LOGE(__func__, "Failed to post the data sensor to dataSensorSentToScreen Queue.");
            }

            memset(&dataSensorTemp, 0, sizeof(struct dataSensor_st));
            memset(&moduleErrorTemp, 0, sizeof(struct moduleError_st));
            vTaskDelayUntil(&task_lastWakeTime,PERIOD_GET_DATA_FROM_SENSOR);
            timeend = esp_timer_get_time();
            ESP_LOGW(__func__, "Time to execute getDataFromSensor loop: %lld", timeend - timestart);
        }
    }
    
}

void saveDataSensorToSDcard_task(void *parameters)
{ 
    struct dataSensor_st dataSensorReceiveFromQueue;
    saveDataSensorToSDcard_semaphore = xSemaphoreCreateMutex();
    for(;;)
    {
        if (uxQueueMessagesWaiting(dataSensorSentToSD_queue) != 0)      // Check if dataSensorSentToSD_queue is empty
        {
            if(xQueueReceive(dataSensorSentToSD_queue, (void *)&dataSensorReceiveFromQueue, WAIT_10_TICK * 50) == pdPASS)   // Get data sesor from queue
            {
                ds3231_convertTimeToString(&ds3231_device, nameFileSaveData, 10);   // Get dateTime string (as name file to save data follow date)
                ESP_LOGI(__func__, "Receiving data from queue successfully.");
                if(xSemaphoreTake(saveDataSensorToSDcard_semaphore, portMAX_DELAY) == pdTRUE)
                {
                    static esp_err_t errorCode_t;
                    //Create data string follow format
                    errorCode_t = sdcard_writeDataToFile(nameFileSaveData, "%s,%llu,%.2f,%.2f,%.2f,%d,%d,%d,%d\n", CONFIG_NAME_DEVICE,
                                                                                                                dataSensorReceiveFromQueue.timeStamp,
                                                                                                                dataSensorReceiveFromQueue.temperature,
                                                                                                                dataSensorReceiveFromQueue.humidity,
                                                                                                                dataSensorReceiveFromQueue.pressure,
                                                                                                                dataSensorReceiveFromQueue.pm1_0,
                                                                                                                dataSensorReceiveFromQueue.pm2_5,
                                                                                                                dataSensorReceiveFromQueue.pm10,
                                                                                                                dataSensorReceiveFromQueue.co2);
                    xSemaphoreGive(saveDataSensorToSDcard_semaphore);
                    if (errorCode_t != ESP_OK)
                    {
                        ESP_LOGE(__func__, "sdcard_writeDataToFile(...) function returned error: 0x%.4X", errorCode_t);
                    }
                }
            } else {
                ESP_LOGI(__func__, "Receiving data from queue failed.");
            }
        }

        vTaskDelay(PERIOD_SAVE_DATA_SENSOR_TO_SDCARD);
    }
};

void readDataSensorFromSDcard_task(void *parameters ){
    struct dataSensor_st dataSensorReadFromSD;
    memset(&dataSensorReadFromSD,0,sizeof(struct dataSensor_st));
    readDataSensorFromSDcard_semaphore = xSemaphoreCreateMutex();
    for(;;){
        vTaskDelay(PERIOD_READ_DATA_SENSOR_FROM_SDCARD);
        if(xSemaphoreTake(readDataSensorFromSDcard_semaphore,portMAX_DELAY) == pdTRUE){
            esp_err_t errorCode_t = sdcard_readDataFromFile(nameFileSaveData,"%llu,%.2f,%.2f,%.2f,%d,%d,%d\n", dataSensorReadFromSD.timeStamp,
                                                                                                   dataSensorReadFromSD.temperature,
                                                                                                   dataSensorReadFromSD.humidity,
                                                                                                   dataSensorReadFromSD.pressure,
                                                                                                   dataSensorReadFromSD.pm1_0,
                                                                                                   dataSensorReadFromSD.pm2_5,
                                                                                                   dataSensorReadFromSD.pm10);
            xSemaphoreGive(readDataSensorFromSDcard_semaphore);
            if(errorCode_t != ESP_OK){
                ESP_LOGE(__func__, "sdcard_readDataFromFile(...) function returned error");
            }
        }
    }
}

void updateScreen_task(void *parameters){
    
    struct dataSensor_st dataSensorReceiveFromQueue;
    updateScreen_semaphore = xSemaphoreCreateMutex();
    for(;;){
        //lv_task_handler();
        if(uxQueueMessagesWaiting(dataSensorSentToScreen_queue)!=0){
            if(xQueueReceive(dataSensorSentToScreen_queue, (void*) &dataSensorReceiveFromQueue, WAIT_10_TICK*50) == pdPASS){
                ESP_LOGI(__func__, "Receiving data from queue successfully.");
                if(pdTRUE == xSemaphoreTake(updateScreen_semaphore, portMAX_DELAY)) {
                    ds3231_convertTimeToStringForScreen(&ds3231_device, timeDisplayOnScreen, 21);
                    esp_err_t errorCode = tft_updateScreen(&dataSensorReceiveFromQueue, timeDisplayOnScreen);
                    lv_task_handler();
                    xSemaphoreGive(updateScreen_semaphore);
                    if(errorCode == ESP_OK){
                        ESP_LOGI(__func__,"TFT update data on screen successfully.");
                    }
                    else{
                        ESP_LOGE(__func__,"TFT update data on screen failed");
                    }
                    
                }
            }
            else {
                ESP_LOGI(__func__, "Receiving data from queue failed.");
            }
        }
        vTaskDelay(PERIOD_UPDATE_DATA_ON_SCREEN);
    }

    // for(;;){
    //     vTaskDelay(pdMS_TO_TICKS(10));
    //     lv_task_handler();
    //     ESP_LOGI(__func__, "Start update screen.");
    //     if (pdTRUE == xSemaphoreTake(updateScreen_semaphore, portMAX_DELAY)) {
    //         lv_task_handler();
    //         ds3231_convertTimeToStringForScreen(&ds3231_device, timeDisplayOnScreen, 21);
    //         esp_err_t errorCode = tft_updateScreen(&dataSensorReceiveFromQueue, timeDisplayOnScreen);
    //         xSemaphoreGive(updateScreen_semaphore);
    //    }
    //    //vTaskDelay(PERIOD_UPDATE_DATA_ON_SCREEN);
    // }
}

void app_main(void)
{
    //Initialize SD card
    ESP_LOGI(__func__, "Initialize SD card with SPI interface.");
    esp_vfs_fat_mount_config_t  mount_config_t   = MOUNT_CONFIG_DEFAULT();
    spi_bus_config_t            spi_bus_config_t = SPI_BUS_CONFIG_DEFAULT();
    sdmmc_host_t                host_t           = SDSPI_HOST_DEFAULT();
    sdspi_device_config_t       slot_config      = SDSPI_DEVICE_CONFIG_DEFAULT();
    slot_config.gpio_cs = CONFIG_PIN_NUM_CS;
    slot_config.host_id = host_t.slot;

    sdmmc_card_t SDCARD;
    ESP_ERROR_CHECK_WITHOUT_ABORT(sdcard_initialize(&mount_config_t, &SDCARD, &host_t, &spi_bus_config_t, &slot_config));
    
    //Initialize TFT display
    ESP_ERROR_CHECK_WITHOUT_ABORT(tft_initialize());
    tft_initScreen();
    lv_task_handler();
    

    //Initialize PMS7003
    ESP_ERROR_CHECK_WITHOUT_ABORT(pms7003_initUart(&pms_uart_config));

    //Initialize BME280
    ESP_ERROR_CHECK_WITHOUT_ABORT(i2cdev_init());
    ESP_LOGI(__func__, "Initialize BME280 sensor(I2C/Wire%d).", CONFIG_BME_I2C_PORT);
    ESP_ERROR_CHECK_WITHOUT_ABORT(bme280_init(&bme280_device,&bme280_params, BME280_ADDRESS,
                                    CONFIG_BME_I2C_PORT, CONFIG_BME_PIN_NUM_SDA, CONFIG_BME_PIN_NUM_SCL));
    
    //Initialize DS3231
    ESP_LOGI(__func__, "Initialize DS3231 sensor(I2C/Wire%d).", CONFIG_RTC_I2C_PORT);
    ESP_ERROR_CHECK_WITHOUT_ABORT(ds3231_initialize(&ds3231_device,CONFIG_RTC_I2C_PORT, CONFIG_RTC_PIN_NUM_SDA, CONFIG_RTC_PIN_NUM_SCL));

    //Initialize MHZ14A
    ESP_ERROR_CHECK_WITHOUT_ABORT(mhz14a_init(&mhz14a_pwm_pin_config));

    //Creat dataSensorSentToSD Queue
    dataSensorSentToSD_queue = xQueueCreate(QUEUE_SIZE, sizeof(struct dataSensor_st));
    while(dataSensorSentToSD_queue == NULL){
        ESP_LOGE(__func__, "Create dataSensorSentToSD Queue failed.");
        ESP_LOGI(__func__, "Retry to create dataSensorSentToSD Queue...");
        vTaskDelay(500/portTICK_RATE_MS);
        dataSensorSentToSD_queue = xQueueCreate(QUEUE_SIZE, sizeof(struct dataSensor_st));
    }
    ESP_LOGI(__func__, "Create dataSensorSentToSD Queue Success.");

    //Creat dataSensorSentToScreen Queue
    dataSensorSentToScreen_queue = xQueueCreate(QUEUE_SIZE, sizeof(struct dataSensor_st));
    while(dataSensorSentToScreen_queue == NULL){
        ESP_LOGE(__func__, "Create dataSensorSentToScreen Queue failed.");
        ESP_LOGI(__func__, "Retry to create dataSensorSentToScreen Queue...");
        vTaskDelay(500/portTICK_RATE_MS);
        dataSensorSentToScreen_queue = xQueueCreate(QUEUE_SIZE, sizeof(struct dataSensor_st));
    }
    ESP_LOGI(__func__, "Create dataSensorSentToScreen Queue Success.");

    //Creat task to get data from sensors with 64KB stack memory and priority 25
    //Sample Period 5000ms
    xTaskCreate(getDataFromSensor_task, "GetDataSensor", (1024 * 64), NULL, (BaseType_t)25, &getDataFromSensorTask_handle);

//Creat task to update data on TFT screen
    xTaskCreate(updateScreen_task, "Update data on screen", (1024*16), NULL, (BaseType_t)10, &updateScreenTask_handle);

    // Create task to save data got from getDataFromSensor_task() to SD card (16Kb stack memory| priority 10)
    // Period 5000ms
    xTaskCreate(saveDataSensorToSDcard_task, "SaveDataSensorToSDcard", (1024*16), NULL, (BaseType_t)9, &saveDataSensorToSDcardTask_handle);

    //Creat task to read data from SD card
    //xTaskCreate(readDataSensorFromSDcard_task, "ReadDataSensorFromSDcard",(1024*16), NULL, (BaseType_t)10, &readDataSensorFromSDcardTask_handle);

}
