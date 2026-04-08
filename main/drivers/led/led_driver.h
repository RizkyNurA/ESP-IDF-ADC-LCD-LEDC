#ifndef LED_DRIVER_H
#define LED_DRIVER_H

#include "driver/ledc.h"

void pwm_timer_init(ledc_mode_t mode,
                    ledc_timer_t timer,
                    ledc_timer_bit_t resolution,
                    uint32_t freq_hz,
                    ledc_clk_cfg_t clk_cfg);

void pwm_channel_init(ledc_mode_t mode,
                      ledc_channel_t channel,
                      ledc_timer_t timer,
                      int gpio,
                      uint32_t duty);

                      
#endif

