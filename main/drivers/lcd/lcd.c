#include "lcd.h"
#include "i2c_lcd_driver.h"
#include "driver/gpio.h"

// ================= BASIC =================

void lcd_init_wrapper(gpio_num_t pin_sda, gpio_num_t pin_scl)
{
    lcd_init(pin_sda, pin_scl);
}

void lcd_clear(void)
{
    lcd_send_cmd(0x01);
}

void lcd_clear_row(int row)
{
    lcd_set_cursor(row, 0);
    for (int i = 0; i < 16; i++) {
        lcd_send_data(' ');
    }
}

void lcd_home(void)
{
    lcd_send_cmd(0x02);
}

void lcd_set_cursor(uint8_t row, uint8_t col)
{
    uint8_t addr = (row == 0) ? 0x80 : 0xC0;
    lcd_send_cmd(addr + col);
}

// ================= WRITE =================

void lcd_write_char(char c)
{
    lcd_send_data(c);
}

void lcd_write_string(const char *str)
{
    while (*str)
    {
        lcd_write_char(*str++);
    }
}

void lcd_write_int(int num)
{
    char buf[12]; // cukup untuk int32
    int i = 0;

    if (num == 0) {
        lcd_send_data('0');
        return;
    }

    if (num < 0) {
        lcd_send_data('-');
        num = -num;
    }

    while (num > 0) {
        buf[i++] = (num % 10) + '0';
        num /= 10;
    }

    // reverse
    for (int j = i - 1; j >= 0; j--) {
        lcd_send_data(buf[j]);
    }
}

void lcd_write_float(float num, int decimal_places)
{
    if (num < 0) {
        lcd_send_data('-');
        num = -num;
    }

    int scale = 1;
    for (int i = 0; i < decimal_places; i++)
        scale *= 10;

    // rounding
    int scaled = (int)(num * scale + 0.5f);

    int int_part = scaled / scale;
    int frac_part = scaled % scale;

    lcd_write_int(int_part);
    lcd_send_data('.');

    // print leading zero kalau perlu
    int divisor = scale / 10;
    while (divisor > 0) {
        int digit = frac_part / divisor;
        lcd_send_data('0' + digit);
        frac_part %= divisor;
        divisor /= 10;
    }
}

void lcd_write_float_2d(float val)
{
    if (val < 0) {
        lcd_write_char('-');
        val = -val;
    }

    int int_part = (int)val;
    int frac = (int)((val - int_part) * 100);

    // paksa 2 digit integer
    lcd_write_char('0' + (int_part / 10) % 10);
    lcd_write_char('0' + int_part % 10);

    lcd_write_char('.');

    // paksa 2 digit decimal
    lcd_write_char('0' + (frac / 10) % 10);
    lcd_write_char('0' + frac % 10);
}

// ================= CONTROL =================

void lcd_display_on(bool cursor, bool blink)
{
    uint8_t cmd = 0x08; // base

    cmd |= 0x04; // display ON

    if (cursor) cmd |= 0x02;
    if (blink)  cmd |= 0x01;

    lcd_send_cmd(cmd);
}

void lcd_shift_cursor_left(void)
{
    lcd_send_cmd(0x10);
}

void lcd_shift_cursor_right(void)
{
    lcd_send_cmd(0x14);
}

// ================= CUSTOM CHAR =================

void lcd_create_char(uint8_t location, uint8_t *pattern)
{
    location &= 0x07; // max 8 char

    lcd_send_cmd(0x40 + (location * 8));

    for (int i = 0; i < 8; i++)
    {
        lcd_send_data(pattern[i]);
    }
}

void lcd_write_custom(uint8_t location)
{
    lcd_send_data(location);
}