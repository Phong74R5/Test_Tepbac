// alarm_task.h
#ifndef ALARM_TASK_H
#define ALARM_TASK_H

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "global_data.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include <stdio.h>

void alarm_task(void *pvParameters);

#endif // ALARM_TASK_H