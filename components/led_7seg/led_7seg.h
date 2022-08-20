#ifndef LED_7SEG_H
#define LED_7SEG_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct led_7seg_t *led_7seg_handle_t;

typedef struct {
    int din_gpio_num;
    int sclk_gpio_num;
    int rclk_gpio_num;
    uint8_t bits_num;
    uint8_t refresh_period_per_bit;     // ms
    bool blink_en;
    uint16_t blink_period;
} led_7seg_config_t;

void led_7seg_init(led_7seg_config_t *config, led_7seg_handle_t *handle);
void led_7seg_set_display_int(int num, led_7seg_handle_t handle);
void led_7seg_blink(uint8_t bit_mask, bool en, led_7seg_handle_t handle);

#ifdef __cplusplus
}
#endif

#endif