#ifndef GLOBAL_DATA_H
#define GLOBAL_DATA_H

#include <stdbool.h>

// Sensor data
extern float temperature;
extern float humidity;

// LED states
extern bool led1_state;
extern bool led2_state;

// System states
extern bool wifi_connected;
extern bool overheat_alarm;

// Temperature threshold for alarm
#define TEMPERATURE_THRESHOLD 40.0f

// GPIO pin definitions
#define LED1_GPIO 18
#define LED2_GPIO 19
#define BUZZER_GPIO 2
#define DHT11_GPIO 4

// I2C configuration
#define I2C_MASTER_NUM       I2C_NUM_0
#define I2C_MASTER_SDA_IO    21
#define I2C_MASTER_SCL_IO    22
#define I2C_MASTER_FREQ_HZ   100000
#define SSD1306_ADDRESS      0x3C

// WiFi configuration
#define WIFI_SSID_MAX_LEN 32
#define WIFI_PASS_MAX_LEN 64
#define UART_NUM UART_NUM_0

// Task delays (in milliseconds)
#define DHT11_READ_DELAY 2000
#define MQTT_PUBLISH_DELAY 10000
#define ALARM_CHECK_DELAY 500
#define OLED_UPDATE_DELAY 1000

#endif // GLOBAL_DATA_H