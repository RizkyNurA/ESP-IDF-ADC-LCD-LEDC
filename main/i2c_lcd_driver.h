#ifndef I2C_LCD_DRIVER_H
#define I2C_LCD_DRIVER_H

#include "esp_err.h"
#include "driver/i2c_master.h"

typedef struct {
    int row;
    int col;
    bool active;
    char display_char;
} lcd_cursor_t;

esp_err_t i2c_master_init(void);

void lcd_init(void);

void lcd_send_cmd(uint8_t cmd);

void lcd_send_data(uint8_t data);



#endif