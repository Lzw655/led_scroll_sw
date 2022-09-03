#include <math.h>
#include <stdio.h>
#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "esp_err.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "led_7seg.h"
#include "led_scroller.h"
#include "rotary_encoder.h"
#include "notify.h"

#define LED_SCROLLER_FREQ           (1000)
#define LED_SCROLLER_FREQ_MAX       (2000)
#define LED_SCROLLER_FREQ_MIN       (1)

static char *TAG = "app_main";

led_7seg_handle_t module_handle;
rotary_encoder_handle_t encoder_handle;
TaskHandle_t task_freq_change_handle;
int8_t encoder_count;
int led_scroller_freq = LED_SCROLLER_FREQ;
bool led_7seg_encoder_enable = false;
bool sw_flag = false;
bool notify_flag = false;

static void on_encoder_act_cb(rotary_encoder_handle_t handle, int count, int period_ms);
static void on_sw_cb(rotary_encoder_handle_t handle);
static void task_freq_change(void *param);
static void on_notify_cb(void);

void app_main(void)
{
    ESP_LOGI(TAG, "hello world");
    // Initialize NVS
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        // NVS partition was truncated and needs to be erased
        // Retry nvs_flash_init
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    ESP_ERROR_CHECK(err);
    nvs_handle_t nvs_handle;
    ESP_ERROR_CHECK(nvs_open("nvs", NVS_READWRITE, &nvs_handle));
    err = nvs_get_i32(nvs_handle, "freq", &led_scroller_freq);
    switch (err) {
        case ESP_OK:
            ESP_LOGI(TAG, "nvs frequency = %d\n", led_scroller_freq);
            break;
        case ESP_ERR_NVS_NOT_FOUND:
            ESP_ERROR_CHECK(nvs_set_i32(nvs_handle, "freq", led_scroller_freq));
            break;
        default :
            ESP_LOGI(TAG, "Error (%s) reading!\n", esp_err_to_name(err));
    }
    ESP_ERROR_CHECK(nvs_commit(nvs_handle));
    nvs_close(nvs_handle);

    ESP_LOGI(TAG, "Init led_7seg");
    led_7seg_config_t module_config = {
        .bits_num = 4,
        .din_gpio_num = 33,
        .sclk_gpio_num = 25,
        .rclk_gpio_num = 26,
        .refresh_period_per_bit = 2,
        .blink = {
            .enable = true,
            .period = 1000,
            .task_core = tskNO_AFFINITY,
            .task_priority = tskIDLE_PRIORITY + 1,
        },
    };
    led_7seg_init(&module_config, &module_handle);
    led_7seg_set_display_int(led_scroller_freq, module_handle);

    ESP_LOGI(TAG, "Init led_scroller");
    led_scroller_init();
    led_scroller_run(true, 5, led_scroller_freq);

    ESP_LOGI(TAG, "Init rotary_encoder");
    rotary_encoder_config_t encoder_cfg = {
        .a_gpio_num = 14,
        .b_gpio_num = 12,
        .sw_gpio_num = 13,
        .encoder_filter_ns = 1000,
        .encoder_period_ms = 100,
        .on_encoder_act = on_encoder_act_cb,
        .sw_filter_ms = 20,
        .on_sw_press = on_sw_cb,
    };
    rotary_encoder_create(&encoder_cfg, &encoder_handle);

    notify_init(on_notify_cb);

    xTaskCreatePinnedToCore(task_freq_change, "freq_change", 2048, NULL, tskIDLE_PRIORITY + 1, &task_freq_change_handle, tskNO_AFFINITY);
    // xTaskNotify(task_freq_change_handle, 0, eNoAction);
}

static void on_encoder_act_cb(rotary_encoder_handle_t handle, int count, int period_ms)
{
    if (led_7seg_encoder_enable) {
        encoder_count = count;
        xTaskNotify(task_freq_change_handle, 0, eNoAction);
    }
}

static void on_sw_cb(rotary_encoder_handle_t handle)
{
    ESP_DRAM_LOGI(TAG, "swtich push");
    sw_flag = true;
    xTaskNotify(task_freq_change_handle, 0, eNoAction);
}

static bool check_sw_state(void)
{
    if (sw_flag) {
        sw_flag = false;
        return true;
    }

    return false;
}

static void on_notify_cb(void)
{
    ESP_DRAM_LOGI(TAG, "notify in");
    notify_flag = true;
    xTaskNotify(task_freq_change_handle, 0, eNoAction);
}

static bool check_notify_state(void)
{
    if (notify_flag) {
        notify_flag = false;
        return true;
    }

    return false;
}

static void resset_led_7seg(void)
{
    led_7seg_encoder_enable = true;
    encoder_count = 0;
}

static void write_freq_to_nvs(int freq)
{
    nvs_handle_t nvs_handle;
    ESP_ERROR_CHECK(nvs_open("nvs", NVS_READWRITE, &nvs_handle));
    ESP_ERROR_CHECK(nvs_set_i32(nvs_handle, "freq", freq));
    ESP_ERROR_CHECK(nvs_commit(nvs_handle));
    nvs_close(nvs_handle);
}

static void task_freq_change(void *param)
{
    ESP_LOGI(TAG, "task_freq_change start");

    int8_t old_num, new_num;
    for (;;) {
begin:
        xTaskNotifyWait(0, ULONG_MAX, NULL, portMAX_DELAY);
        notify_out();
        vTaskDelay(pdMS_TO_TICKS(200));
        xTaskNotifyWait(0, ULONG_MAX, NULL, 0);
        sw_flag = false;
        notify_flag = false;
        ESP_LOGI(TAG, "task_freq_change loop begin");
        led_scroller_run(false, 0, 0);
        resset_led_7seg();
        for (int i = 0; i < 4; i++) {
            led_7seg_blink(1 << i, true, module_handle);
            while (1) {
                xTaskNotifyWait(0, ULONG_MAX, NULL, portMAX_DELAY);
                if (check_notify_state()) {
                    notify_out();
                    vTaskDelay(pdMS_TO_TICKS(200));
                    xTaskNotifyWait(0, ULONG_MAX, NULL, 0);
                    led_7seg_blink(0xff, false, module_handle);
                    led_scroller_freq = (led_scroller_freq > LED_SCROLLER_FREQ_MAX) ? LED_SCROLLER_FREQ_MAX : led_scroller_freq;
                    led_scroller_freq = (led_scroller_freq < LED_SCROLLER_FREQ_MIN) ? LED_SCROLLER_FREQ_MIN : led_scroller_freq;
                    write_freq_to_nvs(led_scroller_freq);
                    led_scroller_run(true, 5, led_scroller_freq);
                    led_7seg_set_display_int(led_scroller_freq, module_handle);
                    goto begin;
                }
                if (check_sw_state()) {
                    led_7seg_blink(1 << i, false, module_handle);
                    if (i == 3) {
                        notify_out();
                        vTaskDelay(pdMS_TO_TICKS(200));
                        xTaskNotifyWait(0, ULONG_MAX, NULL, 0);
                        led_scroller_freq = (led_scroller_freq > LED_SCROLLER_FREQ_MAX) ? LED_SCROLLER_FREQ_MAX : led_scroller_freq;
                        led_scroller_freq = (led_scroller_freq < LED_SCROLLER_FREQ_MIN) ? LED_SCROLLER_FREQ_MIN : led_scroller_freq;
                        write_freq_to_nvs(led_scroller_freq);
                        led_scroller_run(true, 5, led_scroller_freq);
                        led_7seg_set_display_int(led_scroller_freq, module_handle);
                    }
                    break;
                }
                old_num = (int)(led_scroller_freq / pow(10, i)) % 10;
                new_num = (old_num + encoder_count) < 0 ? 9 : (old_num + encoder_count) % 10;
                led_scroller_freq = led_scroller_freq + (new_num - old_num) * pow(10, i);
                ESP_LOGI(TAG, "led_scroller freq: %d", led_scroller_freq);
                led_7seg_set_display_int(led_scroller_freq, module_handle);
            }
        }
    }
}
