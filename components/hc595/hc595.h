#ifndef HC595_H
#define HC595_H

#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    int data_in_gpio_num;                   /*!< -1 if not used */
    int reset_gpio_num;
    int shift_clk_gpio_num;
    int storage_clk_gpio_num;
    int ouput_en_gpio_num;
    struct {
        unsigned int output_remain_en;
    } flags;
} hc595_dev_t;

esp_err_t hc595_init(hc595_dev_t *conf);
void hc595_write_byte(hc595_dev_t *conf, unsigned char data);
void hc595_write_bytes(hc595_dev_t *conf, unsigned char *data, int data_len);
void hc595_reset_shift_reg(hc595_dev_t *conf);
void hc595_reset_storage_reg(hc595_dev_t *conf);
void hc595_enable_output(hc595_dev_t *conf, bool flag);

#ifdef __cplusplus
}
#endif

#endif