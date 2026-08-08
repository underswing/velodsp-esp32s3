#include "status_led.h"

#include "led_strip.h"
#include "led_strip_rmt.h"

static led_strip_handle_t led;

esp_err_t status_led_init(int gpio) {
    const led_strip_config_t strip_config = {
        .strip_gpio_num = gpio,
        .max_leds = 1,
        .led_model = LED_MODEL_WS2812,
        .color_component_format = LED_STRIP_COLOR_COMPONENT_FMT_GRB,
        .flags.invert_out = false,
    };

    const led_strip_rmt_config_t rmt_config = {
        .clk_src = RMT_CLK_SRC_DEFAULT,
        .resolution_hz = 10 * 1000 * 1000,
        .mem_block_symbols = 0,
        .flags.with_dma = false,
    };


    const esp_err_t err = led_strip_new_rmt_device(&strip_config, &rmt_config, &led);

    if (err != ESP_OK) {
        return err;
    }

    return led_strip_clear(led);
}

esp_err_t status_led_set_hsv(const uint32_t hue, const uint8_t saturation, const uint8_t value) {
    const esp_err_t err = led_strip_set_pixel_hsv(led, 0, hue, saturation, value);

    if (err != ESP_OK) {
        return err;
    }

    return led_strip_refresh(led);
}

esp_err_t status_led_off(void) {
    return led_strip_clear(led);
}
