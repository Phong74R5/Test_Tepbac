#include "oled_task.h"
#include "font.h"

static const char *TAG = "OLED_TASK";
static uint8_t ssd1306_buffer[SSD1306_BUFFER_SIZE];

// Initialize I2C master
static esp_err_t i2c_master_init(void) {
    i2c_config_t conf = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = I2C_MASTER_SDA_IO,
        .scl_io_num = I2C_MASTER_SCL_IO,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = I2C_MASTER_FREQ_HZ,
    };
    
    esp_err_t err = i2c_param_config(I2C_MASTER_NUM, &conf);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "I2C param config failed: %s", esp_err_to_name(err));
        return err;
    }
    
    err = i2c_driver_install(I2C_MASTER_NUM, conf.mode, 0, 0, 0);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "I2C driver install failed: %s", esp_err_to_name(err));
        return err;
    }
    
    ESP_LOGI(TAG, "I2C master initialized successfully");
    return ESP_OK;
}

// Write command to SSD1306
static esp_err_t ssd1306_write_command(uint8_t cmd) {
    uint8_t buffer[2] = {0x00, cmd};
    return i2c_master_write_to_device(I2C_MASTER_NUM, SSD1306_ADDRESS, 
                                     buffer, 2, pdMS_TO_TICKS(100));
}

// Write data to SSD1306
static esp_err_t ssd1306_write_data(uint8_t data) {
    uint8_t buffer[2] = {0x40, data};
    return i2c_master_write_to_device(I2C_MASTER_NUM, SSD1306_ADDRESS, 
                                     buffer, 2, pdMS_TO_TICKS(100));
}

// Initialize SSD1306 display
static esp_err_t ssd1306_init(void) {
    esp_err_t ret = i2c_master_init();
    if (ret != ESP_OK) {
        return ret;
    }

    // SSD1306 initialization sequence
    const uint8_t init_commands[] = {
        0xAE, // Display OFF
        0xD5, 0x80, // Set display clock divide ratio/oscillator frequency
        0xA8, 0x3F, // Set multiplex ratio (1 to 64)
        0xD3, 0x00, // Set display offset
        0x40, // Set start line address
        0xA1, // Set segment re-map
        0xC8, // Set COM output scan direction
        0xDA, 0x12, // Set COM pins hardware configuration
        0x81, 0x7F, // Set contrast control
        0xA4, // Disable entire display on
        0xA6, // Set normal display
        0x8D, 0x14, // Enable charge pump
        0xAF  // Display ON
    };

    for (int i = 0; i < sizeof(init_commands); i++) {
        ret = ssd1306_write_command(init_commands[i]);
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "Failed to send init command 0x%02X", init_commands[i]);
            return ret;
        }
    }

    ESP_LOGI(TAG, "SSD1306 initialized successfully");
    return ESP_OK;
}

// Update entire display
static esp_err_t ssd1306_update_screen(void) {
    for (uint8_t page = 0; page < SSD1306_PAGES; page++) {
        // Set page and column address
        ssd1306_write_command(0xB0 + page);
        ssd1306_write_command(0x00); // Lower column start address
        ssd1306_write_command(0x10); // Higher column start address
        
        // Write data for entire row
        for (uint8_t col = 0; col < SSD1306_WIDTH; col++) {
            esp_err_t ret = ssd1306_write_data(ssd1306_buffer[page * SSD1306_WIDTH + col]);
            if (ret != ESP_OK) {
                ESP_LOGE(TAG, "Failed to write display data");
                return ret;
            }
        }
    }
    return ESP_OK;
}

// Clear display buffer
static void ssd1306_clear_buffer(void) {
    memset(ssd1306_buffer, 0x00, sizeof(ssd1306_buffer));
}

// Draw single character (8x16 font)
static void ssd1306_draw_char_8x16(uint8_t x, uint8_t y, char c, const uint8_t *font) {
    if (x >= SSD1306_WIDTH || y >= SSD1306_HEIGHT) {
        return; // Out of bounds
    }
    
    uint16_t char_index = (c - 32) * 16;
    uint8_t page = y / 8;
    
    // Draw character in two pages (8x16 font spans 2 pages)
    for (uint8_t col = 0; col < 8 && (x + col) < SSD1306_WIDTH; col++) {
        if (page < SSD1306_PAGES) {
            ssd1306_buffer[(page * SSD1306_WIDTH) + x + col] = font[4 + char_index + col];
        }
        if ((page + 1) < SSD1306_PAGES) {
            ssd1306_buffer[((page + 1) * SSD1306_WIDTH) + x + col] = font[4 + char_index + 8 + col];
        }
    }
}

// Draw string using 8x16 font
static void ssd1306_draw_string_8x16(uint8_t x, uint8_t y, const char *str, const uint8_t *font) {
    while (*str && x < SSD1306_WIDTH) {
        ssd1306_draw_char_8x16(x, y, *str, font);
        x += font[1]; // Character width
        str++;
    }
}

// Update display with current sensor data
static void update_display_content(void) {
    char buffer[32];
    
    // Clear buffer
    ssd1306_clear_buffer();
    
    // Display temperature
    snprintf(buffer, sizeof(buffer), "Temp: %.1fC", temperature);
    ssd1306_draw_string_8x16(0, 0, buffer, ssd1306xled_font8x16);
    
    // Display humidity
    snprintf(buffer, sizeof(buffer), "Humidity: %.1f%%", humidity);
    ssd1306_draw_string_8x16(0, 16, buffer, ssd1306xled_font8x16);
    
    // Display WiFi status
    const char* wifi_status = wifi_connected ? "WiFi: Connected" : "WiFi: Disconnected";
    ssd1306_draw_string_8x16(0, 32, wifi_status, ssd1306xled_font8x16);
    
    // Display alarm status
    if (overheat_alarm) {
        ssd1306_draw_string_8x16(0, 48, "ALARM: OVERHEAT!", ssd1306xled_font8x16);
    }
    
    // Update screen
    ssd1306_update_screen();
}

// Main OLED task
void oled_task(void *pvParameters) {
    ESP_LOGI(TAG, "OLED task started");
    
    // Initialize display
    if (ssd1306_init() != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize SSD1306 display");
        vTaskDelete(NULL);
        return;
    }
    
    // Initial clear
    ssd1306_clear_buffer();
    ssd1306_update_screen();
    
    while (1) {
        update_display_content();
        vTaskDelay(pdMS_TO_TICKS(OLED_UPDATE_DELAY));
    }
}