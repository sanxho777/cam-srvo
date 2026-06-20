#pragma once
#include "esp_err.h"

/**
 * @brief Connect to WiFi (station mode).  Blocks until connected or fails.
 * @return ESP_OK when an IP is obtained.
 */
esp_err_t wifi_sta_init(void);

/**
 * @brief Returns the board's IP address as a string (e.g. "192.168.1.42").
 *        Valid after wifi_sta_init() returns ESP_OK.
 */
const char *wifi_get_ip(void);
