#ifndef CONFIG_H
#define CONFIG_H

#include "driver/gpio.h"

//#define pin_potensio ADC_CHANNEL_0 //VP
#define pin_sck_hx711 GPIO_NUM_34
#define pin_dt_hx711 GPIO_NUM_18

#define pin_sck_hx711_2 GPIO_NUM_35
#define pin_dt_hx711_2 GPIO_NUM_19

#define pin_sck_hx711_3 GPIO_NUM_36
#define pin_dt_hx711_3 GPIO_NUM_25

#define pin_sck_hx711_3 GPIO_NUM_39
#define pin_dt_hx711_3 GPIO_NUM_26

// #define pin_led_1 GPIO_NUM_2
// #define pin_led_2 GPIO_NUM_19

#define pin_sda_lcd GPIO_NUM_21
#define pin_scl_lcd GPIO_NUM_22

#define pin_button_left GPIO_NUM_12
#define pin_button_center GPIO_NUM_13
#define pin_button_right GPIO_NUM_15

#define pin_ch1_relay GPIO_NUM_2
#define pin_ch2_relay GPIO_NUM_4
#define pin_ch3_relay GPIO_NUM_5

//#define interval_adc_potensio 100000
#define interval_lcd 5000000

#define BLINK_THRESHOLD 3
#define EDITOR_CHAR_MIN 0
#define EDITOR_CHAR_MAX 9
#define EDITOR_CHAR_RANGE (EDITOR_CHAR_MAX - EDITOR_CHAR_MIN + 1)
#define EDITOR_ROW 1
#define EDITOR_COL_START 12

#define SAMPLE_CALIB_VALUE 20

#endif // CONFIG_H