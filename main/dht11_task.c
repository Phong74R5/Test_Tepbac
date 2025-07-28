#include "dht11_task.h"

#define DHT_LOG_TAG "DHT11"
#define DHT_START_SIGNAL_DURATION 18000
#define DHT_RESPONSE_WAIT_TIME 80
#define DHT_BIT_HIGH_THRESHOLD 40
#define DHT_TIMEOUT_US 60
#define DHT_DATA_BITS 40
#define DHT_BYTES 5

// Wait for pin to reach specified state with timeout
static int wait_for_state(int pin, int state, int timeout_us) {
    int count = 0;
    while (gpio_get_level(pin) != state) {
        if (count++ >= timeout_us) {
            return -1;
        }
        ets_delay_us(1);
    }
    return count;
}

// Send start signal to DHT11
static void send_start_signal(int pin, int duration_us) {
    gpio_set_direction(pin, GPIO_MODE_OUTPUT);
    gpio_set_level(pin, 0);
    ets_delay_us(duration_us);
    gpio_set_level(pin, 1);
}

// Read data from DHT11 sensor
static int dht11_read_data(int pin) {
    uint8_t data[DHT_BYTES] = {0};

    // Send start signal
    send_start_signal(pin, DHT_START_SIGNAL_DURATION);
    gpio_set_direction(pin, GPIO_MODE_INPUT);

    // Wait for DHT11 response
    ets_delay_us(DHT_RESPONSE_WAIT_TIME);

    // Read 40 bits (5 bytes) of data
    for (int byte_index = 0; byte_index < DHT_BYTES; byte_index++) {
        for (int bit_index = 0; bit_index < 8; bit_index++) {
            // Wait for bit start (high state)
            if (wait_for_state(pin, 1, DHT_TIMEOUT_US) == -1) {
                return -1;
            }
            
            // Measure high duration to determine bit value
            int duration = wait_for_state(pin, 0, DHT_TIMEOUT_US);
            if (duration == -1) {
                return -1;
            }
            
            // Set bit based on duration (1 if duration > threshold)
            if (duration > DHT_BIT_HIGH_THRESHOLD) {
                data[byte_index] |= (1 << (7 - bit_index));
            }
        }
    }

    // Verify checksum
    uint8_t checksum = data[0] + data[1] + data[2] + data[3];
    if (checksum != data[4]) {
        ESP_LOGW(DHT_LOG_TAG, "Checksum error: calculated=%d, received=%d", 
                 checksum, data[4]);
        return -1;
    }

    // Update global variables
    humidity = data[0] + data[1] / 10.0f;
    temperature = data[2] + data[3] / 10.0f;
    
    return 0;
}

// Initialize DHT11 GPIO
static void dht11_init(int pin) {
    gpio_set_pull_mode(pin, GPIO_PULLUP_ONLY);
}

// Main DHT11 task
void dht11_task(void *pvParameters) {
    dht11_init(DHT11_GPIO);

    while (1) {
        if (dht11_read_data(DHT11_GPIO) == 0) {
            ESP_LOGI(DHT_LOG_TAG, "Temperature: %.1f°C | Humidity: %.1f%%", 
                     temperature, humidity);
        } else {
            ESP_LOGW(DHT_LOG_TAG, "Failed to read DHT11 sensor");
            temperature = -99.0f;
            humidity = -99.0f;
        }

        vTaskDelay(pdMS_TO_TICKS(DHT11_READ_DELAY));
    }
}