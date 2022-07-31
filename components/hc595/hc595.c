/**
 * hc595: shift -> storage -> en
 */

#include "esp_rom_sys.h"
#include "driver/gpio.h"
#include "hc595.h"

static void hc595_shift_clk_once(hc595_dev_t *conf);
static void hc595_storage_clk_once(hc595_dev_t *conf);

esp_err_t hc595_init(hc595_dev_t *conf)
{
    esp_err_t res = ESP_OK;
    uint64_t gpio_pin_mask = 0;
    /* Init necessary gpio */
    gpio_pin_mask = (1 << conf->data_in_gpio_num) | (1 << conf->shift_clk_gpio_num) |
                    (1 << conf->storage_clk_gpio_num);

    /* Init unnecessary gpio */
    if (conf->reset_gpio_num != -1) {
        gpio_pin_mask |= (1 << conf->reset_gpio_num);
    }
    if (conf->ouput_en_gpio_num != -1) {
        gpio_pin_mask |= (1 << conf->ouput_en_gpio_num);
    }

    gpio_config_t io_conf = {};
    io_conf.intr_type = GPIO_INTR_DISABLE;
    io_conf.mode = GPIO_MODE_OUTPUT;
    io_conf.pin_bit_mask = gpio_pin_mask;
    io_conf.pull_down_en = 0;
    io_conf.pull_up_en = 0;
    res = gpio_config(&io_conf);
    if (res != ESP_OK) {
        return res;
    }

    gpio_set_level(conf->data_in_gpio_num, 0);
    gpio_set_level(conf->shift_clk_gpio_num, 0);
    gpio_set_level(conf->storage_clk_gpio_num, 0);
    if (conf->reset_gpio_num != -1) {
        gpio_set_level(conf->reset_gpio_num, 1);
    }
    if (conf->ouput_en_gpio_num != -1) {
        if (conf->flags.output_remain_en) {
            gpio_set_level(conf->reset_gpio_num, 0);
        }
        else {
            gpio_set_level(conf->reset_gpio_num, 1);
        }
    }

    return res;
}

void hc595_write_byte(hc595_dev_t *conf, unsigned char data)
{
    for (int i = 0; i < 8; i++) {
        if (data & 0x80)
            gpio_set_level(conf->data_in_gpio_num, 1);
        else
            gpio_set_level(conf->data_in_gpio_num, 0);
        hc595_shift_clk_once(conf);
    }
    hc595_storage_clk_once(conf);
}

void hc595_write_bytes(hc595_dev_t *conf, unsigned char *data, int data_len)
{
    for (int n = data_len; n > 0; n--) {
        for (int i = 0; i < 8; i++) {
            if (data[n] & 0x80)
                gpio_set_level(conf->data_in_gpio_num, 1);
            else
                gpio_set_level(conf->data_in_gpio_num, 0);
            hc595_shift_clk_once(conf);
        }
    }
    hc595_storage_clk_once(conf);
}

void hc595_reset_shift_reg(hc595_dev_t *conf)
{
    if (conf->reset_gpio_num != -1) {
        // Reset low
        gpio_set_level(conf->reset_gpio_num, 0);
        // Output_en low
        if (!conf->flags.output_remain_en)
            hc595_enable_output(conf, true);
        esp_rom_delay_us(1);
        // Restore Reset and Output_en
        gpio_set_level(conf->reset_gpio_num, 1);
        if (!conf->flags.output_remain_en)
            hc595_enable_output(conf, false);
        esp_rom_delay_us(1);
    }
}

void hc595_reset_storage_reg(hc595_dev_t *conf)
{
    if (conf->reset_gpio_num != -1) {
        // Reset low
        gpio_set_level(conf->reset_gpio_num, 0);
        // Storage clk
        hc595_storage_clk_once(conf);
        esp_rom_delay_us(1);
        // Restore Reset
        gpio_set_level(conf->reset_gpio_num, 1);
        esp_rom_delay_us(1);
    }
}

void hc595_enable_output(hc595_dev_t *conf, bool flag)
{
    if (conf->ouput_en_gpio_num != -1) {
        gpio_set_level(conf->ouput_en_gpio_num, !flag);
    }
}

static void hc595_shift_clk_once(hc595_dev_t *conf)
{
    esp_rom_delay_us(1);
    gpio_set_level(conf->shift_clk_gpio_num, 1);
    esp_rom_delay_us(1);
    gpio_set_level(conf->shift_clk_gpio_num, 0);
}

static void hc595_storage_clk_once(hc595_dev_t *conf)
{
    esp_rom_delay_us(1);
    gpio_set_level(conf->storage_clk_gpio_num, 1);
    esp_rom_delay_us(1);
    gpio_set_level(conf->storage_clk_gpio_num, 0);
}
