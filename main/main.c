#include <stdio.h>
#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_err.h"
#include "esp_log.h"
#include "led_7seg_module.h"

static char *TAG = "app_main";

void app_main(void)
{
    ESP_LOGI(TAG, "hello world");

    ESP_LOGI(TAG, "Init led_7seg_module");
    led_7seg_module_init();
    led_7seg_module_set_display_int(1234);

    for (;;) {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
