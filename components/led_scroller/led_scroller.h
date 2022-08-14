#ifndef LED_SCROLLER_H
#define LED_SCROLLER_H

#ifdef __cplusplus
extern "C" {
#endif

void led_scroller_init(void);
void led_scroller_set(unsigned short sigs, unsigned short bits, int flag);

#ifdef __cplusplus
}
#endif

#endif