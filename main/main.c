#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "nvs_flash.h"
#include "esp_log.h"
#include "esp_psram.h"

#include "config.h"
#include "wifi.h"
#include "camera.h"
#include "servo.h"
#include "http_server.h"

static const char *TAG = "main";

// Track whether camera came up — stream handler checks this
bool g_camera_ok = false;

void app_main(void)
{
    // ── NVS ─────────────────────────────────────────────────────────────────
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES ||
        ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);
    ESP_LOGI(TAG, "NVS ready");

    // ── PSRAM check ─────────────────────────────────────────────────────────
    if (esp_psram_is_initialized()) {
        ESP_LOGI(TAG, "PSRAM: %zu KB available", esp_psram_get_size() / 1024);
    } else {
        ESP_LOGW(TAG, "PSRAM not detected – frame buffers will use DRAM");
    }

    // ── Servos ──────────────────────────────────────────────────────────────
    ESP_ERROR_CHECK(servo_init());
    servo_center_all();

    // ── Camera (non-fatal) ──────────────────────────────────────────────────
    // Use if/log instead of ESP_ERROR_CHECK so a missing/unsupported camera
    // doesn't abort() — WiFi + servo control still works without it.
    ret = camera_init();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Camera init failed (%s) — stream will be unavailable",
                 esp_err_to_name(ret));
        ESP_LOGE(TAG, "Check: 1) CONFIG_OV3660_SUPPORT=y in sdkconfig");
        ESP_LOGE(TAG, "       2) Camera FPC cable is seated");
        ESP_LOGE(TAG, "       3) Pin assignments in config.h match your board");
        g_camera_ok = false;
    } else {
        ESP_LOGI(TAG, "Camera ready");
        g_camera_ok = true;
    }

    // ── WiFi ────────────────────────────────────────────────────────────────
    ESP_ERROR_CHECK(wifi_sta_init());
    ESP_LOGI(TAG, "Board IP: %s", wifi_get_ip());

    // ── HTTP server ─────────────────────────────────────────────────────────
    ESP_ERROR_CHECK(http_server_start());

    ESP_LOGI(TAG, "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━");
    ESP_LOGI(TAG, " Open  http://%s/        (control page)", wifi_get_ip());
    ESP_LOGI(TAG, " Stream http://%s/stream  (%s)",
             wifi_get_ip(), g_camera_ok ? "live" : "CAMERA NOT READY");
    ESP_LOGI(TAG, " Servo  http://%s/servo?axis=pan&angle=90", wifi_get_ip());
    ESP_LOGI(TAG, "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━");

    while (true) {
        vTaskDelay(pdMS_TO_TICKS(10000));
    }
}
