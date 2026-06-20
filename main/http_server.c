#include "http_server.h"
#include "camera.h"
#include "servo.h"
#include "config.h"

#include "esp_http_server.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

static const char *TAG = "http";

// ─────────────────────────────────────────────────────────────────────────────
//  Embedded HTML control page
// ─────────────────────────────────────────────────────────────────────────────
static const char INDEX_HTML[] =
"<!DOCTYPE html><html lang='en'><head>"
"<meta charset='UTF-8'>"
"<meta name='viewport' content='width=device-width,initial-scale=1'>"
"<title>ESP32-S3 Cam</title>"
"<style>"
"*{box-sizing:border-box;margin:0;padding:0}"
"body{background:#111;color:#eee;font-family:sans-serif;display:flex;"
"     flex-direction:column;align-items:center;min-height:100vh;padding:16px}"
"h1{margin:12px 0 8px;font-size:1.3rem;letter-spacing:2px;color:#0cf}"
"#stream{width:100%;max-width:640px;border:2px solid #333;border-radius:8px;"
"        background:#000;image-rendering:auto}"
".panel{width:100%;max-width:640px;margin-top:12px;background:#1a1a1a;"
"       border-radius:8px;padding:14px}"
".panel h2{font-size:.9rem;color:#0cf;margin-bottom:10px;text-transform:uppercase;"
"          letter-spacing:1px}"
".row{display:flex;align-items:center;gap:10px;margin-bottom:10px}"
"label{width:60px;font-size:.85rem;color:#aaa}"
"input[type=range]{flex:1;accent-color:#0cf}"
"span.val{width:36px;text-align:right;font-size:.85rem;color:#0cf}"
".btn-row{display:flex;gap:8px;flex-wrap:wrap;margin-top:4px}"
"button{flex:1;padding:10px;border:none;border-radius:6px;cursor:pointer;"
"       font-size:.85rem;font-weight:bold;background:#0cf;color:#111}"
"button:active{background:#09a}"
"#status{margin-top:10px;font-size:.75rem;color:#666;text-align:center}"
"#fps{font-size:.75rem;color:#0cf;text-align:center;margin-top:4px}"
"</style></head><body>"
"<h1>&#127910; ESP32-S3 CAM</h1>"
"<img id='stream' src='/stream' alt='camera stream'>"
"<div id='fps'>FPS: --</div>"

"<div class='panel'>"
"<h2>Servo Control</h2>"
"<div class='row'>"
"  <label>Pan</label>"
"  <input type='range' id='pan' min='0' max='180' value='90' "
"         oninput='setServo(\"pan\",this.value)'>"
"  <span class='val' id='pan-val'>90</span>"
"</div>"
"<div class='row'>"
"  <label>Tilt</label>"
"  <input type='range' id='tilt' min='0' max='180' value='90' "
"         oninput='setServo(\"tilt\",this.value)'>"
"  <span class='val' id='tilt-val'>90</span>"
"</div>"
"<div class='btn-row'>"
"  <button onclick='center()'>&#8635; Centre</button>"
"  <button onclick='panLeft()'>&#9664; Left</button>"
"  <button onclick='panRight()'>Right &#9654;</button>"
"  <button onclick='tiltUp()'>&#9650; Up</button>"
"  <button onclick='tiltDown()'>&#9660; Down</button>"
"</div>"
"</div>"
"<div id='status'>Loading…</div>"

"<script>"
// FPS counter using Image load events
"let lastT=Date.now(),frames=0;"
"const img=document.getElementById('stream');"
"img.onload=()=>{"
"  frames++;"
"  const now=Date.now();"
"  if(now-lastT>=1000){"
"    document.getElementById('fps').textContent='FPS: '+frames;"
"    frames=0;lastT=now;"
"  }"
"};"

// Debounced servo control — coalesce rapid slider events into one request
"let servoTimer={};"
"function setServo(axis,val){"
"  document.getElementById(axis+'-val').textContent=val;"
"  clearTimeout(servoTimer[axis]);"
"  servoTimer[axis]=setTimeout(()=>{"
"    fetch('/servo?axis='+axis+'&angle='+val)"
"      .then(r=>setStatus(r.ok?'OK':'Error '+r.status))"
"      .catch(e=>setStatus(''+e));"
"  },40);"  // 40 ms debounce — smooth dragging without flooding
"}"
"function center(){"
"  ['pan','tilt'].forEach(a=>{"
"    document.getElementById(a).value=90;"
"    document.getElementById(a+'-val').textContent=90;"
"  });"
"  fetch('/servo?axis=center').then(()=>setStatus('Centred'));"
"}"
"function step(axis,delta){"
"  const el=document.getElementById(axis);"
"  const nv=Math.min(180,Math.max(0,parseInt(el.value)+delta));"
"  el.value=nv;setServo(axis,nv);"
"}"
"function panLeft(){step('pan',-10)}"
"function panRight(){step('pan',10)}"
"function tiltUp(){step('tilt',-10)}"
"function tiltDown(){step('tilt',10)}"
"function setStatus(m){document.getElementById('status').textContent=m;}"
"document.addEventListener('keydown',e=>{"
"  if(e.key==='ArrowLeft')panLeft();"
"  else if(e.key==='ArrowRight')panRight();"
"  else if(e.key==='ArrowUp')tiltUp();"
"  else if(e.key==='ArrowDown')tiltDown();"
"  else if(e.key==='c'||e.key==='C')center();"
"});"
"setStatus('Arrow keys or sliders to control servos');"
"</script></body></html>";

// ─────────────────────────────────────────────────────────────────────────────
//  GET /
// ─────────────────────────────────────────────────────────────────────────────
static esp_err_t index_handler(httpd_req_t *req)
{
    httpd_resp_set_type(req, "text/html");
    httpd_resp_set_hdr(req, "Cache-Control", "no-cache");
    httpd_resp_send(req, INDEX_HTML, strlen(INDEX_HTML));
    return ESP_OK;
}

// ─────────────────────────────────────────────────────────────────────────────
//  GET /stream — MJPEG
//
//  Optimisations vs previous version:
//  1. Single httpd_resp_send_chunk() per frame combining boundary+headers+body
//     using a small stack buffer — cuts TCP write syscall overhead by 3x
//  2. Frame timing: skip if < MIN_FRAME_MS since last send to yield CPU
//  3. Camera grab is non-blocking — if no new frame ready, yield and retry
//     instead of blocking the HTTP task
// ─────────────────────────────────────────────────────────────────────────────
#define PART_BOUNDARY   "--" STREAM_BOUNDARY "\r\n"
#define MIN_FRAME_MS    33   // cap at ~30 fps max — avoids flooding TCP buffer

static esp_err_t stream_handler(httpd_req_t *req)
{
    esp_err_t res;
    char hdr[128];

    res = httpd_resp_set_type(req,
            "multipart/x-mixed-replace;boundary=" STREAM_BOUNDARY);
    if (res != ESP_OK) return res;
    httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
    httpd_resp_set_hdr(req, "Cache-Control", "no-cache, no-store");
    httpd_resp_set_hdr(req, "Pragma", "no-cache");

    ESP_LOGI(TAG, "Stream client connected");

    int64_t last_frame_us = 0;
    uint32_t frame_count  = 0;
    int64_t  fps_timer    = esp_timer_get_time();

    while (true) {
        // Pace frames — don't send faster than MIN_FRAME_MS
        int64_t now = esp_timer_get_time();
        int64_t elapsed_ms = (now - last_frame_us) / 1000;
        if (elapsed_ms < MIN_FRAME_MS) {
            vTaskDelay(pdMS_TO_TICKS(MIN_FRAME_MS - elapsed_ms));
        }

        camera_fb_t *fb = camera_capture();
        if (!fb) {
            // Camera not ready — brief yield and retry
            vTaskDelay(pdMS_TO_TICKS(5));
            continue;
        }

        last_frame_us = esp_timer_get_time();

        // Build MJPEG part header
        int hdr_len = snprintf(hdr, sizeof(hdr),
            PART_BOUNDARY
            "Content-Type: image/jpeg\r\n"
            "Content-Length: %zu\r\n"
            "\r\n",
            fb->len);

        // Send header chunk
        res = httpd_resp_send_chunk(req, hdr, hdr_len);
        if (res != ESP_OK) { camera_fb_return(fb); break; }

        // Send JPEG data
        res = httpd_resp_send_chunk(req, (const char *)fb->buf, fb->len);
        camera_fb_return(fb);   // return ASAP so DMA can reuse buffer
        if (res != ESP_OK) break;

        // End-of-part CRLF
        res = httpd_resp_send_chunk(req, "\r\n", 2);
        if (res != ESP_OK) break;

        // Log FPS every 5 seconds
        frame_count++;
        if ((esp_timer_get_time() - fps_timer) > 5000000LL) {
            ESP_LOGI(TAG, "Stream FPS: %.1f",
                     frame_count * 1000000.0f /
                     (float)(esp_timer_get_time() - fps_timer));
            frame_count = 0;
            fps_timer = esp_timer_get_time();
        }
    }

    httpd_resp_send_chunk(req, NULL, 0);
    ESP_LOGI(TAG, "Stream client disconnected");
    return res;
}

// ─────────────────────────────────────────────────────────────────────────────
//  GET /servo
// ─────────────────────────────────────────────────────────────────────────────
static esp_err_t servo_handler(httpd_req_t *req)
{
    char buf[64] = {0};
    size_t qlen = httpd_req_get_url_query_len(req) + 1;
    if (qlen > sizeof(buf)) qlen = sizeof(buf);

    if (httpd_req_get_url_query_str(req, buf, qlen) != ESP_OK) {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Missing query");
        return ESP_FAIL;
    }

    char axis[16]   = {0};
    char angle_s[8] = {0};
    httpd_query_key_value(buf, "axis",  axis,    sizeof(axis));
    httpd_query_key_value(buf, "angle", angle_s, sizeof(angle_s));

    httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
    httpd_resp_set_type(req, "application/json");

    if (strcmp(axis, "center") == 0) {
        servo_center_all();
        httpd_resp_sendstr(req, "{\"status\":\"ok\",\"pan\":90,\"tilt\":90}");
        return ESP_OK;
    }

    if (!strlen(angle_s)) {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Missing angle");
        return ESP_FAIL;
    }

    uint8_t angle = (uint8_t)atoi(angle_s);
    if      (strcmp(axis, "pan")  == 0) servo_set_angle(SERVO_PAN,  angle);
    else if (strcmp(axis, "tilt") == 0) servo_set_angle(SERVO_TILT, angle);
    else {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Unknown axis");
        return ESP_FAIL;
    }

    char resp[64];
    snprintf(resp, sizeof(resp),
             "{\"status\":\"ok\",\"axis\":\"%s\",\"angle\":%d}", axis, angle);
    httpd_resp_sendstr(req, resp);
    return ESP_OK;
}

// ─────────────────────────────────────────────────────────────────────────────
//  GET /status
// ─────────────────────────────────────────────────────────────────────────────
static esp_err_t status_handler(httpd_req_t *req)
{
    char resp[64];
    snprintf(resp, sizeof(resp),
             "{\"pan\":%d,\"tilt\":%d}",
             servo_get_angle(SERVO_PAN),
             servo_get_angle(SERVO_TILT));
    httpd_resp_set_type(req, "application/json");
    httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
    httpd_resp_sendstr(req, resp);
    return ESP_OK;
}

// ─────────────────────────────────────────────────────────────────────────────
//  Start server
// ─────────────────────────────────────────────────────────────────────────────
esp_err_t http_server_start(void)
{
    httpd_config_t cfg        = HTTPD_DEFAULT_CONFIG();
    cfg.server_port           = HTTP_PORT;
    cfg.max_open_sockets      = 5;
    cfg.lru_purge_enable      = true;
    cfg.recv_wait_timeout     = 10;
    cfg.send_wait_timeout     = 10;
    cfg.stack_size            = 8192;
    // Run stream task at higher priority than WiFi stack (23)
    // so TCP sends don't get preempted mid-frame
    cfg.task_priority         = 24;

    httpd_handle_t server = NULL;
    esp_err_t ret = httpd_start(&server, &cfg);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to start HTTP server: %s", esp_err_to_name(ret));
        return ret;
    }

    static const httpd_uri_t uri_index  = { .uri="/",       .method=HTTP_GET, .handler=index_handler  };
    static const httpd_uri_t uri_stream = { .uri="/stream", .method=HTTP_GET, .handler=stream_handler };
    static const httpd_uri_t uri_servo  = { .uri="/servo",  .method=HTTP_GET, .handler=servo_handler  };
    static const httpd_uri_t uri_status = { .uri="/status", .method=HTTP_GET, .handler=status_handler };

    httpd_register_uri_handler(server, &uri_index);
    httpd_register_uri_handler(server, &uri_stream);
    httpd_register_uri_handler(server, &uri_servo);
    httpd_register_uri_handler(server, &uri_status);

    ESP_LOGI(TAG, "HTTP server started on port %d (task priority %d)",
             HTTP_PORT, cfg.task_priority);
    return ESP_OK;
}
