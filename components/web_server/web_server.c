#include "web_server.h"

#include "esp_check.h"
#include "esp_http_server.h"
#include "esp_log.h"

static const char *TAG = "web_server";

static httpd_handle_t s_server = nullptr;

static esp_err_t websocket_handler(httpd_req_t *req)
{
    if (req->method == HTTP_GET) {
        ESP_LOGI(TAG, "WebSocket connected");
        return ESP_OK;
    }

    httpd_ws_frame_t frame = {0};

    ESP_RETURN_ON_ERROR(
        httpd_ws_recv_frame(req, &frame, 0),
        TAG,
        "Failed to get frame length"
    );

    uint8_t *buffer = malloc(frame.len + 1);
    if (buffer == NULL) {
        return ESP_ERR_NO_MEM;
    }

    frame.payload = buffer;

    esp_err_t err = httpd_ws_recv_frame(req, &frame, frame.len);

    if (err == ESP_OK) {
        buffer[frame.len] = '\0';

        ESP_LOGI(TAG, "Received WS message: %s", buffer);

        err = httpd_ws_send_frame(req, &frame);
    }

    free(buffer);

    return err;
}

esp_err_t web_server_start(void) {
    if (s_server != nullptr) {
        return ESP_OK;
    }

    httpd_config_t config = HTTPD_DEFAULT_CONFIG();

    esp_err_t err = httpd_start(&s_server, &config);
    if (err != ESP_OK) {
        return err;
    }

    static const httpd_uri_t ws_uri = {
        .uri = "/ws",
        .method = HTTP_GET,
        .handler = websocket_handler,
        .user_ctx = NULL,
        .is_websocket = true,
    };

    err = httpd_register_uri_handler(s_server, &ws_uri);
    if (err != ESP_OK) {
        httpd_stop(s_server);
        s_server = NULL;
        return err;
    }

    ESP_LOGI(TAG, "WebSocket server started");

    return ESP_OK;
}

void web_server_stop(void) {
    if (s_server != nullptr) {
        httpd_stop(s_server);
        s_server = nullptr;
    }
}
