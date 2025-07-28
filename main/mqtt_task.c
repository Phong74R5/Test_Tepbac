#include "mqtt_task.h"

static const char *TAG = "MQTT_TASK";
static esp_mqtt_client_handle_t client = NULL;

// Update LED states based on global variables
static void update_led_states(void) {
    gpio_set_level(LED1_GPIO, led1_state ? 1 : 0);
    gpio_set_level(LED2_GPIO, led2_state ? 1 : 0);
    ESP_LOGI(TAG, "LED states updated: LED1=%d, LED2=%d", led1_state, led2_state);
}

// Initialize LED GPIOs
static void init_led_gpios(void) {
    gpio_set_direction(LED1_GPIO, GPIO_MODE_OUTPUT);
    gpio_set_direction(LED2_GPIO, GPIO_MODE_OUTPUT);
    update_led_states();
}

// Publish sensor data to MQTT
static void mqtt_publish_sensor_data(float temp, float hum) {
    char payload[16];
    
    // Publish temperature
    snprintf(payload, sizeof(payload), "%.1f", temp);
    esp_mqtt_client_publish(client, CONFIG_FEED_TEMP, payload, 0, 1, 0);
    
    // Publish humidity
    snprintf(payload, sizeof(payload), "%.1f", hum);
    esp_mqtt_client_publish(client, CONFIG_FEED_HUMID, payload, 0, 1, 0);
    
    ESP_LOGI(TAG, "Published sensor data: Temp=%.1f°C, Humidity=%.1f%%", temp, hum);
}

// Process LED control command
static void process_led_command(const char* topic, const char* data, 
                               size_t topic_len, size_t data_len) {
    bool new_state = (strncmp(data, "1", data_len) == 0);
    
    if (strncmp(topic, CONFIG_FEED_LED1, topic_len) == 0) {
        led1_state = new_state;
        ESP_LOGI(TAG, "LED1 set to: %d", led1_state);
    } 
    else if (strncmp(topic, CONFIG_FEED_LED2, topic_len) == 0) {
        led2_state = new_state;
        ESP_LOGI(TAG, "LED2 set to: %d", led2_state);
    }
    
    update_led_states();
}

// Subscribe to LED control topics
static void subscribe_to_topics(void) {
    esp_mqtt_client_subscribe(client, CONFIG_FEED_LED1, 1);
    esp_mqtt_client_subscribe(client, CONFIG_FEED_LED2, 1);
    ESP_LOGI(TAG, "Subscribed to LED control topics");
}

// MQTT event handler
static void mqtt_event_handler(void *handler_args, esp_event_base_t base, 
                              int32_t event_id, void *event_data) {
    esp_mqtt_event_handle_t event = event_data;

    switch ((esp_mqtt_event_id_t)event_id) {
        case MQTT_EVENT_CONNECTED:
            ESP_LOGI(TAG, "MQTT connected successfully");
            subscribe_to_topics();
            break;

        case MQTT_EVENT_DATA:
            ESP_LOGI(TAG, "Received MQTT data on topic: %.*s | data: %.*s",
                     event->topic_len, event->topic,
                     event->data_len, event->data);
            
            process_led_command(event->topic, event->data, 
                              event->topic_len, event->data_len);
            break;

        case MQTT_EVENT_DISCONNECTED:
            ESP_LOGW(TAG, "MQTT disconnected");
            break;

        case MQTT_EVENT_ERROR:
            ESP_LOGE(TAG, "MQTT error occurred");
            break;

        default:
            ESP_LOGD(TAG, "Unhandled MQTT event: %d", event_id);
            break;
    }
}

// Initialize MQTT client
static esp_mqtt_client_handle_t init_mqtt_client(void) {
    esp_mqtt_client_config_t mqtt_cfg = {
        .uri = CONFIG_BROKER_URI,
        .username = CONFIG_USERNAME,
        .password = CONFIG_AIO_KEY
    };

    esp_mqtt_client_handle_t mqtt_client = esp_mqtt_client_init(&mqtt_cfg);
    if (mqtt_client == NULL) {
        ESP_LOGE(TAG, "Failed to initialize MQTT client");
        return NULL;
    }

    esp_mqtt_client_register_event(mqtt_client, ESP_EVENT_ANY_ID, 
                                  mqtt_event_handler, NULL);
    return mqtt_client;
}

// Wait for WiFi connection
static void wait_for_wifi_connection(void) {
    while (!wifi_connected) {
        ESP_LOGI(TAG, "Waiting for WiFi connection...");
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
    ESP_LOGI(TAG, "WiFi connection established");
}

// Main MQTT task
void mqtt_task_pubsub(void *param) {
    // Initialize hardware
    init_led_gpios();
    
    // Wait for WiFi connection
    wait_for_wifi_connection();

    // Initialize and start MQTT client
    client = init_mqtt_client();
    if (client == NULL) {
        ESP_LOGE(TAG, "Failed to create MQTT client, terminating task");
        vTaskDelete(NULL);
        return;
    }

    esp_mqtt_client_start(client);
    ESP_LOGI(TAG, "MQTT client started");

    // Main publishing loop
    while (wifi_connected) {
        mqtt_publish_sensor_data(temperature, humidity);
        vTaskDelay(pdMS_TO_TICKS(MQTT_PUBLISH_DELAY));
    }

    // Cleanup when WiFi disconnects
    ESP_LOGW(TAG, "WiFi disconnected. Stopping MQTT client and task.");
    esp_mqtt_client_stop(client);
    esp_mqtt_client_destroy(client);
    vTaskDelete(NULL);
}