#include "camera.h"
#include "config.h"
#include "esp_log.h"

static const char *TAG = "camera";

esp_err_t camera_init(void)
{
    camera_config_t cfg = {
        .pin_pwdn      = CAM_PIN_PWDN,
        .pin_reset     = CAM_PIN_RESET,
        .pin_xclk      = CAM_PIN_XCLK,
        .pin_sccb_sda  = CAM_PIN_SIOD,
        .pin_sccb_scl  = CAM_PIN_SIOC,

        .pin_d0        = CAM_PIN_D0,
        .pin_d1        = CAM_PIN_D1,
        .pin_d2        = CAM_PIN_D2,
        .pin_d3        = CAM_PIN_D3,
        .pin_d4        = CAM_PIN_D4,
        .pin_d5        = CAM_PIN_D5,
        .pin_d6        = CAM_PIN_D6,
        .pin_d7        = CAM_PIN_D7,

        .pin_vsync     = CAM_PIN_VSYNC,
        .pin_href      = CAM_PIN_HREF,
        .pin_pclk      = CAM_PIN_PCLK,

        .xclk_freq_hz  = 20000000,
        .ledc_timer    = LEDC_TIMER_1,
        .ledc_channel  = LEDC_CHANNEL_2,

        .pixel_format  = PIXFORMAT_JPEG,
        .frame_size    = CAM_FRAME_SIZE,
        .jpeg_quality  = CAM_JPEG_QUALITY,

        // 4 frame buffers in PSRAM — camera DMA can always write ahead
        // while the HTTP task is sending the previous frame, eliminating stalls
        .fb_count      = 4,
        .fb_location   = CAMERA_FB_IN_PSRAM,

        // LATEST: always return the most recently captured frame.
        // Prevents the stream from showing stale buffered frames when
        // the HTTP sender falls behind momentarily.
        .grab_mode     = CAMERA_GRAB_LATEST,
    };

    esp_err_t ret = esp_camera_init(&cfg);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Camera init failed: %s", esp_err_to_name(ret));
        return ret;
    }

    sensor_t *s = esp_camera_sensor_get();
    if (s) {
        s->set_vflip(s, 1);
        s->set_hmirror(s, 0);

        // OV3660 image quality tuning
        s->set_brightness(s, 1);
        s->set_saturation(s, 0);
        s->set_sharpness(s, 1);       // slight edge enhancement
        s->set_denoise(s, 1);         // reduce noise at QVGA

        // Auto controls
        s->set_whitebal(s, 1);
        s->set_awb_gain(s, 1);
        s->set_wb_mode(s, 0);         // auto WB mode
        s->set_exposure_ctrl(s, 1);
        s->set_aec2(s, 1);            // improved AEC algorithm
        s->set_ae_level(s, 0);
        s->set_gain_ctrl(s, 1);
        s->set_agc_gain(s, 0);
        s->set_gainceiling(s, GAINCEILING_4X); // cap gain to reduce noise

        ESP_LOGI(TAG, "Sensor: %s",
                 s->id.PID == 0x3660 ? "OV3660 ✓" : "unknown");
    }

    ESP_LOGI(TAG, "Camera ready — %d frame buffers, GRAB_LATEST", cfg.fb_count);
    return ESP_OK;
}

camera_fb_t *camera_capture(void)
{
    return esp_camera_fb_get();
}

void camera_fb_return(camera_fb_t *fb)
{
    esp_camera_fb_return(fb);
}
