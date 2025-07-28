// wifi_config.h
#ifndef WIFI_CONFIG_H
#define WIFI_CONFIG_H

#include <string.h>
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "driver/uart.h"
#include "global_data.h"

void wifi_task(void *param);

#endif // WIFI_CONFIG_H