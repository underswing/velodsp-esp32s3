#pragma once

#include <stdint.h>
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

esp_err_t status_led_init(int gpio);

esp_err_t status_led_set_hsv(uint32_t hue, uint8_t saturation, uint8_t value);

esp_err_t status_led_off(void);

#ifdef __cplusplus
}
#endif
