#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"

// Task headers
#include "wifi_config.h"
#include "dht11_task.h"
#include "oled_task.h"
#include "alarm_task.h"
#include "mqtt_task.h"

static const char *TAG = "MAIN";

// Task priorities
#define WIFI_TASK_PRIORITY 5
#define DHT11_TASK_PRIORITY 4
#define MQTT_TASK_PRIORITY 4
#define OLED_TASK_PRIORITY 3
#define ALARM_TASK_PRIORITY 3

// Task stack sizes
#define WIFI_TASK_STACK_SIZE 4096
#define DHT11_TASK_STACK_SIZE 2048
#define MQTT_TASK_STACK_SIZE 4096
#define OLED_TASK_STACK_SIZE 2048
#define ALARM_TASK_STACK_SIZE 2048

// Task creation with error checking
static void create_task_with_check(TaskFunction_t task_func, const char* task_name,
                                  uint32_t stack_size, UBaseType_t priority) {
    BaseType_t result = xTaskCreate(task_func, task_name, stack_size, NULL, priority, NULL);
    
    if (result == pdPASS) {
        ESP_LOGI(TAG, "Created task: %s", task_name);
    } else {
        ESP_LOGE(TAG, "Failed to create task: %s", task_name);
    }
}

void app_main(void) {

    // Create tasks in order of dependency
    // 1. WiFi task (highest priority - needed by MQTT)
    create_task_with_check(&wifi_task, "wifi_task", 
                          WIFI_TASK_STACK_SIZE, WIFI_TASK_PRIORITY);

    // 2. DHT11 sensor task (provides data for other tasks)
    create_task_with_check(&dht11_task, "dht11_sensor", 
                          DHT11_TASK_STACK_SIZE, DHT11_TASK_PRIORITY);

    // 3. MQTT task (depends on WiFi and sensor data)
    create_task_with_check(&mqtt_task_pubsub, "mqtt_client", 
                          MQTT_TASK_STACK_SIZE, MQTT_TASK_PRIORITY);

    // 4. OLED display task (shows system status)
    create_task_with_check(&oled_task, "oled_display", 
                          OLED_TASK_STACK_SIZE, OLED_TASK_PRIORITY);

    // 5. Alarm task (monitors temperature)
    create_task_with_check(&alarm_task, "temperature_alarm", 
                          ALARM_TASK_STACK_SIZE, ALARM_TASK_PRIORITY);
}