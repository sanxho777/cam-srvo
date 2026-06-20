#pragma once
#include "esp_err.h"

/**
 * @brief Start the HTTP server.
 *        Registers all URI handlers (stream, controls, index page).
 */
esp_err_t http_server_start(void);
