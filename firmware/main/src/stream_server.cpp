#include "stream_server.h"

#include <cstdio>
#include <cstring>

#include "esp_camera.h"
#include "esp_http_server.h"
#include "esp_log.h"
#include "esp_timer.h"

static const char *TAG = "stream_server";

#define PART_BOUNDARY "123456789000000000000987654321"
static const char *STREAM_CONTENT_TYPE = "multipart/x-mixed-replace;boundary=" PART_BOUNDARY;
static const char *STREAM_BOUNDARY = "\r\n--" PART_BOUNDARY "\r\n";
static const char *STREAM_PART = "Content-Type: image/jpeg\r\nContent-Length: %zu\r\n\r\n";

// Allow the browser app (served from a different origin) to embed the stream.
static void set_cors_headers(httpd_req_t *req)
{
    httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
}

// GET /capture - single JPEG frame.
static esp_err_t capture_handler(httpd_req_t *req)
{
    camera_fb_t *fb = esp_camera_fb_get();
    if (fb == nullptr) {
        ESP_LOGE(TAG, "camera capture failed");
        httpd_resp_send_500(req);
        return ESP_FAIL;
    }

    httpd_resp_set_type(req, "image/jpeg");
    httpd_resp_set_hdr(req, "Content-Disposition", "inline; filename=capture.jpg");
    set_cors_headers(req);
    esp_err_t res = httpd_resp_send(req, reinterpret_cast<const char *>(fb->buf), fb->len);
    esp_camera_fb_return(fb);
    return res;
}

// GET /stream - continuous MJPEG stream. The loop runs until the client
// disconnects (a chunk send fails), then the frame buffers are released.
static esp_err_t stream_handler(httpd_req_t *req)
{
    esp_err_t res = httpd_resp_set_type(req, STREAM_CONTENT_TYPE);
    if (res != ESP_OK) {
        return res;
    }
    set_cors_headers(req);

    char part_buf[64];
    int64_t last_frame = esp_timer_get_time();

    ESP_LOGI(TAG, "stream client connected");

    while (true) {
        camera_fb_t *fb = esp_camera_fb_get();
        if (fb == nullptr) {
            ESP_LOGE(TAG, "camera capture failed");
            res = ESP_FAIL;
            break;
        }

        res = httpd_resp_send_chunk(req, STREAM_BOUNDARY, strlen(STREAM_BOUNDARY));
        if (res == ESP_OK) {
            int hlen = snprintf(part_buf, sizeof(part_buf), STREAM_PART, fb->len);
            res = httpd_resp_send_chunk(req, part_buf, static_cast<size_t>(hlen));
        }
        if (res == ESP_OK) {
            res = httpd_resp_send_chunk(req, reinterpret_cast<const char *>(fb->buf), fb->len);
        }

        size_t frame_len = fb->len;
        esp_camera_fb_return(fb);

        if (res != ESP_OK) {
            break;
        }

        int64_t now = esp_timer_get_time();
        int64_t frame_time_ms = (now - last_frame) / 1000;
        last_frame = now;
        ESP_LOGD(TAG, "MJPG: %zuKB %lldms (%.1f fps)", frame_len / 1024, frame_time_ms,
                 frame_time_ms > 0 ? 1000.0f / static_cast<float>(frame_time_ms) : 0.0f);
    }

    ESP_LOGI(TAG, "stream client disconnected");
    return res;
}

esp_err_t stream_server_start(void)
{
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.server_port = CONFIG_SNACKYOU_STREAM_PORT;
    config.lru_purge_enable = true;

    httpd_handle_t server = nullptr;
    esp_err_t err = httpd_start(&server, &config);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "failed to start HTTP server: %s", esp_err_to_name(err));
        return err;
    }

    const httpd_uri_t stream_uri = {
        .uri = "/stream",
        .method = HTTP_GET,
        .handler = stream_handler,
        .user_ctx = nullptr,
    };
    const httpd_uri_t capture_uri = {
        .uri = "/capture",
        .method = HTTP_GET,
        .handler = capture_handler,
        .user_ctx = nullptr,
    };
    ESP_ERROR_CHECK(httpd_register_uri_handler(server, &stream_uri));
    ESP_ERROR_CHECK(httpd_register_uri_handler(server, &capture_uri));

    ESP_LOGI(TAG, "stream server started on port %d", config.server_port);
    return ESP_OK;
}
