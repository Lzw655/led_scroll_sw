#include <stdlib.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/timers.h"
#include "driver/pulse_cnt.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "rotary_encoder.h"

#define PCNT_LOW_LIMIT          (-100)
#define PCNT_HIGH_LIMIT         (100)
#define QUEUE_SIZE_MAX          (100)
#define QUEUE_REC_TIMEOUT       (1)

typedef struct rotary_encoder_t rotary_encoder_t;

struct rotary_encoder_t {
    rotary_encoder_config_t config;
    QueueHandle_t queue;
    TimerHandle_t encoder_timer;
    TimerHandle_t sw_timer;
};

static char *TAG = "rotary_encoder";

static bool pcnt_on_reach(pcnt_unit_handle_t unit, const pcnt_watch_event_data_t *edata, void *user_ctx);
static void encoder_timer_cb(TimerHandle_t xTimer);
static void sw_isr_handler(void* arg);
static void sw_timer_cb(TimerHandle_t xTimer);

esp_err_t rotary_encoder_create(rotary_encoder_config_t *config, rotary_encoder_handle_t *handle)
{
    rotary_encoder_t *dev = (rotary_encoder_t *)malloc(sizeof(rotary_encoder_t));
    if (!dev) {
        ESP_LOGE(TAG, "malloc error");
        return ESP_FAIL;
    }
    memcpy(&(dev->config), config, sizeof(rotary_encoder_config_t));
    dev->queue = xQueueCreate(QUEUE_SIZE_MAX, sizeof(int));

    // Create pcnt unit
    pcnt_unit_handle_t unit;
    pcnt_unit_config_t unit_conf = {
        .high_limit = PCNT_HIGH_LIMIT,
        .low_limit = PCNT_LOW_LIMIT,
    };
    ESP_ERROR_CHECK(pcnt_new_unit(&unit_conf, &unit));
    // Set glitch filter
    pcnt_glitch_filter_config_t glitch_conf = {
        .max_glitch_ns = dev->config.encoder_filter_ns,
    };
    ESP_ERROR_CHECK(pcnt_unit_set_glitch_filter(unit, &glitch_conf));
    // Add channel 1 unit
    pcnt_channel_handle_t channel_1;
    pcnt_chan_config_t channel_conf_1 = {
        .edge_gpio_num = dev->config.a_gpio_num,
        .level_gpio_num = dev->config.b_gpio_num,
    };
    ESP_ERROR_CHECK(pcnt_new_channel(unit, &channel_conf_1, &channel_1));
    ESP_ERROR_CHECK(pcnt_channel_set_edge_action(
        channel_1, PCNT_CHANNEL_EDGE_ACTION_DECREASE, PCNT_CHANNEL_EDGE_ACTION_INCREASE
    ));
    ESP_ERROR_CHECK(pcnt_channel_set_level_action(
        channel_1, PCNT_CHANNEL_LEVEL_ACTION_KEEP, PCNT_CHANNEL_LEVEL_ACTION_INVERSE
    ));
    // Add channel 2 in unit
    pcnt_channel_handle_t channel_2;
    pcnt_chan_config_t channel_conf_2 = {
        .edge_gpio_num = dev->config.b_gpio_num,
        .level_gpio_num = dev->config.a_gpio_num,
    };
    ESP_ERROR_CHECK(pcnt_new_channel(unit, &channel_conf_2, &channel_2));
    ESP_ERROR_CHECK(pcnt_channel_set_edge_action(
        channel_2, PCNT_CHANNEL_EDGE_ACTION_INCREASE, PCNT_CHANNEL_EDGE_ACTION_DECREASE
    ));
    ESP_ERROR_CHECK(pcnt_channel_set_level_action(
        channel_2, PCNT_CHANNEL_LEVEL_ACTION_KEEP, PCNT_CHANNEL_LEVEL_ACTION_INVERSE
    ));
    // Add watch points and register callback
    ESP_ERROR_CHECK(pcnt_unit_add_watch_point(unit, -2));
    ESP_ERROR_CHECK(pcnt_unit_add_watch_point(unit, 2));
    QueueHandle_t queue = xQueueCreate(QUEUE_SIZE_MAX, sizeof(int));
    assert(queue);
    pcnt_event_callbacks_t cbs = {
        .on_reach = pcnt_on_reach,
    };
    ESP_ERROR_CHECK(pcnt_unit_register_event_callbacks(unit, &cbs, dev));
    // Create timer
    dev->encoder_timer = xTimerCreate(
        "", pdMS_TO_TICKS(dev->config.encoder_period_ms), pdFALSE, (void *)dev, encoder_timer_cb
    );
    // Enable pcnt unit
    ESP_ERROR_CHECK(pcnt_unit_enable(unit));
    ESP_ERROR_CHECK(pcnt_unit_clear_count(unit));
    ESP_ERROR_CHECK(pcnt_unit_start(unit));

    /* Init switch gpio */
    if (dev->config.sw_gpio_num != -1) {
        gpio_config_t io_conf = {};
        io_conf.intr_type = GPIO_INTR_NEGEDGE;
        io_conf.mode = GPIO_MODE_INPUT;
        io_conf.pin_bit_mask = 1ULL << (dev->config.sw_gpio_num);
        io_conf.pull_down_en = 0;
        io_conf.pull_up_en = 1;
        ESP_ERROR_CHECK(gpio_config(&io_conf));
        ESP_ERROR_CHECK(gpio_install_isr_service(0));
        ESP_ERROR_CHECK(gpio_isr_handler_add(dev->config.sw_gpio_num, sw_isr_handler, dev));

        dev->sw_timer = xTimerCreate(
            "", pdMS_TO_TICKS(dev->config.sw_filter_ms), pdFALSE, (void *)dev, sw_timer_cb
        );
    }

    *handle = dev;

    return ESP_OK;
}

void rotary_encoder_del(rotary_encoder_handle_t handle)
{
    rotary_encoder_t *dev = (rotary_encoder_t *)handle;
    vQueueDelete(dev->queue);
    free(dev);
}

static bool pcnt_on_reach(pcnt_unit_handle_t unit, const pcnt_watch_event_data_t *edata, void *user_ctx)
{
    rotary_encoder_t *dev = (rotary_encoder_t *)user_ctx;
    QueueHandle_t queue = dev->queue;
    int watch_point_value;
    watch_point_value = edata->watch_point_value / 2;
    BaseType_t high_task_wakeup;
    xQueueSendFromISR(queue, &watch_point_value, &high_task_wakeup);
    pcnt_unit_clear_count(unit);

    if (!xTimerIsTimerActive(dev->encoder_timer)) {
        xTimerStartFromISR(dev->encoder_timer, &high_task_wakeup);
    }

    return (high_task_wakeup == pdTRUE);
}

static void encoder_timer_cb(TimerHandle_t xTimer)
{
    rotary_encoder_t *dev = (rotary_encoder_t *)pvTimerGetTimerID(xTimer);
    QueueHandle_t queue = dev->queue;
    int count, count_sum = 0;

    while (xQueueReceive(queue, &count, pdMS_TO_TICKS(QUEUE_REC_TIMEOUT))) {
        ESP_LOGI(TAG, "queue: %d", count);
        count_sum += count;
    }

    if (dev->config.on_encoder_act) {
        dev->config.on_encoder_act(dev, count_sum, dev->config.encoder_period_ms);
    }
}

static void sw_isr_handler(void* arg)
{
    rotary_encoder_t *dev = (rotary_encoder_t *)arg;
    BaseType_t task_woken = pdFALSE;
    if (!xTimerIsTimerActive(dev->sw_timer)) {
        xTimerStartFromISR(dev->sw_timer, &task_woken);
    }
    else {
        xTimerResetFromISR(dev->sw_timer, &task_woken);
    }

    if (task_woken) {
        portYIELD_FROM_ISR();
    }
}

static void sw_timer_cb(TimerHandle_t xTimer)
{
    rotary_encoder_t *dev = (rotary_encoder_t *)pvTimerGetTimerID(xTimer);
    if (!gpio_get_level(dev->config.sw_gpio_num) && dev->config.on_sw_press) {
        dev->config.on_sw_press(dev);
    }
}
