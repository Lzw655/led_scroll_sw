#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "driver/gpio.h"
#include "esp_err.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "hc595.h"
#include "led_scroller.h"

#define DISPLAY_PERIOD          (50)       // us
#define DISPLAY_LED_SIGS        (10)
#define DISPLAY_LED_DATS        (10)

#define DATA_IN_GPIO            (5)
#define SCLK_GPIO               (17)
#define RCLK_GPIO               (16)

static char *TAG = "led_scroller";
static hc595_dev_t hc595_dev = {0};
static TaskHandle_t task_handle;
static int led_nums_max;
static bool scroll_flag = false;
static int scroll_count;
static int scroll_sig_index, scroll_dat_index;
static int scroll_period;
static int gpio_level = 0;

static void refresh_timer_cb(void *args);
static void update_time(void);
static void scroll(uint16_t sig[], uint16_t val[]);
static void refresh_task(void *args);

void led_scroller_init(void)
{
    hc595_dev.data_in_gpio_num = DATA_IN_GPIO;
    hc595_dev.shift_clk_gpio_num = SCLK_GPIO;
    hc595_dev.storage_clk_gpio_num = RCLK_GPIO;
    hc595_dev.ouput_en_gpio_num = -1;
    hc595_dev.reset_gpio_num = -1;
    hc595_dev.flags.output_remain_en = 1;
    hc595_init(&hc595_dev);

    esp_timer_handle_t refresh_timer_handle;
    esp_timer_create_args_t timer_args = {
        .name = "",
        .callback = refresh_timer_cb,
    };
    ESP_ERROR_CHECK(esp_timer_create(&timer_args, &refresh_timer_handle));
    ESP_ERROR_CHECK(esp_timer_start_periodic(refresh_timer_handle, DISPLAY_PERIOD));

    xTaskCreatePinnedToCore(refresh_task, "led_scroller", 2048, NULL, configMAX_PRIORITIES, &task_handle, 1);

    gpio_set_direction(32, GPIO_MODE_OUTPUT);
    gpio_set_level(32, gpio_level);
}

void led_scroller_run(bool flag, uint8_t led_nums, int freq)
{
    uint8_t data[3];
    if (!scroll_flag && flag) {
        led_nums_max = led_nums;
        scroll_period = (1000 * 1000) / (freq * DISPLAY_PERIOD);
        scroll_count = 1;
        scroll_sig_index = scroll_dat_index = 1;
        data[0] = (0xfffe) >> 6;
        data[1] = ((0x1) >> 8) | (((0xfffe) << 2) & 0xff);
        data[2] = (0x1);
        hc595_write_bytes(&hc595_dev, data, 3);
    }
    scroll_flag = flag;
}

static void refresh_timer_cb(void *args)
{
    BaseType_t res;
    xTaskNotifyFromISR(task_handle, 0, eNoAction, &res);
    update_time();
    portYIELD_FROM_ISR(res);
}

static void refresh_task(void *params)
{
    uint8_t data[3];
    uint16_t sig[2];
    uint16_t value[2];
    uint8_t i;

    ESP_LOGI(TAG, "refresh_task begin");
    for (;;) {
        if (scroll_flag) {
            scroll(sig, value);
            for (i = 0; i < 2; i++) {
                xTaskNotifyWait(0, ULONG_MAX, NULL, portMAX_DELAY);
                data[0] = value[i] >> 6;
                data[1] = (sig[i] >> 8) | ((value[i] << 2) & 0xff);
                data[2] = sig[i];
                hc595_write_bytes(&hc595_dev, data, 3);
                // gpio_level = !gpio_level;
                // gpio_set_level(32, gpio_level);
            }
        }
        else {
            vTaskDelay(pdMS_TO_TICKS(100));
        }
    }
}

static void update_time(void)
{
    if (scroll_count < scroll_period) {
        scroll_count++;
    }
    else {
        scroll_count = 1;
        if (scroll_dat_index < DISPLAY_LED_DATS) {
            scroll_dat_index++;
        }
        else {
            scroll_dat_index = 1;
            if (scroll_sig_index < DISPLAY_LED_SIGS + 1) {
                scroll_sig_index++;
            }
            else {
                scroll_sig_index = 1;
            }
        }
    }
}

static void scroll(uint16_t sig[], uint16_t val[])
{
    uint8_t i;
    uint16_t sig_temp[2] = {0}, val_temp[2] = {0xffff, 0xffff};

    if ((scroll_dat_index <= led_nums_max) &&
        (scroll_sig_index > 1 || scroll_sig_index < DISPLAY_LED_SIGS + 1)) {
        sig_temp[1] = 1ULL << (scroll_sig_index - 2);
        for (i = 1; i < led_nums_max - scroll_dat_index + 1; i++) {
            val_temp[1] &= ~(1ULL << (DISPLAY_LED_DATS - i));
        }
    }
    else {
        sig_temp[1] = 1ULL << DISPLAY_LED_SIGS;
    }

    sig_temp[0] = 1ULL << (scroll_sig_index - 1);
    for (i = scroll_dat_index; (i > scroll_dat_index - led_nums_max) && (i > 0); i--) {
            val_temp[0] &= ~(1ULL << (i - 1));
    }

    sig[0] = sig_temp[0];
    val[0] = val_temp[0];
    sig[1] = sig_temp[1];
    val[1] = val_temp[1];
}
