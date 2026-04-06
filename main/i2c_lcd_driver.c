#include "i2c_lcd_driver.h"
#include "driver/i2c_master.h"
#include "freertos/FreeRTOS.h"
#include "esp_log.h"

#define I2C_BUS_PORT I2C_NUM_0
#define I2C_SDA_GPIO GPIO_NUM_21
#define I2C_SCL_GPIO GPIO_NUM_22
#define I2C_MASTER_FREQ_HZ 100000
#define SLAVE_ADDRESS_LCD 0x27
#define LCD_BACKLIGHT 0x08

static i2c_master_bus_handle_t i2c_bus = NULL;
static i2c_master_dev_handle_t lcd_dev = NULL;

static esp_err_t lcd_i2c_write(uint8_t *data, size_t len);
static void lcd_send_nibble(uint8_t nibble);
static const char* TAG = "I2C_LCD";

esp_err_t i2c_master_init(void)
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

static void lcd_send_nibble(uint8_t nibble)
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
    ESP_ERROR_CHECK(i2c_master_init());
    
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

