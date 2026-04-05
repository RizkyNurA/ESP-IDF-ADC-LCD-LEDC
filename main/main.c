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

#define pin_led_1 2
#define pin_led_2 19
#define pin_potensio ADC_CHANNEL_0 //VP
#define pin_sck_hx711 GPIO_NUM_5
#define pin_dt_hx711 GPIO_NUM_4
#define pin_button 23

#define interval_adc_potensio 100000
#define interval_lcd 5000000

void blink_task_1(void *pvParameters)
{
    gpio_reset_pin(pin_led_1);
    gpio_set_direction(pin_led_1, GPIO_MODE_OUTPUT);

    while(1)
    {
        gpio_set_level(pin_led_1, 1);
        vTaskDelay(pdMS_TO_TICKS(500));
        gpio_set_level(pin_led_1, 0);
        vTaskDelay(pdMS_TO_TICKS(500));
    }
}

void blink_task_2(void *pvParameters)
{
    gpio_reset_pin(pin_led_2);
    gpio_set_direction(pin_led_2, GPIO_MODE_OUTPUT);

    while(1)
    {
        gpio_set_level(pin_led_2, 1);
        vTaskDelay(pdMS_TO_TICKS(200));
        gpio_set_level(pin_led_2, 0);
        vTaskDelay(pdMS_TO_TICKS(200));
    }
}


void GPIO_Initialation(){

    gpio_reset_pin(pin_button);
    gpio_set_direction(pin_button, GPIO_MODE_INPUT);
    gpio_set_pull_mode(pin_button, GPIO_PULLDOWN_ONLY);
    
    gpio_reset_pin(pin_led_2);
    gpio_set_direction(pin_led_2, GPIO_MODE_OUTPUT);
    gpio_set_level(pin_led_2, 1);
    

}

void app_main(void)
{
    GPIO_Initialation();

    button_config_t button_1_config = {
        .debounce_time      = 10000,
        .short_press_time   = 50000,
        .long_press_time    = 1000000    
    };

    hx711_t scale = {
        .dout = pin_dt_hx711,
        .pd_sck = pin_sck_hx711,
        .gain = HX711_GAIN_A_128
    };

    ESP_ERROR_CHECK(hx711_init(&scale));

    int32_t raw;

    lcd_init();
    pwm_timer_init(LEDC_LOW_SPEED_MODE,
                   LEDC_TIMER_0,
                   LEDC_TIMER_13_BIT,
                   4000,
                   LEDC_AUTO_CLK);
    pwm_channel_init(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_2, LEDC_TIMER_0, pin_led_1, 4000);
    
    adc_unit_handle_custom_t adc1;

    adc_channel_handle_custom_t ch0;

    adc_unit_init(&adc1, ADC_UNIT_1);

    adc_channel_init(&adc1, &ch0, pin_potensio, ADC_ATTEN_DB_12);

    button_t button_1_state = { .state = BTN_IDLE };
    static int last_adc_time = 0;
    static int last_lcd_time = 0;
    int vp = 0;
    uint32_t duty = 0;
    bool ready;
    bool toggle_state = 0;

    while(1)
    {

        press_type_t event =  button_update(&button_1_state, pin_button, &button_1_config);
        if (event == PRESS_SHORT) {
            ESP_LOGI("BTN", "SHORT");
            toggle_state = !toggle_state;
        }
        else if (event == PRESS_LONG) {
            ESP_LOGI("BTN", "LONG");
            toggle_state = 0;
        }

        if(toggle_state)
        {
            gpio_set_level(pin_led_2, 1);
        }
        else 
        {
            gpio_set_level(pin_led_2, 0);
        }

        if (esp_timer_get_time() - last_adc_time > interval_adc_potensio) 
        {
            vp = adc_read_raw(&adc1, &ch0);
            duty = (vp * 8191) / 4095;
            ESP_LOGI("ADC", "CH0: %d", duty);
            ESP_ERROR_CHECK(ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_2, duty));
            ESP_ERROR_CHECK(ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_2));

            last_adc_time = esp_timer_get_time();
        }

        hx711_is_ready(&scale, &ready);

        if (ready)
        {
            if (hx711_read_data(&scale, &raw) == ESP_OK)
            {
                //ESP_LOGI("RAW", "%d", raw);
            }
        }

        if (esp_timer_get_time() - last_lcd_time > interval_lcd)
        {
            lcd_put_cur(0, 0);
            lcd_send_string("POT :");
            lcd_send_int(duty);
            
            lcd_put_cur(1, 0);
            lcd_send_string("LC  :");
            lcd_send_int(raw);
            last_lcd_time = esp_timer_get_time();
        }

        
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}