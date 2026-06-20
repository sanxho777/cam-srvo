cmake_minimum_required(VERSION 3.16)

# Lock the target to ESP32-S3 — prevents cmake from defaulting to esp32
# (must be set BEFORE the include below)
if(NOT DEFINED ENV{IDF_TARGET})
    set(IDF_TARGET esp32s3)
endif()

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(esp32s3_cam_servo)

# Workaround: GCC 14.2.0 has an ICE (internal compiler error, segfault in the
# IRA register-allocator pass) when compiling esp_lcd_panel_rgb.c at -Og.
# Override that single file to -O2 to avoid the crash without changing the
# global optimization level for the rest of the build.
set_source_files_properties(
    $ENV{IDF_PATH}/components/esp_lcd/rgb/esp_lcd_panel_rgb.c
    PROPERTIES COMPILE_FLAGS "-O2"
)
