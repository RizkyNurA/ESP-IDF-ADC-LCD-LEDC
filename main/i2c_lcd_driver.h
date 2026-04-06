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

void lcd_send_data(uint8_t data);

void lcd_put_cur(int row, int col);

void lcd_send_string (char *str);

void lcd_send_float(float num, int decimal_places);

void lcd_send_int(int num);

void lcd_clear(void);

void lcd_clear_row(int row);

void lcd_create_char(uint8_t location, uint8_t charmap[]);


#endif