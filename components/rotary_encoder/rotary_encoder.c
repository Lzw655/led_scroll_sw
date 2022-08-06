#include <stdlib.h>
#include "driver/pulse_cnt.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "rotary_encoder.h"

static bool pcnt_on_reach(pcnt_unit_handle_t unit, const pcnt_watch_event_data_t *edata, void *user_ctx);
static void sw_isr_handler(void* arg);

void rotary_encoder_init(rotary_encoder_dev_t *dev)
{
    // Create pcnt unit
    pcnt_unit_handle_t unit;
    pcnt_unit_config_t unit_conf = {
        .high_limit = dev->counter.limit_max,
        .low_limit = dev->counter.limit_min,
    };
    ESP_ERROR_CHECK(pcnt_new_unit(&unit_conf, &unit));
    // Set glitch filter
    pcnt_glitch_filter_config_t glitch_conf = {
        .max_glitch_ns = dev->counter.filter_ns,
    };
    ESP_ERROR_CHECK(pcnt_unit_set_glitch_filter(unit, &glitch_conf));
    // Add channel 1 unit
    pcnt_channel_handle_t channel_1;
    pcnt_chan_config_t channel_conf_1 = {
        .edge_gpio_num = dev->a_gpio_num,
        .level_gpio_num = dev->b_gpio_num,
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
        .edge_gpio_num = dev->b_gpio_num,
        .level_gpio_num = dev->a_gpio_num,
    };
    ESP_ERROR_CHECK(pcnt_new_channel(unit, &channel_conf_2, &channel_2));
    ESP_ERROR_CHECK(pcnt_channel_set_edge_action(
        channel_2, PCNT_CHANNEL_EDGE_ACTION_INCREASE, PCNT_CHANNEL_EDGE_ACTION_DECREASE
    ));
    ESP_ERROR_CHECK(pcnt_channel_set_level_action(
        channel_2, PCNT_CHANNEL_LEVEL_ACTION_KEEP, PCNT_CHANNEL_LEVEL_ACTION_INVERSE
    ));
    // Add watch points and register callback
    ESP_ERROR_CHECK(pcnt_unit_add_watch_point(unit, dev->counter.watch_pint));
    ESP_ERROR_CHECK(pcnt_unit_add_watch_point(unit, -(dev->counter.watch_pint)));
    QueueHandle_t queue = xQueueCreate(dev->queue_size, sizeof(int));
    assert(queue);
    pcnt_event_callbacks_t cbs = {
        .on_reach = pcnt_on_reach,
    };
    ESP_ERROR_CHECK(pcnt_unit_register_event_callbacks(unit, &cbs, queue));
    dev->queue = queue;
    // Enable pcnt unit
    ESP_ERROR_CHECK(pcnt_unit_enable(unit));
    ESP_ERROR_CHECK(pcnt_unit_clear_count(unit));
    ESP_ERROR_CHECK(pcnt_unit_start(unit));

    /* Init switch gpio */
    if (dev->sw_gpio_num != -1) {
        gpio_config_t io_conf = {};
        io_conf.intr_type = GPIO_INTR_NEGEDGE;
        io_conf.mode = GPIO_MODE_INPUT;
        io_conf.pin_bit_mask = 1ULL << (dev->sw_gpio_num);
        io_conf.pull_down_en = 0;
        io_conf.pull_up_en = 1;
        ESP_ERROR_CHECK(gpio_config(&io_conf));
        ESP_ERROR_CHECK(gpio_install_isr_service(0));
        ESP_ERROR_CHECK(gpio_isr_handler_add(dev->sw_gpio_num, sw_isr_handler, dev));
    }
}

static bool pcnt_on_reach(pcnt_unit_handle_t unit, const pcnt_watch_event_data_t *edata, void *user_ctx)
{
    BaseType_t high_task_wakeup;
    QueueHandle_t queue = (QueueHandle_t)user_ctx;
    xQueueSendFromISR(queue, &(edata->watch_point_value), &high_task_wakeup);
    pcnt_unit_clear_count(unit);

    return (high_task_wakeup == pdTRUE);
}

static void sw_isr_handler(void* arg)
{
    rotary_encoder_dev_t *dev = (rotary_encoder_dev_t *)arg;
    if (dev->sw_callback) {
        dev->sw_callback();
    }
}
