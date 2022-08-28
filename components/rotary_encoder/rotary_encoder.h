#ifndef ROTARY_ENCODER_H
#define ROTARY_ENCODER_H

#ifdef __cplusplus
extern "C" {
#endif

typedef struct rotary_encoder_t *rotary_encoder_handle_t;
typedef void (*rotary_encoder_encoder_cb_t)(rotary_encoder_handle_t handle, int count, int period_ms);
typedef void (*rotary_encoder_sw_cb_t)(rotary_encoder_handle_t handle);

typedef struct {
    int a_gpio_num;
    int b_gpio_num;
    int sw_gpio_num;
    int encoder_filter_ns;
    int encoder_period_ms;
    rotary_encoder_encoder_cb_t on_encoder_act;
    int sw_filter_ms;
    rotary_encoder_sw_cb_t on_sw_press;
} rotary_encoder_config_t;

esp_err_t rotary_encoder_create(rotary_encoder_config_t *config, rotary_encoder_handle_t *handle);
void rotary_encoder_del(rotary_encoder_handle_t handle);

#ifdef __cplusplus
}
#endif

#endif