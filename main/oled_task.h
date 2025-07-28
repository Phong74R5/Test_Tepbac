#ifndef OLED_TASK_H
#define OLED_TASK_H

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "global_data.h"
#include "driver/i2c.h"
#include "esp_err.h"
#include "esp_log.h"
#include <string.h>

// SSD1306 display constants
#define SSD1306_WIDTH 128
#define SSD1306_HEIGHT 64
#define SSD1306_PAGES 8
#define SSD1306_BUFFER_SIZE (SSD1306_WIDTH * SSD1306_PAGES)

// Function prototype
void oled_task(void *pvParameters);

#endif // OLED_TASK_H