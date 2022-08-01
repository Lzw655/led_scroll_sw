#ifndef ROTARY_ENCODER_H
#define ROTARY_ENCODER_H

#ifdef __cplusplus
extern "C" {
#endif

typedef void (*rotary_encoder_sw_callback_t)(void *);

typedef struct {
    int a_gpio_num;
    int b_gpio_num;
    int sw_gpio_num;
    int *count;
    rotary_encoder_sw_callback_t sw_callback;
} rotary_encoder_dev_t;

void rotary_encoder_init(rotary_encoder_dev_t *dev);

#ifdef __cplusplus
}
#endif

#endif