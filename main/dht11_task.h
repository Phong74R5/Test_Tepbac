// dht11_task.h
#ifndef DHT11_TASK_H
#define DHT11_TASK_H

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "global_data.h"
#include "driver/gpio.h"
#include "rom/ets_sys.h"
#include "esp_log.h"
#include <stdio.h>
#include <string.h>

void dht11_task(void *pvParameters);

#endif // DHT11_TASK_H