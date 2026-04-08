/* ===================== RTOS ===================== */
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"

/* ===================== ESP ===================== */
#include "esp_err.h"
#include "sdkconfig.h"
#include "esp_pm.h"
#include "esp_timer.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "nvs.h"

/* ===================== COMMON ===================== */
#include "common/types.h"

/* ===================== CONFIG ===================== */
#include "config/config.h"

/* ===================== SYSTEM ===================== */
#include "system/app_context.h"

/* ===================== CORE ===================== */
#include "core/drivers.h"

/* ===================== APP ===================== */
#include "app/app_logic.h"
#include "app/editor.h"

/* ===================== DRIVERS ===================== */
#include "drivers/adc/adc_driver.h"
#include "drivers/led/led_driver.h"
#include "drivers/lcd/i2c_lcd_driver.h"
#include "drivers/lcd/lcd.h"
#include "drivers/hx711/hx711_driver.h"
#include "drivers/button/push_button_driver.h"
#include "driver/gpio.h"

void app_main(void)
{
    esp_err_t ret = nvs_flash_init();

    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || 
        ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }

    ESP_ERROR_CHECK(ret);
    
    app_mutex = xSemaphoreCreateMutex();
    if (app_mutex == NULL) 
    {
        ESP_LOGE("APP", "Failed to create mutex");
        abort();
    }

    app_queue = xQueueCreate(10, sizeof(app_event_t));
    if (app_queue == NULL) {
        ESP_LOGE("APP", "Failed to create queue");
        abort();
    }

    sys_mutex = xSemaphoreCreateMutex();
    if (sys_mutex == NULL)
    {
        ESP_LOGE("APP", "Failed to create mutex");
        abort();
    }

    GPIO_Initialation(  pin_button_left,
                        pin_button_center,
                        pin_button_right,
                        pin_led_2);

    xSemaphoreTake(sys_mutex, portMAX_DELAY);

    sys.scale.dout = pin_dt_hx711;
    sys.scale.pd_sck = pin_sck_hx711;
    sys.scale.gain = HX711_GAIN_A_128;
    ESP_ERROR_CHECK(hx711_init(&sys.scale));

    xSemaphoreGive(sys_mutex);

    lcd_init(I2C_SDA_GPIO, I2C_SCL_GPIO);

    pwm_timer_init(LEDC_LOW_SPEED_MODE,
                   LEDC_TIMER_0,
                   LEDC_TIMER_13_BIT,
                   4000,
                   LEDC_AUTO_CLK);

    pwm_channel_init(LEDC_LOW_SPEED_MODE, 
                    LEDC_CHANNEL_2, 
                    LEDC_TIMER_0, 
                    pin_led_1, 
                    4000);

    editor_init(&app.editor, load_editor_value());

    // tandai selesai
    xSemaphoreTake(app_mutex, portMAX_DELAY);
    app.system_ready = true;
    app.screen = APP_IDLE;
    xSemaphoreGive(app_mutex);
                    
    button_config_t cfg = {
        .debounce_time = 10000,
        .short_press_time = 50000,
        .long_press_time = 1000000,
        .very_long_press_time = 5000000
    };

    button_ctx_t buttons[] = {
    {pin_button_left, {.state = BTN_IDLE}, cfg, left_button_handler},
    {pin_button_center, {.state = BTN_IDLE}, cfg, center_button_handler},
    {pin_button_right, {.state = BTN_IDLE}, cfg, right_button_handler}
    };

    uint8_t button_count = sizeof(buttons) / sizeof(button_ctx_t);

    button_group_t btn_group = {
    .buttons = buttons,
    .count = button_count
    };
    
    xTaskCreate(app_task, "app", 4096, NULL, 6, NULL);
    xTaskCreate(button_task, "btn", 2048, &btn_group, 5, NULL);
    xTaskCreate(adc_task, "adc", 2048, NULL, 5, NULL);
    xTaskCreate(hx711_task, "hx", 2048, &sys.scale, 6, NULL);
    xTaskCreate(lcd_task, "lcd", 4096, NULL, 3, NULL);
    xTaskCreate(led_task, "led", 1024, NULL, 2, NULL);
    
    vTaskDelete(NULL);
}