#pragma once
#include "esp_err.h"
#include "esp_camera.h"

/**
 * @brief Initialise the OV3660 camera.
 *        Must be called after NVS and PSRAM are ready.
 * @return ESP_OK on success.
 */
esp_err_t camera_init(void);

/**
 * @brief Capture one JPEG frame.
 *        Caller must release the frame buffer with camera_fb_return().
 */
camera_fb_t *camera_capture(void);

/**
 * @brief Return a previously captured frame buffer to the driver.
 */
void camera_fb_return(camera_fb_t *fb);
