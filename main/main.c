#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "esp_err.h"
#include "sdkconfig.h"
#include "esp_pm.h"
#include "esp_timer.h"
#include "esp_log.h"

#include "adc_driver.h"

#include "led_driver.h"

#include "i2c_lcd_driver.h"

#include "hx711_driver.h"

#define pin_led 2
#define pin_potensio ADC_CHANNEL_0 //VP
#define pin_sck_hx711 GPIO_NUM_5
#define pin_dt_hx711 GPIO_NUM_4

void app_main(void)
{
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
    pwm_channel_init(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_2, LEDC_TIMER_0, pin_led, 4000);
    
    adc_unit_handle_custom_t adc1;

    adc_channel_handle_custom_t ch0;

    adc_unit_init(&adc1, ADC_UNIT_1);

    adc_channel_init(&adc1, &ch0, pin_potensio, ADC_ATTEN_DB_12);

    static int last_adc_time = 0;
    static int last_load_cell_time = 0;
    static int last_lcd_time = 0;
    int vp = 0;
    uint32_t duty = 0;
    bool ready;

    while(1)
    {
        if (esp_timer_get_time() - last_adc_time > 100000) 
        {
            vp = adc_read_raw(&adc1, &ch0);
            duty = (vp * 8191) / 4095;
            ESP_LOGI("ADC", "CH0: %d", duty);
            ESP_ERROR_CHECK(ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_2, vp));
            ESP_ERROR_CHECK(ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_2));

            last_adc_time = esp_timer_get_time();
        }

        hx711_is_ready(&scale, &ready);

        if (ready)
        {
            if (hx711_read_data(&scale, &raw) == ESP_OK)
            {
                printf("RAW: %ld\n", raw);
            }
        }
        
        if (esp_timer_get_time() - last_lcd_time > 5000000)
        {
            lcd_put_cur(0, 0);
            lcd_send_string("POT :");
            lcd_send_int(duty);
            
            lcd_put_cur(1, 0);
            lcd_send_string("LC  :");
            lcd_send_int(raw);
        }

        
        vTaskDelay(pdMS_TO_TICKS(50));
    }
}
