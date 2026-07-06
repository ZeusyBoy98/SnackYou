#include "camera_uploader.h"

#include "esp_camera.h"
#include "esp_crt_bundle.h"
#include "esp_http_client.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "camera_uploader";
static constexpr uint32_t UPLOADER_STACK_SIZE = 8192;
static constexpr UBaseType_t UPLOADER_PRIORITY = 4;

static void upload_task(void *)
{
    esp_http_client_config_t config = {};
    config.url = CONFIG_SNACKYOU_CAMERA_UPLOAD_URL;
    config.timeout_ms = 5000;
    config.crt_bundle_attach = esp_crt_bundle_attach;

    esp_http_client_handle_t client = esp_http_client_init(&config);
    if (client == nullptr) {
        ESP_LOGE(TAG, "failed to create HTTP client");
        vTaskDelete(nullptr);
        return;
    }

    esp_http_client_set_method(client, HTTP_METHOD_POST);
    esp_http_client_set_header(client, "Content-Type", "image/jpeg");

    while (true) {
        int64_t started = esp_timer_get_time();
        camera_fb_t *fb = esp_camera_fb_get();
        if (fb == nullptr) {
            ESP_LOGW(TAG, "camera frame unavailable");
            vTaskDelay(pdMS_TO_TICKS(CONFIG_SNACKYOU_CAMERA_UPLOAD_INTERVAL_MS));
            continue;
        }

        if (fb->format != PIXFORMAT_JPEG) {
            ESP_LOGW(TAG, "skipping non-JPEG frame");
            esp_camera_fb_return(fb);
            vTaskDelay(pdMS_TO_TICKS(CONFIG_SNACKYOU_CAMERA_UPLOAD_INTERVAL_MS));
            continue;
        }

        esp_http_client_set_post_field(client, reinterpret_cast<const char *>(fb->buf), fb->len);
        esp_err_t err = esp_http_client_perform(client);
        int status = esp_http_client_get_status_code(client);
        size_t frame_len = fb->len;
        esp_camera_fb_return(fb);

        if (err == ESP_OK && status >= 200 && status < 300) {
            ESP_LOGD(TAG, "uploaded %zu byte frame", frame_len);
        } else {
            ESP_LOGW(TAG, "upload failed: err=%s status=%d", esp_err_to_name(err), status);
        }

        int64_t elapsed_ms = (esp_timer_get_time() - started) / 1000;
        int delay_ms = CONFIG_SNACKYOU_CAMERA_UPLOAD_INTERVAL_MS - static_cast<int>(elapsed_ms);
        if (delay_ms < 1) {
            delay_ms = 1;
        }
        vTaskDelay(pdMS_TO_TICKS(delay_ms));
    }
}

esp_err_t camera_uploader_start(void)
{
#if CONFIG_SNACKYOU_CAMERA_UPLOAD_ENABLED
    BaseType_t ok = xTaskCreate(upload_task, "camera_uploader", UPLOADER_STACK_SIZE, nullptr, UPLOADER_PRIORITY, nullptr);
    if (ok != pdPASS) {
        ESP_LOGE(TAG, "failed to start uploader task");
        return ESP_ERR_NO_MEM;
    }
    ESP_LOGI(TAG, "uploading camera frames to %s", CONFIG_SNACKYOU_CAMERA_UPLOAD_URL);
#else
    ESP_LOGI(TAG, "camera frame upload disabled");
#endif
    return ESP_OK;
}
