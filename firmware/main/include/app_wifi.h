#pragma once

#include "esp_err.h"

// Connects to the WiFi network configured via menuconfig (station mode).
// Blocks until an IP address is obtained or the retry limit is reached.
esp_err_t app_wifi_connect(void);
