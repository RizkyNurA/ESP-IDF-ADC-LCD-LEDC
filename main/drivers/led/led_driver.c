#include "led_driver.h"

void pwm_timer_init(ledc_mode_t mode,
                    ledc_timer_t timer,
                    ledc_timer_bit_t resolution,
                    uint32_t freq_hz,
                    ledc_clk_cfg_t clk_cfg)
{
    ledc_timer_config_t t = {
        .speed_mode      = mode,
        .timer_num       = timer,
        .duty_resolution = resolution,
        .freq_hz         = freq_hz,
        .clk_cfg         = clk_cfg
    };

    ESP_ERROR_CHECK(ledc_timer_config(&t));
}

void pwm_channel_init(ledc_mode_t mode,
                      ledc_channel_t channel,
                      ledc_timer_t timer,
                      int gpio,
                      uint32_t duty)
{
    ledc_channel_config_t ch = {
        .speed_mode = mode,
        .channel    = channel,
        .timer_sel  = timer,
        .gpio_num   = gpio,
        .duty       = duty,
        .hpoint     = 0
    };

    ESP_ERROR_CHECK(ledc_channel_config(&ch));
}