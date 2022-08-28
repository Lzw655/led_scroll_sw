#include <stdio.h>
#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "esp_err.h"
#include "esp_log.h"
#include "led_7seg.h"
#include "led_scroller.h"
#include "rotary_encoder.h"

#define DISPLAY_PERIOD          (2)       // ms

#define DATA_IN_GPIO            (33)
#define SCLK_GPIO               (25)
#define RCLK_GPIO               (26)

static void on_encoder_act_cb(rotary_encoder_handle_t handle, int count, int period_ms);
static void on_sw_cb(rotary_encoder_handle_t handle);

static char *TAG = "app_main";
void app_main(void)
{
    ESP_LOGI(TAG, "hello world");

    ESP_LOGI(TAG, "Init led_7seg");
    led_7seg_handle_t module_handle;
    led_7seg_config_t module_config = {
        .bits_num = 4,
        .din_gpio_num = 33,
        .sclk_gpio_num = 25,
        .rclk_gpio_num = 26,
        .refresh_period_per_bit = 2,
        .blink_en = true,
        .blink_period = 1000,
    };
    led_7seg_init(&module_config, &module_handle);
    led_7seg_set_display_int(1234, module_handle);
    led_7seg_blink(0x01, true, module_handle);

    ESP_LOGI(TAG, "Init led_scroller");
    led_scroller_init();

    ESP_LOGI(TAG, "Init rotary_encoder");
    rotary_encoder_handle_t encoder_handle;
    rotary_encoder_config_t encoder_cfg = {
        .a_gpio_num = 14,
        .b_gpio_num = 12,
        .sw_gpio_num = 13,
        .encoder_filter_ns = 1000,
        .encoder_period_ms = 300,
        .on_encoder_act = on_encoder_act_cb,
        .sw_filter_ms = 20,
        .on_sw_press = on_sw_cb,
    };
    rotary_encoder_create(&encoder_cfg, &encoder_handle);

    int last_i = 0, last_j = 0;
    for (;;) {
        for (int i = 0; i < 10; i++) {
            for (int j = 0; j < 10; j++) {
                led_scroller_set(1ULL << last_i, 1ULL << last_j, 0);
                led_scroller_set(1ULL << i, 1ULL << j, 1);
                last_i = i;
                last_j = j;
                vTaskDelay(pdMS_TO_TICKS(1));
            }
        }
    }
}

static void on_encoder_act_cb(rotary_encoder_handle_t handle, int count, int period_ms)
{
    ESP_LOGI(TAG, "encode: %d", count);
}

static void on_sw_cb(rotary_encoder_handle_t handle)
{
    ESP_DRAM_LOGI(TAG, "swtich push");
}
