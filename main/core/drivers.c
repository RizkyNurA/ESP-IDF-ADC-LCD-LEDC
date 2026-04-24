#include "config.h"
#include "driver/gpio.h"
#include "esp_err.h"
#include "adc_driver.h"
#include "lcd.h"
#include "types.h"
#include "push_button_driver.h"
#include "esp_log.h"

#include "app_context.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/task.h"
#include "driver/ledc.h"

#include "app_logic.h"


static app_screen_t last_screen = APP_LOADING;

void GPIO_Initialation(gpio_num_t left_button, 
                        gpio_num_t center_button,
                        gpio_num_t right_button,
                        gpio_num_t second_led)
{
    gpio_reset_pin(left_button);
    gpio_set_direction(left_button, GPIO_MODE_INPUT);

    gpio_reset_pin(center_button);
    gpio_set_direction(center_button , GPIO_MODE_INPUT);

    gpio_reset_pin(right_button);
    gpio_set_direction(right_button, GPIO_MODE_INPUT);
    
    gpio_reset_pin(second_led);
    gpio_set_direction(second_led, GPIO_MODE_OUTPUT);
    gpio_set_level(second_led, 1);
    
}

void app_task(void *pv)
{
    app_event_t evt;

    while (1)
    {
        if (xQueueReceive(app_queue, &evt, portMAX_DELAY))
        {
            xSemaphoreTake(app_mutex, portMAX_DELAY);

            app_handle_event(&app, evt);

            xSemaphoreGive(app_mutex);
        }
    }
}

void adc_task(void *pv)
{
    adc_unit_handle_custom_t adc1;
    adc_channel_handle_custom_t ch0;

    adc_unit_init(&adc1, ADC_UNIT_1);
    adc_channel_init(&adc1, &ch0, pin_potensio, ADC_ATTEN_DB_12);

    while (1)
    {
        int vp = adc_read_raw(&adc1, &ch0);
        uint32_t duty_local = (vp * 8191) / 4095;

        xSemaphoreTake(app_mutex, portMAX_DELAY);
        app.duty = duty_local;
        xSemaphoreGive(app_mutex);

        ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_2, duty_local);
        ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_2);

        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

void led_task(void *pv)
{
    while (1)
    {
        gpio_set_level(pin_led_2, toggle_state);
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

void lcd_task(void *pv)
{
    bool blink_state = false;
    uint8_t blink_counter = 0;

    while (1)
    {
        blink_counter++;
        if (blink_counter > BLINK_THRESHOLD) {
            blink_counter = 0;
            blink_state = !blink_state;
        }

        app_state_t snapshot;

        xSemaphoreTake(app_mutex, portMAX_DELAY);
        snapshot = app;
        xSemaphoreGive(app_mutex);

        if (!snapshot.system_ready)
        {
            lcd_clear();
            lcd_set_cursor(0, 0);
            lcd_write_string("Loading...");
            vTaskDelay(pdMS_TO_TICKS(100));
            continue;
        }

        if (snapshot.screen != last_screen)
        {
            lcd_clear();
            last_screen = snapshot.screen;
        }

        switch (snapshot.screen)
        {
            case APP_IDLE:
                lcd_set_cursor(0, 0);
                lcd_write_string("TR:");
                lcd_write_int(snapshot.tare);
                lcd_write_string("CAL:");
                lcd_write_int(snapshot.calib);

                lcd_set_cursor(1, 0);
                lcd_write_string("LC :");
                lcd_write_int(snapshot.raw);
                break;

            case APP_MENU:
                lcd_set_cursor(0, 0);
                lcd_write_string("MENU");
                lcd_set_cursor(1, 0);
                lcd_write_string("Press Center");
                break;

            case APP_CALIB_TARE:
                lcd_set_cursor(0, 0);
                lcd_write_string("TARE");
                lcd_set_cursor(1, 0);
                lcd_write_string("Hold=OK");
                break;

            case APP_CALIB_INPUT:
                lcd_set_cursor(0, 0);
                lcd_write_string("CAL:");

                for (int i = 0; i < 4; i++)
                {
                    uint8_t digit = editor_get_digit(&snapshot.editor, i);
                    uint8_t col = EDITOR_COL_START + i;

                    lcd_set_cursor(EDITOR_ROW, col);

                    bool blink = editor_should_blink(&snapshot.editor, i);

                    if (blink && blink_state)
                        lcd_write_char(' ');
                    else
                        lcd_write_char('0' + digit);
                }
                break;

            case APP_CALIB_DONE:
                lcd_set_cursor(0, 0);
                lcd_write_string("DONE");
                lcd_set_cursor(1, 0);
                lcd_write_string("Hold=Exit");
                break;

            default:
                break;
        }

        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

void hx711_task(void *pv)
{
    hx711_t *scale = (hx711_t *)pv;
    bool ready;

    while (1)
    {
        hx711_is_ready(scale, &ready);

        if (ready)
        {
            xSemaphoreTake(app_mutex, portMAX_DELAY);
            hx711_read_data(scale, &app.raw);
            ESP_LOGI("raw", "%d", app.raw);
            xSemaphoreGive(app_mutex);
        }

        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

void button_task(void *pv)
{
    button_group_t *grp = (button_group_t *)pv;

    while (1)
    {
        for (int i = 0; i < grp->count; i++)
        {
            press_type_t event = button_update(
                &grp->buttons[i].state,
                grp->buttons[i].pin,
                &grp->buttons[i].cfg
            );

            if (event != PRESS_NONE && grp->buttons[i].callback != NULL) {
                grp->buttons[i].callback(event);
            }
        }

        vTaskDelay(pdMS_TO_TICKS(10));
    }
    
}

void left_button_handler(press_type_t event) 
{
    if (event != PRESS_SHORT) return;

    app_event_t evt = EVT_LEFT;
    xQueueSend(app_queue, &evt, 0);
}

void right_button_handler(press_type_t event) 
{
    if (event != PRESS_SHORT) return;

    app_event_t evt = EVT_RIGHT;
    xQueueSend(app_queue, &evt, 0);
}

void center_button_handler(press_type_t event) 
{
    app_event_t evt;

    if (event == PRESS_SHORT) {
        evt = EVT_CENTER_SHORT;
    } 
    else if (event == PRESS_VERY_LONG) {
        evt = EVT_CENTER_LONG;
    } 
    else {
        return;
    }

    xQueueSend(app_queue, &evt, 0);
}