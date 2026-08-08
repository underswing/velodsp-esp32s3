#include <stdint.h>

#include "freertos/FreeRTOS.h"

#include "esp_err.h"
#include "status_led.h"

#define RGB_LED_GPIO 38

void app_main(void)
{
    ESP_ERROR_CHECK(status_led_init(RGB_LED_GPIO));
    ESP_ERROR_CHECK(status_led_set_hsv(120, 255, 10));
}