#include "driver/gpio.h"
#include "rotary_encoder.h"

static void IRAM_ATTR a_isr_handler(void* arg);
static void IRAM_ATTR b_isr_handler(void* arg);
static void IRAM_ATTR sw_isr_handler(void* arg);

void rotary_encoder_init(rotary_encoder_dev_t *dev)
{
    uint64_t gpio_pin_mask = 0;
    /* Init necessary gpio */
    gpio_pin_mask = (1ULL << (dev->a_gpio_num)) | (1ULL << (dev->b_gpio_num));
    gpio_config_t io_conf = {};
    io_conf.intr_type = GPIO_INTR_ANYEDGE;
    io_conf.mode = GPIO_MODE_INPUT;
    io_conf.pin_bit_mask = gpio_pin_mask;
    io_conf.pull_down_en = 1;
    io_conf.pull_up_en = 0;
    ESP_ERROR_CHECK(gpio_config(&io_conf));

    /* Init unnecessary gpio */
    if (dev->sw_gpio_num != -1) {
        gpio_pin_mask = (1 << dev->sw_gpio_num);
        io_conf.intr_type = GPIO_INTR_NEGEDGE;
        io_conf.pull_down_en = 0;
        io_conf.pull_up_en = 1;
        io_conf.pin_bit_mask = gpio_pin_mask;
        ESP_ERROR_CHECK(gpio_config(&io_conf));
    }

    // Install gpio isr service
    gpio_install_isr_service(0);
    // Hook isr handler for specific gpio pin
    gpio_isr_handler_add(dev->a_gpio_num, a_isr_handler, (void*)dev);
    gpio_isr_handler_add(dev->b_gpio_num, b_isr_handler, (void*)dev);
    gpio_isr_handler_add(dev->sw_gpio_num, sw_isr_handler, (void*)dev);
}

void rotary_encoder_count_register(rotary_encoder_dev_t *dev, int *count)
{
    dev->count = count;
}

static void IRAM_ATTR a_isr_handler(void* arg)
{
    rotary_encoder_dev_t *dev = (rotary_encoder_dev_t *)arg;
    int a_level = gpio_get_level(dev->a_gpio_num);
    int b_level = gpio_get_level(dev->b_gpio_num);

    if (dev->count && (a_level && (!b_level)) || ((!a_level) && b_level)) {
        (*(dev->count))++;
    }
}

static void IRAM_ATTR b_isr_handler(void* arg)
{
    rotary_encoder_dev_t *dev = (rotary_encoder_dev_t *)arg;
    int a_level = gpio_get_level(dev->a_gpio_num);
    int b_level = gpio_get_level(dev->b_gpio_num);

    if (dev->count && (a_level && (!b_level)) || ((!a_level) && b_level)) {
        (*(dev->count))--;
    }
}

static void IRAM_ATTR sw_isr_handler(void* arg)
{
    rotary_encoder_dev_t *dev = (rotary_encoder_dev_t *)arg;
    if (dev->sw_callback) {
        dev->sw_callback();
    }
}
