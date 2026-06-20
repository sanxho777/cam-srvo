#pragma once
#include "esp_err.h"
#include <stdint.h>

typedef enum {
    SERVO_PAN  = 0,
    SERVO_TILT = 1,
} servo_id_t;

/**
 * @brief Initialise both LEDC-driven servo channels.
 */
esp_err_t servo_init(void);

/**
 * @brief Set servo position.
 * @param id     SERVO_PAN or SERVO_TILT
 * @param angle  0–180 degrees
 */
esp_err_t servo_set_angle(servo_id_t id, uint8_t angle);

/**
 * @brief Get current angle (cached, not read from hardware).
 */
uint8_t servo_get_angle(servo_id_t id);

/**
 * @brief Centre both servos (90°).
 */
esp_err_t servo_center_all(void);
