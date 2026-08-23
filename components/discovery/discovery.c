#include "discovery.h"

#include "esp_check.h"
#include "mdns.h"

static const char *TAG = "discovery";

static bool s_mdns_initialized = false;

esp_err_t discovery_start(void) {
    if (s_mdns_initialized) {
        return ESP_OK;
    }

    ESP_RETURN_ON_ERROR(
        mdns_init(),
        TAG,
        "Failed to initialize mDNS"
    );

    ESP_RETURN_ON_ERROR(
        mdns_hostname_set("velodsp"),
        TAG,
        "Failed to set hostname"
    );

    ESP_RETURN_ON_ERROR(
        mdns_instance_name_set("VeloDSP"),
        TAG,
        "Failed to set instance name"
    );

    ESP_RETURN_ON_ERROR(
        mdns_service_add("VeloDSP", "_http", "_tcp", 80, nullptr, 0),
        TAG,
        "Failed to add mDNS service"
    );

    ESP_LOGI(TAG, "mDNS started");

    return ESP_OK;
}
