#pragma once

#include "esp_err.h"

// Initializes the OV2640 camera on the XIAO ESP32-S3 Sense for JPEG streaming.
esp_err_t app_camera_init(void);
