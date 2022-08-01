#include <stdio.h>
#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_err.h"
#include "esp_log.h"
#include "led_7seg_module.h"
#include "rotary_encoder.h"

static char *TAG = "app_main";
static rotary_encoder_dev_t rotary_encoder_dev = {
    .a_gpio_num = 14,
    .b_gpio_num = 12,
    .sw_gpio_num = 13,
};

void app_main(void)
{
    ESP_LOGI(TAG, "hello world");

    ESP_LOGI(TAG, "Init led_7seg_module");
    led_7seg_module_init();
    led_7seg_module_set_display_int(1234);

    rotary_encoder_init(&rotary_encoder_dev);

    for (;;) {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
