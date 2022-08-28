#ifndef LED_SCROLLER_H
#define LED_SCROLLER_H

#ifdef __cplusplus
extern "C" {
#endif

void led_scroller_init(void);
void led_scroller_run(bool flag, uint8_t led_nums, int freq);

#ifdef __cplusplus
}
#endif

#endif