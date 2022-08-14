#include <stdio.h>
#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "esp_err.h"
#include "esp_log.h"
#include "led_7seg_module.h"
#include "led_scroller.h"
#include "rotary_encoder.h"

static void sw_on_push(void *arg);

static char *TAG = "app_main";
static rotary_encoder_dev_t rotary_encoder_dev = {
    .a_gpio_num = 14,
    .b_gpio_num = 12,
    .sw_gpio_num = 13,
    .counter.limit_max = 100,
    .counter.limit_min = -100,
    .counter.watch_pint = 2,
    .counter.filter_ns = 1000,
    .queue_size = 10,
    .sw_callback = sw_on_push,
};

void app_main(void)
{
    ESP_LOGI(TAG, "hello world");

    ESP_LOGI(TAG, "Init led_7seg_module");
    led_7seg_module_init();
    led_7seg_module_set_display_int(1234);

    ESP_LOGI(TAG, "Init led_scroller");
    led_scroller_init();
    led_scroller_set(0x3, 0x33, 1);

    rotary_encoder_init(&rotary_encoder_dev);

    QueueHandle_t queue = rotary_encoder_dev.queue;
    int count;
    for (;;) {
        if (xQueueReceive(queue, &count, portMAX_DELAY)) {
            ESP_LOGI(TAG, "count: %d", count);
        }
    }
}

static void sw_on_push(void *arg)
{
    ESP_DRAM_LOGI(TAG, "swtich push");
}
