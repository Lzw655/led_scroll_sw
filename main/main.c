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

#define LED_SCROLLER_FRE        (10)
#define LED_SCROLLER_FRE_MAX    (2000)
#define LED_SCROLLER_FRE_MIN    (1)

static char *TAG = "app_main";

led_7seg_handle_t module_handle;
rotary_encoder_handle_t encoder_handle;
TaskHandle_t task_freq_change_handle;

static void on_encoder_act_cb(rotary_encoder_handle_t handle, int count, int period_ms);
static void on_sw_cb(rotary_encoder_handle_t handle);
static void task_freq_change(void *param);

void app_main(void)
{
    ESP_LOGI(TAG, "hello world");

    ESP_LOGI(TAG, "Init led_7seg");
    led_7seg_config_t module_config = {
        .bits_num = 4,
        .din_gpio_num = 33,
        .sclk_gpio_num = 25,
        .rclk_gpio_num = 26,
        .refresh_period_per_bit = 2,
        .blink = {
            .enable = true,
            .period = 1000,
            .task_core = tskNO_AFFINITY,
            .task_priority = tskIDLE_PRIORITY + 1,
        },
    };
    led_7seg_init(&module_config, &module_handle);
    led_7seg_set_display_int(LED_SCROLLER_FRE, module_handle);

    ESP_LOGI(TAG, "Init led_scroller");
    led_scroller_init();
    led_scroller_run(true, 5, LED_SCROLLER_FRE);

    ESP_LOGI(TAG, "Init rotary_encoder");
    rotary_encoder_config_t encoder_cfg = {
        .a_gpio_num = 14,
        .b_gpio_num = 12,
        .sw_gpio_num = 13,
        .encoder_filter_ns = 1000,
        .encoder_period_ms = 100,
        .on_encoder_act = on_encoder_act_cb,
        .sw_filter_ms = 20,
        .on_sw_press = on_sw_cb,
    };
    rotary_encoder_create(&encoder_cfg, &encoder_handle);

    xTaskCreatePinnedToCore(task_freq_change, "freq_change", 2048, NULL, tskIDLE_PRIORITY + 1, &task_freq_change_handle, tskNO_AFFINITY);
}

static void on_encoder_act_cb(rotary_encoder_handle_t handle, int count, int period_ms)
{
    ESP_LOGI(TAG, "encode: %d", count);
}

static void on_sw_cb(rotary_encoder_handle_t handle)
{
    ESP_DRAM_LOGI(TAG, "swtich push");
}

static void task_freq_change(void *param)
{
    ESP_LOGI(TAG, "task_freq_change begin");

    for (;;) {
                xTaskNotifyWait(0, ULONG_MAX, NULL, portMAX_DELAY);
    }
}
