#pragma once

#include "esp_err.h"

// Starts a background task that uploads JPEG frames to the hosted SnackYou API.
esp_err_t camera_uploader_start(void);
