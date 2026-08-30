#pragma once

#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    WIFI_STATUS_DISCONNECTED,
    WIFI_STATUS_CONNECTED
} wifi_status_t;

typedef void (*wifi_status_callback_t)(wifi_status_t status);

esp_err_t wifi_init_sta();
esp_err_t wifi_set_credentials(const char *ssid, const char *password);
esp_err_t wifi_connect();

void wifi_set_status_callback(wifi_status_callback_t callback);

#ifdef __cplusplus
}
#endif
