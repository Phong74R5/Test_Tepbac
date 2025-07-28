// mqtt_task.h
#ifndef MQTT_TASK_H
#define MQTT_TASK_H

#include <stdio.h>
#include <stddef.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "mqtt_client.h"
#include "global_data.h"
#include "driver/gpio.h"

void mqtt_task_pubsub(void *param);

#endif // MQTT_TASK_H