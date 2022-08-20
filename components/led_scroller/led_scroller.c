#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "esp_err.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "hc595.h"
#include "led_scroller.h"

#define DISPLAY_PERIOD          (200)       // us
#define DISPLAY_COUNT           (10)

#define DATA_IN_GPIO            (5)
#define SCLK_GPIO               (17)
#define RCLK_GPIO               (16)

typedef struct {
    unsigned short L1: 1;
    unsigned short L2: 1;
    unsigned short L3: 1;
    unsigned short L4: 1;
    unsigned short L5: 1;
    unsigned short L6: 1;
    unsigned short L7: 1;
    unsigned short L8: 1;
    unsigned short L9: 1;
    unsigned short L10: 1;
    unsigned short unused: 6;
} array_element_t;

static char *TAG = "led_scroller";
static hc595_dev_t hc595_dev = {0};
static array_element_t display_array[DISPLAY_COUNT] = {0};
static TaskHandle_t task_handle;
static SemaphoreHandle_t sem_dev_lock;

static void timer_cb(void *args);
static void refresh_task(void *args);

void led_scroller_init(void)
{
    hc595_dev.data_in_gpio_num = DATA_IN_GPIO;
    hc595_dev.shift_clk_gpio_num = SCLK_GPIO;
    hc595_dev.storage_clk_gpio_num = RCLK_GPIO;
    hc595_dev.ouput_en_gpio_num = -1;
    hc595_dev.reset_gpio_num = -1;
    hc595_dev.flags.output_remain_en = 1;
    hc595_init(&hc595_dev);

    sem_dev_lock = xSemaphoreCreateMutex();
    esp_timer_handle_t timer_handle;
    esp_timer_create_args_t timer_args = {
        .name = "led_scroller_refresh",
        .callback = timer_cb,
    };
    ESP_ERROR_CHECK(esp_timer_create(&timer_args, &timer_handle));
    ESP_ERROR_CHECK(esp_timer_start_periodic(timer_handle, DISPLAY_PERIOD));
    // xSemaphoreGive(sem_dev_lock);
    xTaskCreate(refresh_task, "led_scroller task", 2048, NULL, configMAX_PRIORITIES, &task_handle);
}

void led_scroller_set(unsigned short sigs, unsigned short bits, int flag)
{
    unsigned short *data;
    xSemaphoreTake(sem_dev_lock, portMAX_DELAY);
    for (int i = 0; i < DISPLAY_COUNT; i++, sigs >>= 1) {
        if ((sigs & 0x1) == 0) {
            continue;
        }
        data = (unsigned short *)&display_array[i];
        if (flag) {
            *data |= bits;
        }
        else {
            *data &= ~bits;
        }
    }
    xSemaphoreGive(sem_dev_lock);
}

static void timer_cb(void *args)
{
    BaseType_t res;
    xTaskNotifyFromISR(task_handle, 0, eNoAction, &res);
    portYIELD_FROM_ISR(res);
}

static void refresh_task(void *params)
{
    TickType_t tick;
    unsigned char data[3];
    unsigned short sig;
    array_element_t element;

    ESP_LOGI(TAG, "refresh_task begin");
    for (;;) {
        // Refresh All bits
        for (int i = 0; i < DISPLAY_COUNT; i++) {
            xTaskNotifyWait(0, ULONG_MAX, NULL, portMAX_DELAY);
            xSemaphoreTake(sem_dev_lock, portMAX_DELAY);
            sig = 1ULL << i;
            *(unsigned short *)&element = ~(*(unsigned short *)(display_array + i));
            data[0] = (element.L7 << 0) | (element.L8 << 1) | (element.L9 << 2) | (element.L10 << 3);
            data[1] = (((sig >> 8) & 0x1) << 0) | (((sig >> 9) & 0x1) << 1) | (element.L1 << 2) | (element.L2 << 3) |
                      (element.L3 << 4) | (element.L4 << 5) | (element.L5 << 6) | (element.L6 << 7);
            data[2] = sig & 0xff;
            hc595_write_bytes(&hc595_dev, data, 3);
            xSemaphoreGive(sem_dev_lock);
        }
    }
}
