#include "servo.h"
#include "config.h"
#include "esp_log.h"
#include "driver/ledc.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "servo";

static uint8_t s_angle[2] = {90, 90};

// ─── duty calculation ────────────────────────────────────────────────────────

static uint32_t angle_to_duty(uint8_t angle)
{
    // Use 64-bit math to avoid overflow
    // period = 20000 µs (50 Hz), max_duty = 16383 (14-bit)
    const uint32_t period_us = 1000000u / SERVO_FREQ_HZ;
    const uint32_t max_duty  = (1u << SERVO_RESOLUTION) - 1u;

    uint32_t pulse_us = SERVO_MIN_US +
        ((uint32_t)angle * (SERVO_MAX_US - SERVO_MIN_US)) / 180u;

    if (pulse_us < SERVO_MIN_US) pulse_us = SERVO_MIN_US;
    if (pulse_us > SERVO_MAX_US) pulse_us = SERVO_MAX_US;

    uint32_t duty = (uint32_t)(((uint64_t)pulse_us * max_duty) / period_us);
    ESP_LOGD(TAG, "angle=%d pulse_us=%" PRIu32 " duty=%" PRIu32,
             angle, pulse_us, duty);
    return duty;
}

// ─── public API ─────────────────────────────────────────────────────────────

esp_err_t servo_init(void)
{
    ledc_timer_config_t timer_cfg = {
        .speed_mode      = SERVO_SPEED_MODE,
        .timer_num       = SERVO_TIMER,
        .duty_resolution = SERVO_RESOLUTION,
        .freq_hz         = SERVO_FREQ_HZ,
        .clk_cfg         = LEDC_AUTO_CLK,
    };
    ESP_ERROR_CHECK(ledc_timer_config(&timer_cfg));

    ledc_channel_config_t pan_cfg = {
        .gpio_num   = SERVO_PAN_GPIO,
        .speed_mode = SERVO_SPEED_MODE,
        .channel    = SERVO_PAN_CHANNEL,
        .timer_sel  = SERVO_TIMER,
        .duty       = angle_to_duty(90),
        .hpoint     = 0,
        .intr_type  = LEDC_INTR_DISABLE,
    };
    ESP_ERROR_CHECK(ledc_channel_config(&pan_cfg));

    ledc_channel_config_t tilt_cfg = {
        .gpio_num   = SERVO_TILT_GPIO,
        .speed_mode = SERVO_SPEED_MODE,
        .channel    = SERVO_TILT_CHANNEL,
        .timer_sel  = SERVO_TIMER,
        .duty       = angle_to_duty(90),
        .hpoint     = 0,
        .intr_type  = LEDC_INTR_DISABLE,
    };
    ESP_ERROR_CHECK(ledc_channel_config(&tilt_cfg));

    // Fade service is required for ledc_set_duty_and_update()
    ESP_ERROR_CHECK(ledc_fade_func_install(0));

    ESP_LOGI(TAG, "Servos ready – Pan GPIO%d, Tilt GPIO%d",
             SERVO_PAN_GPIO, SERVO_TILT_GPIO);
    return ESP_OK;
}

esp_err_t servo_set_angle(servo_id_t id, uint8_t angle)
{
    if (angle > 180) angle = 180;

    ledc_channel_t ch = (id == SERVO_PAN) ? SERVO_PAN_CHANNEL
                                           : SERVO_TILT_CHANNEL;
    uint32_t duty = angle_to_duty(angle);

    // Use the fade ISR path for a guaranteed atomic duty update.
    // ledc_set_duty_with_hpoint + ledc_update_duty can miss a PWM
    // cycle boundary on ESP32-S3 low-speed mode, causing the update
    // to silently drop. ledc_set_duty_and_update() avoids this.
    esp_err_t ret = ledc_set_duty_and_update(SERVO_SPEED_MODE, ch, duty, 0);
    if (ret == ESP_OK) {
        s_angle[id] = angle;
        ESP_LOGI(TAG, "%s → %d° (duty=%" PRIu32 ")",
                 id == SERVO_PAN ? "Pan" : "Tilt", angle, duty);
    } else {
        ESP_LOGE(TAG, "servo_set_angle failed: %s", esp_err_to_name(ret));
    }
    return ret;
}

uint8_t servo_get_angle(servo_id_t id)
{
    return s_angle[id];
}

esp_err_t servo_center_all(void)
{
    esp_err_t r  = servo_set_angle(SERVO_PAN,  90);
              r |= servo_set_angle(SERVO_TILT, 90);
    return r;
}
