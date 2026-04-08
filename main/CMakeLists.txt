idf_component_register(
    SRCS 
        "main.c"

        "app/app_logic.c"
        "app/editor.c"

        "system/app_context.c"

        "core/drivers.c"

        "drivers/adc/adc_driver.c"
        "drivers/button/push_button_driver.c"
        "drivers/hx711/hx711_driver.c"
        "drivers/lcd/i2c_lcd_driver.c"
        "drivers/lcd/lcd.c"
        "drivers/led/led_driver.c"

    INCLUDE_DIRS 
        "."
        "app"
        "system"
        "core"
        "common"
        "config"
        "drivers"
        "drivers/adc"
        "drivers/button"
        "drivers/hx711"
        "drivers/lcd"
        "drivers/led"
)