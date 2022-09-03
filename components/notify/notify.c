#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/timers.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "notify.h"

#define NOTIFY_IN_GPIO              (22)
#define NOTIFY_OUT_GPIO             (23)

#define FILTER_MS                   (10)

static char *TAG = "notify";

static TimerHandle_t notify_timer;
static notify_callback_t notify_callback = NULL;

static void notify_isr_handler(void* arg);
static void notify_timer_cb(TimerHandle_t xTimer);

void notify_init(notify_callback_t cb)
{
    notify_callback = cb;

    gpio_config_t notify_config = {
        .pin_bit_mask = (1ULL << NOTIFY_IN_GPIO),
        .intr_type = GPIO_INTR_NEGEDGE,
        .mode = GPIO_MODE_INPUT,
        .pull_down_en = false,
        .pull_up_en = true,
    };
    gpio_config(&notify_config);
    notify_config.pin_bit_mask = 1ULL << NOTIFY_OUT_GPIO;
    notify_config.intr_type = GPIO_INTR_DISABLE,
    notify_config.mode = GPIO_MODE_OUTPUT,
    notify_config.pull_down_en = false;
    notify_config.pull_up_en = false;
    gpio_config(&notify_config);
    gpio_set_level(NOTIFY_OUT_GPIO, 1);

    ESP_ERROR_CHECK(gpio_isr_handler_add(NOTIFY_IN_GPIO, notify_isr_handler, NULL));

    notify_timer = xTimerCreate(
        "", pdMS_TO_TICKS(FILTER_MS), pdFALSE, NULL, notify_timer_cb
    );
}

void notify_out(void)
{
    gpio_set_level(NOTIFY_OUT_GPIO, 1);
    vTaskDelay(pdMS_TO_TICKS(10));
    gpio_set_level(NOTIFY_OUT_GPIO, 0);
}

static void notify_isr_handler(void* arg)
{
    BaseType_t task_woken = pdFALSE;
    if (!xTimerIsTimerActive(notify_timer)) {
        xTimerStartFromISR(notify_timer, &task_woken);
    }
    else {
        xTimerResetFromISR(notify_timer, &task_woken);
    }

    if (task_woken) {
        portYIELD_FROM_ISR();
    }
}

static void notify_timer_cb(TimerHandle_t xTimer)
{
    if (!gpio_get_level(NOTIFY_IN_GPIO)) {
        notify_callback();
    }
}
