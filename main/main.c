#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_err.h"
#include "sdkconfig.h"
#include "esp_pm.h"
#include "esp_timer.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "nvs.h"

#include "adc_driver.h"

#include "led_driver.h"

#include "i2c_lcd_driver.h"

#include "hx711_driver.h"

#include "push_button_driver.h"

#include "editor.h"

#define pin_potensio ADC_CHANNEL_0 //VP
#define pin_sck_hx711 GPIO_NUM_5
#define pin_dt_hx711 GPIO_NUM_4

#define pin_led_1 GPIO_NUM_2
#define pin_led_2 GPIO_NUM_19

#define pin_button_left GPIO_NUM_32
#define pin_button_center GPIO_NUM_35
#define pin_button_right GPIO_NUM_34

#define interval_adc_potensio 100000
#define interval_lcd 5000000

#define BLINK_THRESHOLD 3
#define EDITOR_CHAR_MIN 0
#define EDITOR_CHAR_MAX 9
#define EDITOR_CHAR_RANGE (EDITOR_CHAR_MAX - EDITOR_CHAR_MIN + 1)

typedef struct {
    uint32_t duty;
    int32_t raw;

    editor_t editor;
} app_state_t;


// typedef struct {
//     uint32_t duty;
//     int32_t raw;
//     uint16_t editor_value;

//     uint8_t cursor_col;   // ganti lcd_cursor.col
//     bool edit_mode;       // ganti lcd_cursor.active
// } app_state_t;

typedef enum {
    EVT_LEFT,
    EVT_RIGHT,
    EVT_CENTER_SHORT,
    EVT_CENTER_LONG
} app_event_t;

static app_state_t app = {0};

SemaphoreHandle_t app_mutex;
QueueHandle_t app_queue;

volatile bool toggle_state = 0;

void save_editor_value(int val) 
{
    nvs_handle_t handle;
    ESP_ERROR_CHECK(nvs_open("storage", NVS_READWRITE, &handle));
    ESP_ERROR_CHECK(nvs_set_i32(handle, "editor_val", val));
    ESP_ERROR_CHECK(nvs_commit(handle));
    nvs_close(handle);
}

int load_editor_value() 
{
    nvs_handle_t handle;
    int32_t val = 0;
    if (nvs_open("storage", NVS_READONLY, &handle) == ESP_OK) {
        nvs_get_i32(handle, "editor_val", &val);
        nvs_close(handle);
    }
    return val;
}

void GPIO_Initialation()
{
    gpio_reset_pin(pin_button_left);
    gpio_set_direction(pin_button_left, GPIO_MODE_INPUT);

    gpio_reset_pin(pin_button_center);
    gpio_set_direction(pin_button_center, GPIO_MODE_INPUT);

    gpio_reset_pin(pin_button_right);
    gpio_set_direction(pin_button_right, GPIO_MODE_INPUT);
    
    gpio_reset_pin(pin_led_2);
    gpio_set_direction(pin_led_2, GPIO_MODE_OUTPUT);
    gpio_set_level(pin_led_2, 1);
    
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

void app_task(void *pv)
{
    app_event_t evt;

    while (1)
    {
        if (xQueueReceive(app_queue, &evt, portMAX_DELAY))
        {
            xSemaphoreTake(app_mutex, portMAX_DELAY);

            switch (evt)
            {
                case EVT_LEFT:
                    editor_move_left(&app.editor);
                    break;

                case EVT_RIGHT:
                    editor_move_right(&app.editor);
                    break;

                case EVT_CENTER_SHORT:
                    editor_toggle_mode(&app.editor);
                    break;

                case EVT_CENTER_LONG:
                {
                    uint16_t val = editor_get_value(&app.editor);

                    xSemaphoreGive(app_mutex);
                    save_editor_value(val);
                    continue;
                }
            }
            xSemaphoreGive(app_mutex);
        }
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
            xSemaphoreGive(app_mutex);
        }

        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

void lcd_task(void *pv)
{
    bool blink_state = false;
    uint8_t blink_counter = 0;
    int temp;

    while (1)
    {
        blink_counter++;
        if (blink_counter > BLINK_THRESHOLD) 
        {
            blink_counter = 0;
            blink_state = !blink_state;
        }

        app_state_t snapshot;

        xSemaphoreTake(app_mutex, portMAX_DELAY);
        snapshot = app;
        xSemaphoreGive(app_mutex);

        lcd_put_cur(0, 0);
        lcd_send_string("POT :");
        lcd_send_int(snapshot.duty);

        lcd_put_cur(1, 0);
        lcd_send_string("LC  :");
        lcd_send_int(snapshot.raw);

        temp = snapshot.editor.value;
        for (int i = 3; i >= 0; i--) 
        {
            int digit = temp % 10;
            temp /= 10;

            lcd_put_cur(1, 12 + i);

            if (snapshot.editor.edit_mode && snapshot.editor.cursor_col == 12 + i) {
                if (blink_state) 
                {
                    lcd_send_data(' ');
                } 
                else 
                {
                    lcd_send_data('0' + digit);
                }
            } 
            else 
            {
                lcd_send_data('0' + digit);
            }
        }

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

void app_main(void)
{
    app_mutex = xSemaphoreCreateMutex();
    if (app_mutex == NULL) 
    {
        ESP_LOGE("APP", "Failed to create mutex");
        abort();
    }

    app_queue = xQueueCreate(10, sizeof(app_event_t));
    if (app_queue == NULL) {
        ESP_LOGE("APP", "Failed to create queue");
        abort();
    }
    
    esp_err_t ret = nvs_flash_init();

    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || 
        ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }

    ESP_ERROR_CHECK(ret);

    editor_init(&app.editor, load_editor_value());

    GPIO_Initialation();

    hx711_t scale = {
        .dout = pin_dt_hx711,
        .pd_sck = pin_sck_hx711,
        .gain = HX711_GAIN_A_128
    };

    ESP_ERROR_CHECK(hx711_init(&scale));

    lcd_init();

    pwm_timer_init(LEDC_LOW_SPEED_MODE,
                   LEDC_TIMER_0,
                   LEDC_TIMER_13_BIT,
                   4000,
                   LEDC_AUTO_CLK);

    pwm_channel_init(LEDC_LOW_SPEED_MODE, 
                    LEDC_CHANNEL_2, 
                    LEDC_TIMER_0, 
                    pin_led_1, 
                    4000);
                    

    button_config_t cfg = {
        .debounce_time = 10000,
        .short_press_time = 50000,
        .long_press_time = 1000000,
        .very_long_press_time = 5000000
    };

    button_ctx_t buttons[] = {
    {pin_button_left, {.state = BTN_IDLE}, cfg, left_button_handler},
    {pin_button_center, {.state = BTN_IDLE}, cfg, center_button_handler},
    {pin_button_right, {.state = BTN_IDLE}, cfg, right_button_handler}
    };

    uint8_t button_count = sizeof(buttons) / sizeof(button_ctx_t);

    button_group_t btn_group = {
    .buttons = buttons,
    .count = button_count
    };
    
    xTaskCreate(app_task, "app", 4096, NULL, 6, NULL);
    xTaskCreate(button_task, "btn", 2048, &btn_group, 5, NULL);
    xTaskCreate(adc_task, "adc", 2048, NULL, 5, NULL);
    xTaskCreate(hx711_task, "hx", 2048, &scale, 5, NULL);
    xTaskCreate(lcd_task, "lcd", 4096, NULL, 3, NULL);
    xTaskCreate(led_task, "led", 1024, NULL, 2, NULL);
    
    vTaskDelete(NULL);
}