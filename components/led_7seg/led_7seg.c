#include <stdlib.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "freertos/timers.h"
#include "esp_err.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "hc595.h"
#include "led_7seg.h"

typedef struct led_7seg_t led_7seg_t;

struct led_7seg_t {
    hc595_dev_t hc595_dev;
    TimerHandle_t blink_timer;
    uint8_t bits_num;
    uint8_t refresh_period_per_bit;
    uint8_t blink_bits_mask;
    uint8_t refresh_bits_mask;
    uint8_t display_array[];
};

static char *TAG = "led_7seg";
static const uint8_t display_char[] = {
//   0     1     2     3     4     5     6     7     8     9     A     b     C     d     E     F     -
    0xC0, 0xF9, 0xA4, 0xB0, 0x99, 0x92, 0x82, 0xF8, 0x80, 0x90, 0x8C, 0xBF, 0xC6, 0xA1, 0x86, 0xFF, 0xBF, 0xFF
};

static void refresh_task(void *param);
static void blink_timer_cb(TimerHandle_t xTimer);

void led_7seg_init(led_7seg_config_t *config, led_7seg_handle_t *handle)
{
    // Init hc595 device
    led_7seg_t *module = (led_7seg_t *)malloc(sizeof(led_7seg_t) + config->bits_num);
    module->hc595_dev.data_in_gpio_num = config->din_gpio_num;
    module->hc595_dev.shift_clk_gpio_num = config->sclk_gpio_num;
    module->hc595_dev.storage_clk_gpio_num = config->rclk_gpio_num;
    module->hc595_dev.ouput_en_gpio_num = -1;
    module->hc595_dev.reset_gpio_num = -1;
    module->hc595_dev.flags.output_remain_en = 1;
    hc595_init(&(module->hc595_dev));

    // Init module
    module->bits_num = config->bits_num;
    module->blink_bits_mask = 0;
    module->refresh_bits_mask = 0xff;
    module->refresh_period_per_bit = config->refresh_period_per_bit;
    xTaskCreatePinnedToCore(
        refresh_task, "led_7seg", 2048, (void *)module, config->blink.task_priority, NULL, config->blink.task_core
    );
    module->blink_timer = NULL;
    if (config->blink.enable) {
        module->blink_timer = xTimerCreate(
            "", pdMS_TO_TICKS(config->blink.period / 2), pdTRUE, (void *)module, blink_timer_cb
        );
    }
    *handle = module;

    ESP_LOGI(TAG, "Finish init");
}

void led_7seg_set_display_int(int num, led_7seg_handle_t handle)
{
    uint8_t temp;
    uint8_t *display_array = handle->display_array;
    uint8_t bits_num = handle->bits_num;

    for (int i = 0; i < bits_num; i++) {
        temp = num % 10;
        display_array[i] = display_char[temp];
        num /= 10;
    }
}

void led_7seg_blink(uint8_t bit_mask, bool en, led_7seg_handle_t handle)
{
    uint8_t old_mask = handle->blink_bits_mask;
    if (en) {
        handle->blink_bits_mask |= bit_mask;
    }
    else {
        handle->blink_bits_mask &= ~bit_mask;
    }
    // Start blink
    if (!old_mask && handle->blink_bits_mask) {
        if (!xTimerIsTimerActive(handle->blink_timer)) {
            xTimerStart(handle->blink_timer, portMAX_DELAY);
        }
    }
    // Stop blink
    else if (old_mask && !handle->blink_bits_mask) {
        if (xTimerIsTimerActive(handle->blink_timer)) {
            xTimerStop(handle->blink_timer, portMAX_DELAY);
        }
        handle->refresh_bits_mask = 0xff;
    }
}

static void refresh_task(void *param)
{
    ESP_LOGI(TAG, "refresh_task start");

    led_7seg_handle_t handle = (led_7seg_handle_t)param;
    hc595_dev_t *dev = &handle->hc595_dev;
    uint8_t *display_array;
    uint8_t bits_num;
    uint8_t refresh_bits_mask;
    uint8_t period;
    TickType_t tick;
    uint8_t temp[2];

    for (;;) {
        display_array = handle->display_array;
        bits_num = handle->bits_num;
        refresh_bits_mask = handle->refresh_bits_mask;
        period = handle->refresh_period_per_bit;

        // Refresh all bits
        for (int i = 0; i < bits_num; i++) {
            tick = xTaskGetTickCount();
            temp[1] = 1 << i;
            if (refresh_bits_mask & 0x1) {
                temp[0] = display_array[i];
            }
            else {
                temp[0] = display_char[17];
            }
            refresh_bits_mask >>= 1;
            hc595_write_bytes(dev, temp, 2);
            xTaskDelayUntil(&tick, pdMS_TO_TICKS(period));
        }
    }
}

static void blink_timer_cb(TimerHandle_t xTimer)
{
    led_7seg_handle_t handle = (led_7seg_handle_t)pvTimerGetTimerID(xTimer);
    uint8_t bits_num = handle->bits_num;
    uint8_t blink_bits_mask = handle->blink_bits_mask;
    uint8_t refresh_bits_mask = handle->refresh_bits_mask;
    for (int i = 0; i < bits_num; i++) {
        if (blink_bits_mask & 0x1) {
            refresh_bits_mask ^= (1 << i);
        }
        blink_bits_mask >>= 1;
    }
    handle->refresh_bits_mask = refresh_bits_mask;
}
