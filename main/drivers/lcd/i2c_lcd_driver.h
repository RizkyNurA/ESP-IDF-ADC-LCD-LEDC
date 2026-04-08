#ifndef I2C_LCD_DRIVER_H
#define I2C_LCD_DRIVER_H

#include "esp_err.h"
#include "driver/i2c_master.h"
#include "driver/gpio.h"

typedef struct {
    int row;
    int col;
    bool active;
    char display_char;
} lcd_cursor_t;

esp_err_t i2c_master_init(gpio_num_t pin_sda, gpio_num_t pin_scl);

void lcd_init(gpio_num_t pin_sda_, gpio_num_t pin_scl_);

void lcd_send_cmd(uint8_t cmd);

void lcd_send_data(uint8_t data);



#endif