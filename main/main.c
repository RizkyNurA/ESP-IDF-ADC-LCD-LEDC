#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "driver/ledc.h"
#include "esp_err.h"
#include "sdkconfig.h"
#include "esp_pm.h"
#include "esp_timer.h"
#include "esp_log.h"

#include "driver/i2c_master.h"

#include "adc_driver.h"

#include "led_driver.h"

#define I2C_BUS_PORT I2C_NUM_0
#define I2C_SDA_GPIO GPIO_NUM_21
#define I2C_SCL_GPIO GPIO_NUM_22
#define I2C_MASTER_FREQ_HZ 100000
#define SLAVE_ADDRESS_LCD 0x27
#define LCD_BACKLIGHT 0x08

#define pin_led 2
#define pin_potensio ADC_CHANNEL_0 //VP

static const char* TAG = "I2C_LCD";

static i2c_master_bus_handle_t i2c_bus = NULL;
static i2c_master_dev_handle_t lcd_dev = NULL;

static esp_err_t i2c_master_init(void)
{
    i2c_master_bus_config_t bus_config = {
        .i2c_port = I2C_BUS_PORT,
        .sda_io_num = I2C_SDA_GPIO,
        .scl_io_num = I2C_SCL_GPIO,
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .glitch_ignore_cnt = 7,
        .flags.enable_internal_pullup = true,
    };
    
    ESP_ERROR_CHECK(i2c_new_master_bus(&bus_config, &i2c_bus));
    
    i2c_device_config_t dev_cfg = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = SLAVE_ADDRESS_LCD,
        .scl_speed_hz = I2C_MASTER_FREQ_HZ,
    };
    
    ESP_ERROR_CHECK(i2c_master_bus_add_device(i2c_bus, &dev_cfg, &lcd_dev));

    ESP_LOGI(TAG, "I2C master bus initialized");
    return ESP_OK;
}

static esp_err_t lcd_i2c_write(uint8_t *data, size_t len)
{
    return i2c_master_transmit(lcd_dev, data, len, -1);
}

void lcd_send_data(uint8_t data)
{
    uint8_t data_u = data & 0xF0;
    uint8_t data_l = (data << 4) & 0xF0;

    uint8_t data_t[4] = {
        data_u | 0b00001101 | LCD_BACKLIGHT,  // RS=1
        data_u | 0b00001001 | LCD_BACKLIGHT,
        data_l | 0b00001101 | LCD_BACKLIGHT,
        data_l | 0b00001001 | LCD_BACKLIGHT
    };

    for (int i = 0; i < 4; i++) {
        lcd_i2c_write(&data_t[i], 1);
        esp_rom_delay_us(200);
    }
}

void lcd_send_cmd(uint8_t cmd)
{
    uint8_t data_u = cmd & 0xF0;
    uint8_t data_l = (cmd << 4) & 0xF0;

    uint8_t data_t[4] = { 
        data_u | 0b00001100 | LCD_BACKLIGHT,  // EN=1
        data_u | 0b00001000 | LCD_BACKLIGHT,  // EN=0
        data_l | 0b00001100 | LCD_BACKLIGHT,
        data_l | 0b00001000 | LCD_BACKLIGHT
    };

    for (int i = 0; i < 4; i++) {
        lcd_i2c_write(&data_t[i], 1);
        esp_rom_delay_us(50);
    }
}

void lcd_send_nibble(uint8_t nibble)
{
    uint8_t data = (nibble & 0xF0) | LCD_BACKLIGHT;

    uint8_t seq[2] = {
        data | 0b00000100, // EN=1
        data | 0b00000000  // EN=0
    };

    lcd_i2c_write(seq, 2);
    esp_rom_delay_us(50);
}

void lcd_init(void)
{
    vTaskDelay(pdMS_TO_TICKS(50));

    lcd_send_nibble(0x30);
    vTaskDelay(pdMS_TO_TICKS(5));

    lcd_send_nibble(0x30);
    vTaskDelay(pdMS_TO_TICKS(5));

    lcd_send_nibble(0x30);
    vTaskDelay(pdMS_TO_TICKS(5));

    lcd_send_nibble(0x20); // masuk 4-bit
    vTaskDelay(pdMS_TO_TICKS(5));

    lcd_send_cmd(0x28);
    lcd_send_cmd(0x08);
    lcd_send_cmd(0x01);
    vTaskDelay(pdMS_TO_TICKS(2));
    lcd_send_cmd(0x06);
    lcd_send_cmd(0x0C);
    vTaskDelay(pdMS_TO_TICKS(10));
}

void lcd_put_cur(int row, int col)
{
    switch (row)
    {
        case 0:
            col |= 0x80;
            break;
        case 1:
            col |= 0xC0;
            break;
    }

    lcd_send_cmd (col);
}

void lcd_send_string (char *str)
{
	while (*str) lcd_send_data (*str++);
}

void lcd_send_int(int num)
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

void lcd_send_float(float num, int decimal_places)
{
    if (num < 0) {
        lcd_send_data('-');
        num = -num;
    }

    int int_part = (int)num;
    float frac = num - int_part;

    lcd_send_int(int_part);
    lcd_send_data('.');

    for (int i = 0; i < decimal_places; i++) {
        frac *= 10;
        int digit = (int)frac;
        lcd_send_data(digit + '0');
        frac -= digit;
    }
}

void lcd_clear_row(int row)
{
    lcd_put_cur(row, 0);
    for (int i = 0; i < 16; i++) {
        lcd_send_data(' ');
    }
}

void lcd_clear(void)
{
    lcd_send_cmd(0x01);
    vTaskDelay(pdMS_TO_TICKS(2));;  // minimal 1.5ms
}

void app_main(void)
{
    ESP_ERROR_CHECK(i2c_master_init());
    lcd_init();
    pwm_timer_init(LEDC_LOW_SPEED_MODE,
                   LEDC_TIMER_0,
                   LEDC_TIMER_13_BIT,
                   4000,
                   LEDC_AUTO_CLK);
    pwm_channel_init(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_2, LEDC_TIMER_0, pin_led, 4000);
    
    adc_unit_handle_custom_t adc1;

    adc_channel_handle_custom_t ch0;

    adc_unit_init(&adc1, ADC_UNIT_1);

    adc_channel_init(&adc1, &ch0, pin_potensio, ADC_ATTEN_DB_12);

    static int last_adc_time = 0;
    while(1)
    {
        if (esp_timer_get_time() - last_adc_time > 100) 
        {
            int vp = adc_read_raw(&adc1, &ch0);
            lcd_put_cur(0, 0);
            lcd_send_string("ADC CH0 : ");
            lcd_send_int(vp);
            ESP_LOGI("ADC", "CH0: %d", vp);
            ESP_ERROR_CHECK(ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_2, vp));
            ESP_ERROR_CHECK(ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_2));

            last_adc_time = esp_timer_get_time();
        }
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}
