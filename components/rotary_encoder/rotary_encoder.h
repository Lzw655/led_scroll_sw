#ifndef ROTARY_ENCODER_H
#define ROTARY_ENCODER_H

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/timers.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef void (*rotary_encoder_sw_callback_t)(void);

typedef struct {
    int a_gpio_num;
    int b_gpio_num;
    int sw_gpio_num;
    struct {
        int limit_max;
        int limit_min;
        int watch_pint;
        uint32_t filter_ns;
    } counter;
    int queue_size;
    QueueHandle_t queue;
    int sw_filter_ms;
    TimerHandle_t sw_filter_timer;
    rotary_encoder_sw_callback_t sw_callback;
} rotary_encoder_dev_t;

void rotary_encoder_init(rotary_encoder_dev_t *dev);
int rotary_encoder_check_sw(rotary_encoder_dev_t *dev);

#ifdef __cplusplus
}
#endif

#endif