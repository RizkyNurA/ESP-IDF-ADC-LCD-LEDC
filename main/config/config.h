#ifndef CONFIG_H
#define CONFIG_H

#include "driver/gpio.h"

/* ===================== HX711 (SISI KIRI - URUTAN TUA KE MUDA) ===================== */
/* Pin DT (Input Only) menggunakan GPIO 39, 36, 35, 34 */
#define pin_dt_hx711_4      GPIO_NUM_26  // HX711 1 (Paling Tua)
#define pin_dt_hx711_3      GPIO_NUM_25
#define pin_dt_hx711_2      GPIO_NUM_14
#define pin_dt_hx711        GPIO_NUM_13  // HX711 4 (Paling Muda)

/* Pin SCK (Output) menggunakan GPIO 33, 32, 26, 25 */
#define pin_sck_hx711_4     GPIO_NUM_33  // HX711 1 (Paling Tua)
#define pin_sck_hx711_3     GPIO_NUM_32
#define pin_sck_hx711_2     GPIO_NUM_27
#define pin_sck_hx711       GPIO_NUM_12  // HX711 4 (Paling Muda)

#define CONFIG_NUM_LOADCELL 1

/* ===================== LCD (SISI KANAN) ==============rr======= */
#define pin_sda_lcd       GPIO_NUM_21
#define pin_scl_lcd       GPIO_NUM_22

/* ===================== BUTTON (SISI KANAN) ===================== */
#define pin_button_left   GPIO_NUM_36
#define pin_button_center GPIO_NUM_39
#define pin_button_right  GPIO_NUM_34

/* ===================== RELAY (SISI KANAN) ===================== */
#define pin_ch1_relay     GPIO_NUM_5
#define pin_ch2_relay     GPIO_NUM_18
#define pin_ch3_relay     GPIO_NUM_19

/* ===================== THRESHOLD & OTHERS ===================== */
#define ALARM1_THRESHOLD  1000   
#define ALARM2_THRESHOLD  5000   
#define ALARM3_THRESHOLD  10000  

#define interval_lcd      5000000

#define BLINK_THRESHOLD   3
#define EDITOR_CHAR_MIN   0
#define EDITOR_CHAR_MAX   9
#define EDITOR_CHAR_RANGE (EDITOR_CHAR_MAX - EDITOR_CHAR_MIN + 1)
#define EDITOR_ROW        1
#define EDITOR_COL_START  0

#define SAMPLE_CALIB_VALUE 20

#endif