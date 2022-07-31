#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_err.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "hc595.h"
#include "led_7seg_module.h"

#define DISPLAY_PERIOD          2       // ms

static char *TAG = "led_7seg_module";
static hc595_dev_t hc595_dev = {0};
static const unsigned char display_char[10] = {
//   0     1     2     3     4     5     6     7     8     9     A     b     C     d     E     F     -
    0xC0, 0xF9, 0xA4, 0xB0, 0x99, 0x92, 0x82, 0xF8, 0x80, 0x90, 0x8C, 0xBF, 0xC6, 0xA1, 0x86, 0xFF, 0xBF
};
static unsigned char display_array[4][2] = {
//  {char, bit}
    {0xBF, 0x01},
    {0xBF, 0x02},
    {0xBF, 0x04},
    {0xBF, 0x08},
};

static void display_task(void *args);

void led_7seg_module_init(void)
{
    hc595_dev.data_in_gpio_num = 32;
    hc595_dev.shift_clk_gpio_num = 25;
    hc595_dev.storage_clk_gpio_num = 26;
    hc595_dev.ouput_en_gpio_num = -1;
    hc595_dev.reset_gpio_num = -1;
    hc595_dev.flags.output_remain_en = 1;
    hc595_init(&hc595_dev);

    xTaskCreate(display_task, "led_7seg_module task", 2048, NULL, tskIDLE_PRIORITY + 1, NULL);
}

void led_7seg_module_set_display_int(int num)
{
    unsigned char temp;
    for (int i = 0; i < 4; i++) {
        temp = num % 10;
        display_array[i][0] = display_char[temp];
        num /= 10;
    }
}

static void display_task(void *params)
{
    TickType_t tick;

    ESP_LOGI(TAG, "display task begin");
    for (;;) {
        tick = xTaskGetTickCount();
        // Refresh All bits
        for (int i = 0; i < 4; i++) {
            hc595_write_bytes(&hc595_dev, display_array[i], 2);
            xTaskDelayUntil(&tick, pdMS_TO_TICKS(DISPLAY_PERIOD));
        }
    }
}
