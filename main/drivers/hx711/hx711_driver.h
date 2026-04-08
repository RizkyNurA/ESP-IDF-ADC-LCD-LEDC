#ifndef HX711_DRIVER_H
#define HX711_DRIVER_H

#include "driver/gpio.h"
#include "esp_err.h"

typedef enum
{
    HX711_GAIN_A_128 = 0, //!< Channel A, gain factor 128
    HX711_GAIN_B_32,      //!< Channel B, gain factor 32
    HX711_GAIN_A_64       //!< Channel A, gain factor 64
} hx711_gain_t;

typedef struct
{
    gpio_num_t dout;
    gpio_num_t pd_sck;
    hx711_gain_t gain;
} hx711_t;

uint32_t read_raw(gpio_num_t dout, gpio_num_t pd_sck, hx711_gain_t gain);

esp_err_t hx711_init(hx711_t *dev);

esp_err_t hx711_power_down(hx711_t *dev, bool down);

esp_err_t hx711_set_gain(hx711_t *dev, hx711_gain_t gain);

esp_err_t hx711_is_ready(hx711_t *dev, bool *ready);

esp_err_t hx711_wait(hx711_t *dev, size_t timeout_ms);

esp_err_t hx711_read_data(hx711_t *dev, int32_t *data);

esp_err_t hx711_read_average(hx711_t *dev, size_t times, int32_t *data);

#endif