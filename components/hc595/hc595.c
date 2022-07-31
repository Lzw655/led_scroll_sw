/**
 * hc595: shift -> storage -> en
 */
#include "esp_log.h"
#include "esp_rom_sys.h"
#include "driver/gpio.h"
#include "hc595.h"

static void hc595_shift_clk_once(hc595_dev_t *dev);
static void hc595_storage_clk_once(hc595_dev_t *dev);

void hc595_init(hc595_dev_t *dev)
{
    uint64_t gpio_pin_mask = 0;
    /* Init necessary gpio */
    gpio_pin_mask = (1ULL << (dev->data_in_gpio_num)) | (1ULL << (dev->shift_clk_gpio_num)) |
                    (1ULL << (dev->storage_clk_gpio_num));

    /* Init unnecessary gpio */
    if (dev->reset_gpio_num != -1) {
        gpio_pin_mask |= (1 << dev->reset_gpio_num);
    }
    if (dev->ouput_en_gpio_num != -1) {
        gpio_pin_mask |= (1 << dev->ouput_en_gpio_num);
    }

    gpio_config_t io_conf = {};
    io_conf.intr_type = GPIO_INTR_DISABLE;
    io_conf.mode = GPIO_MODE_OUTPUT;
    io_conf.pin_bit_mask = gpio_pin_mask;
    io_conf.pull_down_en = 0;
    io_conf.pull_up_en = 0;
    ESP_ERROR_CHECK(gpio_config(&io_conf));

    gpio_set_level(dev->data_in_gpio_num, 0);
    gpio_set_level(dev->shift_clk_gpio_num, 0);
    gpio_set_level(dev->storage_clk_gpio_num, 0);
    if (dev->reset_gpio_num != -1) {
        gpio_set_level(dev->reset_gpio_num, 1);
    }
    if (dev->ouput_en_gpio_num != -1) {
        if (dev->flags.output_remain_en) {
            gpio_set_level(dev->reset_gpio_num, 0);
        }
        else {
            gpio_set_level(dev->reset_gpio_num, 1);
        }
    }
}

void hc595_write_byte(hc595_dev_t *dev, unsigned char data)
{
    for (int i = 0; i < 8; i++) {
        if (data & 0x80)
            gpio_set_level(dev->data_in_gpio_num, 1);
        else
            gpio_set_level(dev->data_in_gpio_num, 0);
        data <<= 1;
        hc595_shift_clk_once(dev);
    }
    hc595_storage_clk_once(dev);
}

void hc595_write_bytes(hc595_dev_t *dev, unsigned char *data, int data_len)
{
    unsigned char temp;
    for (int n = 0; n < data_len; n++) {
        temp = data[n];
        for (int i = 0; i < 8; i++) {
            if (temp & 0x80)
                gpio_set_level(dev->data_in_gpio_num, 1);
            else
                gpio_set_level(dev->data_in_gpio_num, 0);
            temp <<= 1;
            hc595_shift_clk_once(dev);
        }
    }
    hc595_storage_clk_once(dev);
}

void hc595_reset_shift_reg(hc595_dev_t *dev)
{
    if (dev->reset_gpio_num != -1) {
        // Reset low
        gpio_set_level(dev->reset_gpio_num, 0);
        // Output_en low
        if (!dev->flags.output_remain_en)
            hc595_enable_output(dev, true);
        esp_rom_delay_us(1);
        // Restore Reset and Output_en
        gpio_set_level(dev->reset_gpio_num, 1);
        if (!dev->flags.output_remain_en)
            hc595_enable_output(dev, false);
        esp_rom_delay_us(1);
    }
}

void hc595_reset_storage_reg(hc595_dev_t *dev)
{
    if (dev->reset_gpio_num != -1) {
        // Reset low
        gpio_set_level(dev->reset_gpio_num, 0);
        // Storage clk
        hc595_storage_clk_once(dev);
        esp_rom_delay_us(1);
        // Restore Reset
        gpio_set_level(dev->reset_gpio_num, 1);
        esp_rom_delay_us(1);
    }
}

void hc595_enable_output(hc595_dev_t *dev, int flag)
{
    if (dev->ouput_en_gpio_num != -1) {
        gpio_set_level(dev->ouput_en_gpio_num, !flag);
    }
}

static void hc595_shift_clk_once(hc595_dev_t *dev)
{
    esp_rom_delay_us(1);
    gpio_set_level(dev->shift_clk_gpio_num, 1);
    esp_rom_delay_us(1);
    gpio_set_level(dev->shift_clk_gpio_num, 0);
}

static void hc595_storage_clk_once(hc595_dev_t *dev)
{
    esp_rom_delay_us(1);
    gpio_set_level(dev->storage_clk_gpio_num, 1);
    esp_rom_delay_us(1);
    gpio_set_level(dev->storage_clk_gpio_num, 0);
}
