#include <inttypes.h>
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
                        gpio_num_t right_button)
{
    gpio_reset_pin(left_button);
    gpio_set_direction(left_button, GPIO_MODE_INPUT);

    gpio_reset_pin(center_button);
    gpio_set_direction(center_button , GPIO_MODE_INPUT);

    gpio_reset_pin(right_button);
    gpio_set_direction(right_button, GPIO_MODE_INPUT);
    
    // gpio_reset_pin(second_led);
    // gpio_set_direction(second_led, GPIO_MODE_OUTPUT);
    // gpio_set_level(second_led, 1);
    
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

// void adc_task(void *pv)
// {
//     adc_unit_handle_custom_t adc1;
//     adc_channel_handle_custom_t ch0;

//     adc_unit_init(&adc1, ADC_UNIT_1);
//     adc_channel_init(&adc1, &ch0, pin_potensio, ADC_ATTEN_DB_12);

//     while (1)
//     {
//         int vp = adc_read_raw(&adc1, &ch0);
//         uint32_t duty_local = (vp * 8191) / 4095;

//         xSemaphoreTake(app_mutex, portMAX_DELAY);
//         app.duty = duty_local;
//         xSemaphoreGive(app_mutex);

//         ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_2, duty_local);
//         ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_2);

//         vTaskDelay(pdMS_TO_TICKS(100));
//     }
// }

// void led_task(void *pv)
// {
//     while (1)
//     {
//         gpio_set_level(pin_led_2, toggle_state);
//         vTaskDelay(pdMS_TO_TICKS(10));
//     }
// }

void lcd_task(void *pv)
{
    bool blink_state = false;
    uint8_t blink_counter = 0;

    static int32_t last_total = -1;
    static app_screen_t last_screen = APP_LOADING;

    while (1)
    {
        blink_counter++;
        if (blink_counter > BLINK_THRESHOLD) {
            blink_counter = 0;
            blink_state = !blink_state;
        }

        app_state_t snapshot;

        xSemaphoreTake(app_mutex, portMAX_DELAY);
        app_update(&app);
        snapshot = app;
        xSemaphoreGive(app_mutex);

        // =========================
        // LOADING SCREEN
        // =========================
        if (!snapshot.system_ready)
        {
            if (last_screen != APP_LOADING)
            {
                lcd_clear();
                lcd_set_cursor(0, 0);
                lcd_write_string("Loading...");
                last_screen = APP_LOADING;
            }

            vTaskDelay(pdMS_TO_TICKS(100));
            continue;
        }

        // =========================
        // CLEAR HANYA SAAT GANTI SCREEN
        // =========================
        if (snapshot.screen != last_screen)
        {
            lcd_clear();
            last_screen = snapshot.screen;
            last_total = -1; // force refresh
        }

        // =========================
        // RENDER
        // =========================
        switch (snapshot.screen)
        {
            case APP_IDLE:
            {
                int32_t total = get_total_weight_raw(&snapshot);

                // update hanya jika berubah
                if (total != last_total)
                {
                    lcd_set_cursor(0, 0);
                    lcd_write_string("TOTAL:      ");

                    lcd_set_cursor(1, 0);
                    lcd_write_float(total / 1000.0f, 2);
                    lcd_write_string("      "); // padding

                    last_total = total;
                }
            }
            break;

            case APP_MONITOR:
            {
                int total = CONFIG_NUM_LOADCELL;

                // render statis label saja (tidak tiap loop)
                static bool initialized = false;

                if (!initialized)
                {
                    lcd_clear();

                    if (total == 1)
                    {
                        lcd_set_cursor(0, 0);
                        lcd_write_string("RAW:");
                        lcd_set_cursor(1, 0);
                        lcd_write_string("W:");
                    }
                    else if (total == 2)
                    {
                        lcd_set_cursor(0, 0);
                        lcd_write_string("L0:");
                        lcd_set_cursor(1, 0);
                        lcd_write_string("L1:");
                    }
                    else if (total == 3)
                    {
                        lcd_set_cursor(0, 0);
                        lcd_write_string("L0:");
                        lcd_set_cursor(0, 8);
                        lcd_write_string("L2:");
                        lcd_set_cursor(1, 0);
                        lcd_write_string("L1:");
                    }
                    else
                    {
                        lcd_set_cursor(0, 0);
                        lcd_write_string("L0:");
                        lcd_set_cursor(0, 8);
                        lcd_write_string("L2:");
                        lcd_set_cursor(1, 0);
                        lcd_write_string("L1:");
                        lcd_set_cursor(1, 8);
                        lcd_write_string("L3:");
                    }

                    initialized = true;
                }

                // update nilai saja
                if (total >= 1)
                {
                    lcd_set_cursor(0, 4);
                    lcd_write_int((int)snapshot.lc[0].raw);
                }

                if (total >= 1)
                {
                    lcd_set_cursor(1, 2);
                    lcd_write_float(snapshot.lc[0].weight / 1000.0f, 2);
                }

                if (total >= 2)
                {
                    lcd_set_cursor(1, 4);
                    lcd_write_float(snapshot.lc[1].weight / 1000.0f, 2);
                }

                if (total >= 3)
                {
                    lcd_set_cursor(0, 12);
                    lcd_write_float(snapshot.lc[2].weight / 1000.0f, 2);
                }

                if (total >= 4)
                {
                    lcd_set_cursor(1, 12);
                    lcd_write_float(snapshot.lc[3].weight / 1000.0f, 2);
                }
            }
            break;

            case APP_MENU:
                lcd_set_cursor(0, 0);
                lcd_write_string("MENU        ");

                lcd_set_cursor(1, 0);
                lcd_write_string("Press Center");
                break;

            case APP_CALIB_TARE:
                lcd_set_cursor(0, 0);
                lcd_write_string("TARE        ");

                lcd_set_cursor(1, 0);
                lcd_write_string("Hold=OK     ");
                break;

            case APP_CALIB_TARE_WAIT:
                lcd_set_cursor(0, 0);
                lcd_write_string("TARE...     ");

                lcd_set_cursor(1, 0);
                lcd_write_string("Wait        ");
                if (blink_state) lcd_write_string("...");
                break;

            case APP_CALIB_INPUT:
                lcd_set_cursor(0, 0);
                lcd_write_string("CAL:        ");

                for (int i = 0; i < 6; i++)
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

            case APP_CALIB_INPUT_WAIT:
                lcd_set_cursor(0, 0);
                lcd_write_string("CAL...      ");

                lcd_set_cursor(1, 0);
                lcd_write_string("Sampling    ");
                if (blink_state) lcd_write_string("...");
                break;

            case APP_CALIB_DONE:
                lcd_set_cursor(0, 0);
                lcd_write_string("DONE        ");

                lcd_set_cursor(1, 0);
                lcd_write_string("Hold=Exit   ");
                break;

            default:
                break;
        }

        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

void hx711_task(void *pv)
{
    hx711_ctx_t *ctx = (hx711_ctx_t *)pv;
    bool ready;

    while (1)
    {
        hx711_is_ready(ctx->scale, &ready);

        if (ready)
        {
            int32_t val;
            hx711_read_data(ctx->scale, &val);

            xSemaphoreTake(app_mutex, portMAX_DELAY);
            app.lc[ctx->index].raw = val;
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
    app_event_t evt;

    if (event == PRESS_SHORT) {
        evt = EVT_LEFT_SHORT;
    } 
    else if (event == PRESS_LONG) {
        evt = EVT_LEFT_LONG;
    }
    else if (event == PRESS_VERY_LONG) {
        evt = EVT_LEFT_VERY_LONG;
    } 
    else {
        return;
    }

    xQueueSend(app_queue, &evt, 0);
}

void right_button_handler(press_type_t event) 
{
    app_event_t evt;

    if (event == PRESS_SHORT) {
        evt = EVT_RIGHT_SHORT;
    } 
    else if (event == PRESS_LONG) {
        evt = EVT_RIGHT_LONG;
    }
    else if (event == PRESS_VERY_LONG) {
        evt = EVT_RIGHT_VERY_LONG;
    } 
    else {
        return;
    }

    xQueueSend(app_queue, &evt, 0);
}

void center_button_handler(press_type_t event) 
{
    app_event_t evt;

    if (event == PRESS_SHORT) {
        evt = EVT_CENTER_SHORT;
    } 
    else if (event == PRESS_LONG) {
        evt = EVT_CENTER_LONG;
    }
    else if (event == PRESS_VERY_LONG) {
        evt = EVT_CENTER_VERY_LONG;
    } 
    else {
        return;
    }

    xQueueSend(app_queue, &evt, 0);
}