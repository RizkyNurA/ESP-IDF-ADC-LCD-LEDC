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

#define pin_led 2
#define pin_potensio ADC_CHANNEL_0 //VP

void app_main(void)
{
    ESP_ERROR_CHECK(i2c_master_init());
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
    while(1)
    {
        if (esp_timer_get_time() - last_adc_time > 100) 
        {
            int vp = adc_read_raw(&adc1, &ch0);
            lcd_put_cur(0, 0);
            lcd_send_string("ADC CH0 : ");
            lcd_send_int(vp);
            ESP_LOGI("ADC", "CH0: %d", vp);
            ESP_ERROR_CHECK(ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_2, vp));
            ESP_ERROR_CHECK(ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_2));

            last_adc_time = esp_timer_get_time();
        }
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}
