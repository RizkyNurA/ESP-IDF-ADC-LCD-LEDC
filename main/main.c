#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_err.h"
#include "sdkconfig.h"
#include "esp_pm.h"
#include "esp_timer.h"
#include "esp_log.h"

#include "adc_driver.h"

#include "led_driver.h"

#include "i2c_lcd_driver.h"

#include "hx711_driver.h"

#include "push_button_driver.h"

#define pin_potensio ADC_CHANNEL_0 //VP
#define pin_sck_hx711 GPIO_NUM_5
#define pin_dt_hx711 GPIO_NUM_4

#define pin_led_1 GPIO_NUM_2
#define pin_led_2 GPIO_NUM_19

#define pin_button_left GPIO_NUM_32
#define pin_button_center GPIO_NUM_35
#define pin_button_right GPIO_NUM_34

#define interval_adc_potensio 100000
#define interval_lcd 5000000

#define BLINK_THRESHOLD 3
#define EDITOR_CHAR_MIN 0
#define EDITOR_CHAR_MAX 5
#define EDITOR_CHAR_RANGE (EDITOR_CHAR_MAX - EDITOR_CHAR_MIN + 1)

volatile uint32_t duty = 0;
int32_t raw = 0;
volatile bool toggle_state = 0;

static lcd_cursor_t lcd_cursor = {
    .row = 1,
    .col = 12,
    .active = true,
    .display_char = '_'
};

void GPIO_Initialation()
{
    gpio_reset_pin(pin_button_left);
    gpio_set_direction(pin_button_left, GPIO_MODE_INPUT);

    gpio_reset_pin(pin_button_center);
    gpio_set_direction(pin_button_center, GPIO_MODE_INPUT);

    gpio_reset_pin(pin_button_right);
    gpio_set_direction(pin_button_right, GPIO_MODE_INPUT);
    
    gpio_reset_pin(pin_led_2);
    gpio_set_direction(pin_led_2, GPIO_MODE_OUTPUT);
    gpio_set_level(pin_led_2, 1);
    
}

void left_button_handler(press_type_t event) 
{
    if (event == PRESS_SHORT && lcd_cursor.active) 
    {
        if (lcd_cursor.col > 0) lcd_cursor.col--;
    }
}

void right_button_handler(press_type_t event) 
{
    if (event == PRESS_SHORT && lcd_cursor.active) 
    {
        if (lcd_cursor.col < 15) lcd_cursor.col++;
    }
}

void center_button_handler(press_type_t event) 
{
    if (event == PRESS_SHORT) 
    {
        lcd_cursor.active = !lcd_cursor.active; // toggle aktif/nonaktif
    }
}

void button_task(void *pv)
{
    button_group_t *grp = (button_group_t *)pv;

    while (1)
    {
        for (int i = 0; i < grp->count; i++)
        {
            press_type_t event = button_update(
                &grp->buttons[i].state,
                grp->buttons[i].pin,
                &grp->buttons[i].cfg
            );

            if (event != PRESS_NONE && grp->buttons[i].callback != NULL) {
                grp->buttons[i].callback(event);
            }
        }

        vTaskDelay(pdMS_TO_TICKS(10));
    }
    
}

void adc_task(void *pv)
{
    adc_unit_handle_custom_t adc1;
    adc_channel_handle_custom_t ch0;

    adc_unit_init(&adc1, ADC_UNIT_1);
    adc_channel_init(&adc1, &ch0, pin_potensio, ADC_ATTEN_DB_12);

    while (1)
    {
        int vp = adc_read_raw(&adc1, &ch0);
        duty = (vp * 8191) / 4095;

        ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_2, duty);
        ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_2);

        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

void hx711_task(void *pv)
{
    hx711_t *scale = (hx711_t *)pv;
    bool ready;

    while (1)
    {
        hx711_is_ready(scale, &ready);

        if (ready)
        {
            hx711_read_data(scale, &raw);
        }

        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

void lcd_task(void *pv)
{
    while (1) 
    {
        lcd_put_cur(0, 0);
        lcd_send_string("POT :");
        lcd_send_int(duty);

        lcd_put_cur(1, 0);
        lcd_send_string("LC  :");
        lcd_send_int(raw);

        // tampilkan cursor
        lcd_put_cur(lcd_cursor.row, lcd_cursor.col);
        lcd_send_data(lcd_cursor.display_char);

        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

void led_task(void *pv)
{
    while (1)
    {
        gpio_set_level(pin_led_2, toggle_state);
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

void app_main(void)
{
    GPIO_Initialation();

    hx711_t scale = {
        .dout = pin_dt_hx711,
        .pd_sck = pin_sck_hx711,
        .gain = HX711_GAIN_A_128
    };

    ESP_ERROR_CHECK(hx711_init(&scale));

    lcd_init();

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
                    

    button_config_t cfg = {
        .debounce_time = 10000,
        .short_press_time = 50000,
        .long_press_time = 1000000
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

    xTaskCreate(button_task, "btn", 2048, &btn_group, 5, NULL);
    xTaskCreate(adc_task, "adc", 2048, NULL, 5, NULL);
    xTaskCreate(hx711_task, "hx", 2048, &scale, 5, NULL);
    xTaskCreate(lcd_task, "lcd", 4096, NULL, 3, NULL);
    xTaskCreate(led_task, "led", 1024, NULL, 2, NULL);
    
    vTaskDelete(NULL);
}