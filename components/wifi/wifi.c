#include "wifi.h"

#include <string.h>

#include "esp_check.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "esp_wifi.h"
#include "esp_wifi_default.h"
#include "nvs_flash.h"

static const char *TAG = "wifi";

static bool s_wifi_initialized = false;
static wifi_status_callback_t s_status_callback = nullptr;

void wifi_set_status_callback(const wifi_status_callback_t callback) {
    s_status_callback = callback;
}

static void wifi_event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data) {
    if (event_base == WIFI_EVENT) {
        switch (event_id) {
            case WIFI_EVENT_STA_START:
                ESP_LOGI(TAG, "Wi-Fi station started");
                break;

            case WIFI_EVENT_STA_DISCONNECTED: {
                const wifi_event_sta_disconnected_t *event = event_data;

                ESP_LOGW(TAG, "Disconnected, reason: %d", event->reason);

                if (s_status_callback != NULL) {
                    s_status_callback(WIFI_STATUS_DISCONNECTED);
                }

                ESP_LOGI(TAG, "Retrying connection...");
                esp_wifi_connect();

                break;
            }

            default:
                break;
        }
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        const ip_event_got_ip_t *event = event_data;

        ESP_LOGI(TAG, "Got IP: " IPSTR, IP2STR(&event->ip_info.ip));

        if (s_status_callback != NULL) {
            s_status_callback(WIFI_STATUS_CONNECTED);
        }
    }
}

esp_err_t wifi_init_sta(void) {
    if (s_wifi_initialized) {
        return ESP_OK;
    }

    esp_err_t err = nvs_flash_init();

    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_RETURN_ON_ERROR(nvs_flash_erase(), TAG, "Failed to erase NVS");
        ESP_RETURN_ON_ERROR(nvs_flash_init(), TAG, "Failed to initialize NVS");
    } else if (err != ESP_OK) {
        return err;
    }

    ESP_RETURN_ON_ERROR(esp_netif_init(), TAG, "Failed to initialize esp-netif");

    err = esp_event_loop_create_default();
    if (err != ESP_OK && err != ESP_ERR_INVALID_STATE) {
        return err;
    }

    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();

    ESP_RETURN_ON_ERROR(esp_wifi_init(&cfg), TAG, "Failed to initialize Wi-Fi");

    ESP_RETURN_ON_ERROR(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL), TAG,
                        "Failed to register Wi-Fi event handler");

    ESP_RETURN_ON_ERROR(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &wifi_event_handler, NULL), TAG,
                        "Failed to register Wi-Fi event handler");

    ESP_RETURN_ON_ERROR(esp_wifi_set_mode(WIFI_MODE_STA), TAG, "Failed to set station mode");

    ESP_RETURN_ON_ERROR(esp_wifi_start(), TAG, "Failed to start Wi-Fi");

    ESP_RETURN_ON_ERROR(esp_wifi_set_ps(WIFI_PS_NONE), TAG, "Failed to disable Wi-Fi power saving");

    s_wifi_initialized = true;

    return ESP_OK;
}

esp_err_t wifi_set_credentials(const char *ssid, const char *password) {
    if (!s_wifi_initialized) {
        return ESP_ERR_INVALID_STATE;
    }

    if (ssid == NULL || password == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    wifi_config_t config = {0};

    strlcpy((char *) config.sta.ssid, ssid, sizeof(config.sta.ssid));

    strlcpy((char *) config.sta.password, password, sizeof(config.sta.password));

    ESP_RETURN_ON_ERROR(esp_wifi_set_config(WIFI_IF_STA, &config), TAG, "Failed to set Wi-Fi configuration");

    ESP_LOGI(TAG, "Wi-Fi credentials updated");

    return ESP_OK;
}


esp_err_t wifi_connect(void) {
    if (!s_wifi_initialized) {
        return ESP_ERR_INVALID_STATE;
    }

    ESP_LOGI(TAG, "Connecting...");

    return esp_wifi_connect();
}
