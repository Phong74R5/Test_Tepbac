#include "wifi_config.h"

static const char *TAG = "WIFI_CONFIG";
static char ssid[WIFI_SSID_MAX_LEN] = {0};
static char password[WIFI_PASS_MAX_LEN] = {0};

// Read user input from UART with timeout
static void read_uart_input(char *buffer, int max_len, const char* prompt) {
    printf("%s", prompt);
    fflush(stdout);
    
    int len = 0;
    while (len < max_len - 1) {
        int bytes_read = uart_read_bytes(UART_NUM, (uint8_t*)&buffer[len], 1, 
                                        pdMS_TO_TICKS(30000)); // 30s timeout
        
        if (bytes_read > 0) {
            if (buffer[len] == '\n' || buffer[len] == '\r') {
                buffer[len] = '\0';
                printf("\n");
                break;
            }
            printf("%c", buffer[len]); // Echo character
            len++;
        } else {
            ESP_LOGW(TAG, "UART read timeout");
            break;
        }
    }
    buffer[len] = '\0';
}

// WiFi event handler
static void wifi_event_handler(void* arg, esp_event_base_t event_base,
                              int32_t event_id, void* event_data) {
    switch (event_id) {
        case WIFI_EVENT_STA_START:
            ESP_LOGI(TAG, "WiFi station started, connecting...");
            esp_wifi_connect();
            break;
            
        case WIFI_EVENT_STA_DISCONNECTED:
            ESP_LOGW(TAG, "WiFi disconnected, attempting reconnect...");
            wifi_connected = false;
            esp_wifi_connect();
            break;
            
        default:
            break;
    }
}

// IP event handler
static void ip_event_handler(void* arg, esp_event_base_t event_base,
                            int32_t event_id, void* event_data) {
    if (event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
        ESP_LOGI(TAG, "WiFi connected successfully! IP: " IPSTR, 
                 IP2STR(&event->ip_info.ip));
        wifi_connected = true;
    }
}

// Initialize UART for user input
static void init_uart(void) {
    uart_config_t uart_config = {
        .baud_rate = 115200,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE
    };
    
    ESP_ERROR_CHECK(uart_param_config(UART_NUM, &uart_config));
    ESP_ERROR_CHECK(uart_driver_install(UART_NUM, 1024, 0, 0, NULL, 0));
}

// Get WiFi credentials from user
static void get_wifi_credentials(void) {
    printf("\n=== WiFi Configuration ===\n");
    
    read_uart_input(ssid, WIFI_SSID_MAX_LEN, "Enter WiFi SSID: ");
    if (strlen(ssid) == 0) {
        ESP_LOGE(TAG, "SSID cannot be empty!");
        return;
    }
    
    read_uart_input(password, WIFI_PASS_MAX_LEN, "Enter WiFi Password: ");
    
    ESP_LOGI(TAG, "WiFi credentials received - SSID: %s", ssid);
}

// Connect to WiFi with provided credentials
static void connect_to_wifi(void) {
    wifi_config_t wifi_config = {0};
    
    strncpy((char*)wifi_config.sta.ssid, ssid, sizeof(wifi_config.sta.ssid) - 1);
    strncpy((char*)wifi_config.sta.password, password, sizeof(wifi_config.sta.password) - 1);
    
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());
    
    ESP_LOGI(TAG, "Attempting to connect to WiFi network: %s", ssid);
}

// Initialize WiFi station mode
static void init_wifi_station(void) {
    // Initialize NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    // Initialize network interface
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_sta();

    // Initialize WiFi
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    // Register event handlers
    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
                                                        ESP_EVENT_ANY_ID,
                                                        &wifi_event_handler,
                                                        NULL, NULL));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT,
                                                        IP_EVENT_STA_GOT_IP,
                                                        &ip_event_handler,
                                                        NULL, NULL));
}

// Main WiFi configuration task
void wifi_task(void *param) {
    ESP_LOGI(TAG, "WiFi configuration task started");
    
    // Initialize components
    init_uart();
    init_wifi_station();
    
    // Get credentials and connect
    get_wifi_credentials();
    
    if (strlen(ssid) > 0) {
        connect_to_wifi();
        
        // Wait for connection
        int retry_count = 0;
        const int max_retries = 30; // 30 seconds timeout
        
        while (!wifi_connected && retry_count < max_retries) {
            ESP_LOGI(TAG, "Waiting for WiFi connection... (%d/%d)", 
                     retry_count + 1, max_retries);
            vTaskDelay(pdMS_TO_TICKS(1000));
            retry_count++;
        }
        
        if (wifi_connected) {
            ESP_LOGI(TAG, "WiFi configuration completed successfully");
        } else {
            ESP_LOGE(TAG, "Failed to connect to WiFi after %d seconds", max_retries);
        }
    } else {
        ESP_LOGE(TAG, "Invalid WiFi credentials provided");
    }
    
    // Task cleanup
    ESP_LOGI(TAG, "WiFi configuration task terminating");
    vTaskDelete(NULL);
}