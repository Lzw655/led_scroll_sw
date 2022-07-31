#include <stdio.h>
#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_chip_info.h"
#include "esp_flash.h"
#include "hc595.h"

void app_main(void)
{
    for (;;) {
        printf("hello world\n");
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
