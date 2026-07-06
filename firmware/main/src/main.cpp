#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "app_camera.h"
#include "app_wifi.h"
#include "camera_uploader.h"
#include "stream_server.h"

static const char *TAG = "main";

extern "C" void app_main(void)
{
    if (app_camera_init() != ESP_OK) {
        ESP_LOGE(TAG, "camera init failed, rebooting in 5s");
        vTaskDelay(pdMS_TO_TICKS(5000));
        esp_restart();
    }

    if (app_wifi_connect() != ESP_OK) {
        ESP_LOGE(TAG, "WiFi connection failed, rebooting in 5s");
        vTaskDelay(pdMS_TO_TICKS(5000));
        esp_restart();
    }

    ESP_ERROR_CHECK(stream_server_start());
    ESP_ERROR_CHECK(camera_uploader_start());

    ESP_LOGI(TAG, "local camera stream ready at http://<device-ip>:%d/stream", CONFIG_SNACKYOU_STREAM_PORT);
    ESP_LOGI(TAG, "hosted app stream is relayed at %s/camera/stream", "https://snackapi.zeusyboy.com/api");
}
