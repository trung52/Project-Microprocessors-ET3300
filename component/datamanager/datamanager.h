#ifndef __DATAMANAGER_H__
#define __DATAMANAGER_H__

#include "esp_err.h"
#include "esp_log.h"
#include "sdkconfig.h"
#include <string.h>

struct dataSensor_st
{
    uint64_t timeStamp;

    uint32_t pm1_0;
    uint32_t pm2_5;
    uint32_t pm10;

    uint32_t co2;

    float temperature;
    float humidity;
    float pressure;
};


#endif