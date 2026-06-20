# Freenove ESP32-S3-WROOM + OV3660 — MJPEG Stream & Dual Servo Control

ESP-IDF project for the **Freenove ESP32-S3-WROOM (FNK0085)** board.

---

## Board Identification

| Feature | Detail |
|---------|--------|
| Board | Freenove ESP32-S3-WROOM V1.0 |
| Chip | ESP32-S3 R8M (8 MB octal PSRAM embedded in package) |
| Camera connector | 24-pin FPC |
| Camera module | OV3660 (included) |
| USB | USB-C (built-in programmer, no FTDI needed) |
| Buttons | RESET + BOOT on board |
| IO2 LED | GPIO2 onboard LED |

---

## Pinout (from back of PCB)

```
Left header (top→bottom)    Right header (top→bottom)
  RES                          GPIO0
  GPIO44 (UART RX)             GPIO14
  GPIO43 (UART TX)             GPIO47
  GPIO42  ← SERVO PAN          NC
  GPIO41  ← SERVO TILT         GPIO48
  GPIO46                       GPIO1
  GND                          GND
  3V3                          5V

Bottom pads: GND | 5V | GPIO47 | GPIO48 | GND | 5V
```

---

## Servo Wiring

```
Servo (Brown/Black)  GND → ESP32 GND     (left header, 7th pin)
Servo (Red)          VCC → ESP32 5V      (right header, 8th pin)
Servo Pan  (signal)  SIG → GPIO42        (left header, 4th pin)
Servo Tilt (signal)  SIG → GPIO41        (left header, 5th pin)
```

> ⚠️ Two servos under load can draw 1–2 A. For USB-powered use, add a
> separate 5 V / 2 A supply and share only GND with the ESP32.

---

## Camera Pin Mapping (OV3660 on FPC)

| Signal | GPIO | Notes |
|--------|------|-------|
| XCLK | 15 | 20 MHz clock output |
| SIOD (SDA) | 4 | SCCB data |
| SIOC (SCL) | 5 | SCCB clock |
| VSYNC | 6 | |
| HREF | 7 | |
| PCLK | 13 | |
| Y2 (D0 LSB) | 11 | |
| Y3 (D1) | 9 | |
| Y4 (D2) | 8 | |
| Y5 (D3) | 10 | |
| Y6 (D4) | 12 | |
| Y7 (D5) | 18 | |
| Y8 (D6) | 17 | |
| Y9 (D7 MSB) | 16 | |

---

## Quick Start

```bash
# 1. Edit WiFi credentials
#    main/config.h → WIFI_SSID / WIFI_PASSWORD

# 2. Install esp32-camera component
mkdir -p components
cd components
git clone https://github.com/espressif/esp32-camera.git
cd ..

# 3. Build & flash (USB-C cable directly — no programmer needed)
idf.py set-target esp32s3
idf.py build flash monitor
```

> **First flash:** Delete any old `sdkconfig` and `build/` directory first:
> ```powershell
> Remove-Item -Recurse -Force build, sdkconfig
> idf.py set-target esp32s3
> idf.py build flash monitor
> ```

After boot, monitor shows:
```
I (xxxx) main: Open  http://192.168.x.x/
I (xxxx) main: Stream http://192.168.x.x/stream
```

---

## REST API

| Endpoint | Description |
|----------|-------------|
| `GET /` | Browser control page |
| `GET /stream` | MJPEG stream (use in `<img>` tag or VLC) |
| `GET /servo?axis=pan&angle=90` | Set pan 0–180° |
| `GET /servo?axis=tilt&angle=45` | Set tilt 0–180° |
| `GET /servo?axis=center` | Centre both servos |
| `GET /status` | JSON `{"pan":90,"tilt":90}` |

---

## sdkconfig Note

`CONFIG_OV3660_SUPPORT=y` is set in `sdkconfig.defaults`.
**Always delete `sdkconfig` and `build/` before first flash** or when
changing `sdkconfig.defaults`, otherwise the old cached config is used
and the camera sensor will not be compiled in (returns `ESP_ERR_NOT_SUPPORTED`).

---

## Project Structure

```
esp32s3_freenove/
├── CMakeLists.txt
├── partitions.csv
├── sdkconfig.defaults       ← OV3660 + 16MB flash + PSRAM config
├── README.md
└── main/
    ├── CMakeLists.txt
    ├── idf_component.yml    ← auto-downloads esp32-camera
    ├── config.h             ← ALL pins + WiFi creds here
    ├── main.c               ← boot sequence (non-fatal camera init)
    ├── camera.c / .h
    ├── servo.c / .h
    ├── wifi.c / .h
    └── http_server.c / .h
```
