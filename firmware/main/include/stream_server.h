#pragma once

#include "esp_err.h"

// Starts the HTTP server providing:
//   GET /stream  - continuous MJPEG stream (multipart/x-mixed-replace)
//   GET /capture - single JPEG frame
esp_err_t stream_server_start(void);
