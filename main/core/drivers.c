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

static uint8_t last_adv_cursor = 255;

static alarm_ui_state_t
    last_ui_state =
    (alarm_ui_state_t)-1;

static void render_editor_digits(
    editor_t *editor,
    bool blink_state
)
{
    for (int i = 0; i < 6; i++)
    {
        uint8_t digit =
            editor_get_digit(
                editor,
                i
            );

        uint8_t col =
            EDITOR_COL_START + i;

        lcd_set_cursor(1, col);

        bool blink =
            editor_should_blink(
                editor,
                i
            );

        if (
            blink &&
            blink_state
        )
        {
            lcd_write_char(' ');
        }
        else
        {
            lcd_write_char(
                '0' + digit
            );
        }
    }
}


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

void alarm_init(void)
{
    gpio_reset_pin(pin_ch1_relay);
    gpio_set_direction(pin_ch1_relay, GPIO_MODE_OUTPUT);

    gpio_reset_pin(pin_ch2_relay);
    gpio_set_direction(pin_ch2_relay, GPIO_MODE_OUTPUT);

    gpio_reset_pin(pin_ch3_relay);
    gpio_set_direction(pin_ch3_relay, GPIO_MODE_OUTPUT);

    // default OFF
    gpio_set_level(pin_ch1_relay, 0);
    gpio_set_level(pin_ch2_relay, 0);
    gpio_set_level(pin_ch3_relay, 0);
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

void lcd_task(void *pv)
{
    bool blink_state = false;
    uint8_t blink_counter = 0;

    static int32_t last_total = -1;
    static app_screen_t last_screen =
        APP_LOADING;

    while (1)
    {
        // =====================================
        // BLINK TIMER
        // =====================================

        blink_counter++;

        if (
            blink_counter >= 10
        )
        {
            blink_counter = 0;

            blink_state =
                !blink_state;
        }

        // =====================================
        // SNAPSHOT
        // =====================================

        app_state_t snapshot;

        xSemaphoreTake(
            app_mutex,
            portMAX_DELAY
        );

        alarm_update(&app);

        app_update(&app);

        snapshot = app;

        xSemaphoreGive(app_mutex);

        // =====================================
        // LOADING
        // =====================================

        if (!snapshot.system_ready)
        {
            if (
                last_screen !=
                APP_LOADING
            )
            {
                lcd_clear();

                lcd_set_cursor(0, 0);

                lcd_write_string(
                    "Loading..."
                );

                last_screen =
                    APP_LOADING;
            }

            vTaskDelay(
                pdMS_TO_TICKS(100)
            );

            continue;
        }

        // =====================================
        // CLEAR ON SCREEN CHANGE
        // =====================================

        if (
            snapshot.screen !=
            last_screen
        )
        {
            lcd_clear();

            last_screen =
                snapshot.screen;

            last_total = -1;
        }

        // =====================================
        // RENDER
        // =====================================

        // =====================================
        // REFRESH ADVANCED PAGE
        // =====================================

        if
        (
            snapshot.screen ==
            APP_CONFIG_ALARM
        )
        {
            bool adv_changed = false;

            if
            (
                snapshot.adv_cursor !=
                last_adv_cursor
            )
            {
                adv_changed = true;
            }

            if
            (
                snapshot.ui_state !=
                last_ui_state
            )
            {
                adv_changed = true;
            }

            if (adv_changed)
            {
                lcd_clear();

                last_adv_cursor =
                    snapshot.adv_cursor;

                last_ui_state =
                    snapshot.ui_state;
            }
        }

        switch (snapshot.screen)
        {
            // =================================
            // IDLE
            // =================================

            case APP_IDLE:
            {
                int32_t total =
                    get_total_weight(
                        &snapshot
                    );

                if (
                    total !=
                    last_total
                )
                {
                    lcd_set_cursor(0, 0);

                    lcd_write_string(
                        "TOTAL:"
                    );

                    lcd_set_cursor(1, 0);

                    lcd_write_float(
                        total / 1000.0f,
                        2
                    );

                    lcd_write_string(
                        "      "
                    );

                    last_total =
                        total;
                }
            }
            break;

            // =================================
            // MONITOR
            // =================================

            case APP_MONITOR:
            {
                int total =
                    CONFIG_NUM_LOADCELL;

                if (total >= 1)
                {
                    lcd_set_cursor(0, 0);

                    lcd_write_string(
                        "L0:"
                    );

                    lcd_write_float(
                        snapshot.lc[0].weight /
                        1000.0f,
                        2
                    );
                }

                if (total >= 3)
                {
                    lcd_set_cursor(0, 8);

                    lcd_write_string(
                        "L2:"
                    );

                    lcd_write_float(
                        snapshot.lc[2].weight /
                        1000.0f,
                        2
                    );
                }

                if (total >= 2)
                {
                    lcd_set_cursor(1, 0);

                    lcd_write_string(
                        "L1:"
                    );

                    lcd_write_float(
                        snapshot.lc[1].weight /
                        1000.0f,
                        2
                    );
                }

                if (total >= 4)
                {
                    lcd_set_cursor(1, 8);

                    lcd_write_string(
                        "L3:"
                    );

                    lcd_write_float(
                        snapshot.lc[3].weight /
                        1000.0f,
                        2
                    );
                }
            }
            break;

            // =================================
            // MENU
            // =================================

            case APP_MENU:
            {
                lcd_set_cursor(0, 0);

                lcd_write_string(
                    "MENU            "
                );

                lcd_set_cursor(1, 0);

                lcd_write_string(
                    "< > SEL"
                );
            }
            break;

            // =================================
            // CONFIG ALARM
            // =================================
            case APP_CONFIG_ALARM:
            {
                alarm_t *a =
                    &snapshot.alarm[
                        snapshot.current_alarm
                    ];

                // =============================
                // ADVANCED CONFIG
                // =============================

                if
                (
                    snapshot.ui_state ==
                    ALARM_UI_ADVANCED ||

                    snapshot.ui_state ==
                    ALARM_UI_EDIT_TRIGGER_DELAY ||

                    snapshot.ui_state ==
                    ALARM_UI_EDIT_OUTPUT_DELAY
                )
                {
                    // =========================
                    // TRIGGER MODE
                    // =========================

                    if
                    (
                        snapshot.adv_cursor ==
                        ADV_ITEM_TRIGGER_MODE
                    )
                    {
                        lcd_set_cursor(0, 0);

                        lcd_write_string(
                            "TRIGGER MODE  "
                        );

                        lcd_set_cursor(1, 0);

                        if (blink_state)
                        {
                            lcd_write_char('>');
                        }
                        else
                        {
                            lcd_write_char(' ');
                        }

                        lcd_write_string(
                            a->trigger_selector
                            .items[
                                a->trigger_selector
                                .selected
                            ]
                        );
                    }

                    // =========================
                    // OUTPUT MODE
                    // =========================

                    else if
                    (
                        snapshot.adv_cursor ==
                        ADV_ITEM_OUTPUT_MODE
                    )
                    {
                        lcd_set_cursor(0, 0);

                        lcd_write_string(
                            "OUTPUT MODE   "
                        );

                        lcd_set_cursor(1, 0);

                        if (blink_state)
                        {
                            lcd_write_char('>');
                        }
                        else
                        {
                            lcd_write_char(' ');
                        }

                        lcd_write_string(
                            a->output_selector
                            .items[
                                a->output_selector
                                .selected
                            ]
                        );
                    }

                    // =========================
                    // TRIGGER DELAY
                    // =========================

                    else if
                    (
                        snapshot.adv_cursor ==
                        ADV_ITEM_TRIGGER_DELAY
                    )
                    {
                        lcd_set_cursor(0, 0);

                        lcd_write_string(
                            "TRIG DELAY MS "
                        );

                    render_editor_digits(
                        &snapshot.editor,
                        blink_state
                    );

                    }

                    // =========================
                    // OUTPUT DELAY
                    // =========================

                    else if
                    (
                        snapshot.adv_cursor ==
                        ADV_ITEM_OUTPUT_DELAY
                    )
                    {
                        lcd_set_cursor(0, 0);

                        lcd_write_string(
                            "OUT DELAY MS  "
                        );

                        render_editor_digits(
                            &snapshot.editor,
                            blink_state
                        );
                    }
                }

                // =============================
                // NORMAL CONFIG
                // =============================

                else
                {
                    lcd_set_cursor(0, 0);

                    lcd_write_string("A");

                    lcd_write_char(
                        '1' +
                        snapshot.current_alarm
                    );

                    lcd_write_string(":");

                    if (
                        snapshot.ui_state ==
                        ALARM_UI_SELECT
                    )
                    {
                        if (blink_state)
                        {
                            lcd_write_char('>');
                        }
                        else
                        {
                            lcd_write_char(' ');
                        }
                    }

                    switch (a->mode)
                    {
                        case ALARM_ATAS:
                            lcd_write_string(
                                "ATAS "
                            );
                            break;

                        case ALARM_BAWAH:
                            lcd_write_string(
                                "BAWAH"
                            );
                            break;

                        case ALARM_DALAM:
                            lcd_write_string(
                                "DALAM"
                            );
                            break;

                        case ALARM_LUAR:
                            lcd_write_string(
                                "LUAR "
                            );
                            break;

                        default:
                            lcd_write_string(
                                "-----"
                            );
                            break;
                    }

                    render_editor_digits(
                        &snapshot.editor,
                        blink_state
                    );
                }
            }
            break;

            // =================================
            // CALIB TARE
            // =================================

            case APP_CALIB_TARE:
            {
                lcd_set_cursor(0, 0);

                lcd_write_string(
                    "TARE"
                );

                lcd_set_cursor(1, 0);

                lcd_write_string(
                    "Hold=OK"
                );
            }
            break;

            // =================================
            // CALIB TARE WAIT
            // =================================

            case APP_CALIB_TARE_WAIT:
            {
                lcd_set_cursor(0, 0);

                lcd_write_string(
                    "TARE..."
                );

                lcd_set_cursor(1, 0);

                lcd_write_string(
                    "Wait"
                );
            }
            break;

            // =================================
            // CALIB INPUT
            // =================================

            case APP_CALIB_INPUT:
            {
                lcd_set_cursor(0, 0);

                lcd_write_string(
                    "CAL:"
                );

                for (
                    int i = 0;
                    i < 6;
                    i++
                )
                {
                    uint8_t digit =
                        editor_get_digit(
                            &snapshot.editor,
                            i
                        );

                    uint8_t col =
                        EDITOR_COL_START +
                        i;

                    lcd_set_cursor(
                        1,
                        col
                    );

                    bool blink =
                        editor_should_blink(
                            &snapshot.editor,
                            i
                        );

                    if (
                        blink &&
                        blink_state
                    )
                    {
                        lcd_write_char(' ');
                    }
                    else
                    {
                        lcd_write_char(
                            '0' + digit
                        );
                    }
                }
            }
            break;

            // =================================
            // CALIB WAIT
            // =================================

            case APP_CALIB_INPUT_WAIT:
            {
                lcd_set_cursor(0, 0);

                lcd_write_string(
                    "CAL..."
                );

                lcd_set_cursor(1, 0);

                lcd_write_string(
                    "Sampling"
                );
            }
            break;

            // =================================
            // DONE
            // =================================

            case APP_CALIB_DONE:
            {
                lcd_set_cursor(0, 0);

                lcd_write_string(
                    "DONE"
                );

                lcd_set_cursor(1, 0);

                lcd_write_string(
                    "Hold=Exit"
                );
            }
            break;

            default:
                break;
        }

        vTaskDelay(
            pdMS_TO_TICKS(50)
        );
    }
}

void hx711_task(void *pv)
{
    hx711_ctx_t *ctx = (hx711_ctx_t *)pv;

    bool ready;
    int32_t val;
    esp_err_t err;

    while (1)
    {
        int dout_level = gpio_get_level(ctx->scale->dout);

        err = hx711_is_ready(ctx->scale, &ready);

        if (err != ESP_OK)
        {
            // ESP_LOGE("HX", "IDX %d is_ready ERR %d", ctx->index, err);
            vTaskDelay(pdMS_TO_TICKS(50));
            continue;
        }

        if (!ready)
        {
            // ESP_LOGW("HX", "IDX %d NOT READY (DOUT=%d)", ctx->index, dout_level);
            vTaskDelay(pdMS_TO_TICKS(50));
            continue;
        }

        err = hx711_read_data(ctx->scale, &val);

        if (err != ESP_OK)
        {
            // ESP_LOGE("HX", "IDX %d READ FAIL err=%d (DOUT=%d)", ctx->index, err, dout_level);
        }
        else
        {
            ESP_LOGI("HX", "IDX %d OK RAW=%ld (DOUT=%d)", ctx->index, val, dout_level);

            xSemaphoreTake(app_mutex, portMAX_DELAY);
            app.lc[ctx->index].raw = val;
            xSemaphoreGive(app_mutex);
        }

        vTaskDelay(pdMS_TO_TICKS(50));
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