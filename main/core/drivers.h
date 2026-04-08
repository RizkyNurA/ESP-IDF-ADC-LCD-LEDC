#ifndef DRIVERS_H
#define DRIVERS_H

#include "driver/gpio.h"
#include "push_button_driver.h"

void GPIO_Initialation(gpio_num_t left_button, 
                        gpio_num_t center_button,
                        gpio_num_t right_button,
                        gpio_num_t second_led);

void app_task(void *pv);

// ADC
void adc_task(void *pv);

// HX711
void hx711_task(void *pv);

// LED
void led_task(void *pv);

// LCD
void lcd_task(void *pv);

// Button
void button_task(void *pv);

// Button handlers
void left_button_handler(press_type_t event);
void right_button_handler(press_type_t event);
void center_button_handler(press_type_t event);


#endif