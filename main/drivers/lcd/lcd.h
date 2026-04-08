#ifndef LCD_WRAPPER_H
#define LCD_WRAPPER_H

#include <stdint.h>
#include <stdbool.h>

// init
void lcd_init_wrapper(void);

// basic
void lcd_clear(void);
void lcd_clear_row(int row);
void lcd_home(void);
void lcd_set_cursor(uint8_t row, uint8_t col);

// write
void lcd_write_char(char c);
void lcd_write_string(const char *str);
void lcd_write_int(int val);
void lcd_write_float(float num, int decimal_places);

// control
void lcd_display_on(bool cursor, bool blink);
void lcd_shift_cursor_left(void);
void lcd_shift_cursor_right(void);

// custom
void lcd_create_char(uint8_t location, uint8_t *pattern);
void lcd_write_custom(uint8_t location);

#endif