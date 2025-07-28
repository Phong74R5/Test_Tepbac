#include "alarm_task.h"

static const char *TAG = "ALARM_TASK";

// Initialize buzzer GPIO
static void init_buzzer_gpio(void) {
    gpio_set_direction(BUZZER_GPIO, GPIO_MODE_OUTPUT);
    gpio_set_level(BUZZER_GPIO, 0);
    ESP_LOGI(TAG, "Buzzer GPIO initialized on pin %d", BUZZER_GPIO);
}

// Check if temperature exceeds threshold
static bool is_temperature_critical(float temp) {
    return (temp >= TEMPERATURE_THRESHOLD);
}

// Control buzzer based on temperature
static void control_buzzer(bool activate) {
    static bool previous_state = false;
    
    if (activate != previous_state) {
        gpio_set_level(BUZZER_GPIO, activate ? 1 : 0);
        
        if (activate) {
            ESP_LOGW(TAG, "OVERHEAT ALARM! Temperature: %.1f°C (Threshold: %.1f°C)", 
                     temperature, TEMPERATURE_THRESHOLD);
        } else {
            ESP_LOGI(TAG, "Temperature normalized: %.1f°C", temperature);
        }
        
        previous_state = activate;
        overheat_alarm = activate;
    }
}

// Main alarm monitoring task
void alarm_task(void *pvParameters) {
    init_buzzer_gpio();
    
    ESP_LOGI(TAG, "Alarm monitoring started (Threshold: %.1f°C)", TEMPERATURE_THRESHOLD);

    while (1) {
        bool critical = is_temperature_critical(temperature);
        control_buzzer(critical);
        
        vTaskDelay(pdMS_TO_TICKS(ALARM_CHECK_DELAY));
    }
}