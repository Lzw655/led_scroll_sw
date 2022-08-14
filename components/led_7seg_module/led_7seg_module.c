#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "esp_err.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "hc595.h"
#include "led_7seg_module.h"

#define DISPLAY_PERIOD          (2)       // ms

#define DATA_IN_GPIO            (33)
#define SCLK_GPIO               (25)
#define RCLK_GPIO               (26)

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
static SemaphoreHandle_t sem_dev_lock;

static void display_task(void *args);

void led_7seg_module_init(void)
{
    hc595_dev.data_in_gpio_num = DATA_IN_GPIO;
    hc595_dev.shift_clk_gpio_num = SCLK_GPIO;
    hc595_dev.storage_clk_gpio_num = RCLK_GPIO;
    hc595_dev.ouput_en_gpio_num = -1;
    hc595_dev.reset_gpio_num = -1;
    hc595_dev.flags.output_remain_en = 1;
    hc595_init(&hc595_dev);

    sem_dev_lock = xSemaphoreCreateMutex();
    xSemaphoreGive(sem_dev_lock);
    xTaskCreate(display_task, "led_7seg_module task", 2048, NULL, tskIDLE_PRIORITY + 1, NULL);
}

void led_7seg_module_set_display_int(int num)
{
    unsigned char temp;

    xSemaphoreTake(sem_dev_lock, portMAX_DELAY);
    for (int i = 0; i < 4; i++) {
        temp = num % 10;
        display_array[i][0] = display_char[temp];
        num /= 10;
    }
    xSemaphoreGive(sem_dev_lock);
}

static void display_task(void *params)
{
    TickType_t tick;

    ESP_LOGI(TAG, "display task begin");
    for (;;) {
        tick = xTaskGetTickCount();
        // Refresh All bits
        for (int i = 0; i < 4; i++) {
            xSemaphoreTake(sem_dev_lock, portMAX_DELAY);
            hc595_write_bytes(&hc595_dev, display_array[i], 2);
            xSemaphoreGive(sem_dev_lock);
            xTaskDelayUntil(&tick, pdMS_TO_TICKS(DISPLAY_PERIOD));
        }
    }
}
