/* ===================== RTOS ===================== */
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"

/* ===================== ESP ===================== */
#include "esp_err.h"
#include "sdkconfig.h"
#include "esp_pm.h"
#include "esp_timer.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "nvs.h"

/* ===================== COMMON ===================== */
#include "common/types.h"

/* ===================== CONFIG ===================== */
#include "config/config.h"

/* ===================== SYSTEM ===================== */
#include "system/app_context.h"
#include "utils.h"
#include "selector.h"

/* ===================== CORE ===================== */
#include "core/drivers.h"

/* ===================== APP ===================== */
#include "app/app_logic.h"
#include "app/editor.h"

/* ===================== DRIVERS ===================== */
#include "drivers/adc/adc_driver.h"
#include "drivers/led/led_driver.h"
#include "drivers/lcd/i2c_lcd_driver.h"
#include "drivers/lcd/lcd.h"
#include "drivers/hx711/hx711_driver.h"
#include "drivers/button/push_button_driver.h"
#include "driver/gpio.h"

/* ===================== GLOBAL ===================== */
static hx711_t scale[CONFIG_NUM_LOADCELL];
static hx711_ctx_t ctx[CONFIG_NUM_LOADCELL];

static const char *alarm_mode_items[] =
{
    "ATAS",
    "BAWAH",
    "DALAM",
    "LUAR"
};

const char *trigger_mode_items[] =
{
    "LEVEL ",
    "STABLE"
};

const char *output_mode_items[] =
{
    "DIRECT ",
    "ON DEL ",
    "OFF DEL"
};

/* ===================== MAIN ===================== */
void app_main(void)
{
    esp_err_t ret = nvs_flash_init();

    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || 
        ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    for (int i = 0; i < ALARM_COUNT; i++)
    {
        app.alarm[i].enabled = true;

        app.alarm[i].selector.items =
            alarm_mode_items;

        app.alarm[i].selector.count = 4;
        app.alarm[i].trigger_selector.items = trigger_mode_items;

        app.alarm[i].trigger_selector.count = 2;

        app.alarm[i].trigger_selector.selected = 0;

        app.alarm[i].trigger_mode =
            TRIGGER_LEVEL;

        app.alarm[i].output_selector.items = output_mode_items;

        app.alarm[i].output_selector.count = 3;

        app.alarm[i].output_selector.selected = 0;

        app.alarm[i].output_mode =
            OUTPUT_DIRECT;

        char key[16];

        // =========================
        // LOAD MODE
        // =========================

        make_nvs_key(
            key,
            sizeof(key),
            "alm_mode",
            i
        );

        app.alarm[i].selector.selected =
            nvs_load_i32(key, 0);

        app.alarm[i].mode =
            (alarm_mode_t)
            app.alarm[i].selector.selected;

        // =========================
        // LOAD LOW
        // =========================

        make_nvs_key(
            key,
            sizeof(key),
            "alm_low",
            i
        );

        app.alarm[i].threshold_low =
            nvs_load_i32(key, 1000);

        // =========================
        // LOAD HIGH
        // =========================

        make_nvs_key(
            key,
            sizeof(key),
            "alm_high",
            i
        );

        app.alarm[i].threshold_high =
            nvs_load_i32(key, 5000);

        
        // =========================
        // LOAD TRIGGER MODE
        // =========================

        make_nvs_key(
            key,
            sizeof(key),
            "trig_mode",
            i
        );

        app.alarm[i].trigger_selector.selected =
            nvs_load_i32(key, 0);

        app.alarm[i].trigger_mode =
            (trigger_mode_t)
            app.alarm[i].trigger_selector.selected;

        // =========================
        // LOAD OUTPUT MODE
        // =========================

        make_nvs_key(
            key,
            sizeof(key),
            "out_mode",
            i
        );

        app.alarm[i].output_selector.selected =
            nvs_load_i32(key, 0);

        app.alarm[i].output_mode =
            (output_mode_t)
            app.alarm[i].output_selector.selected;

        // =========================
        // LOAD TRIGGER DELAY
        // =========================

        make_nvs_key(
            key,
            sizeof(key),
            "trig_dly",
            i
        );

        app.alarm[i].trigger_delay_ms =
            nvs_load_i32(key, 0);

        // =========================
        // LOAD OUTPUT DELAY
        // =========================

        make_nvs_key(
            key,
            sizeof(key),
            "out_dly",
            i
        );

        app.alarm[i].output_delay_ms =
            nvs_load_i32(key, 0);
            }

    

    /* ===================== RTOS INIT ===================== */
    app_mutex = xSemaphoreCreateMutex();
    app_queue = xQueueCreate(10, sizeof(app_event_t));
    sys_mutex = xSemaphoreCreateMutex();

    if (!app_mutex || !app_queue || !sys_mutex)
    {
        ESP_LOGE("APP", "Init failed");
        abort();
    }

    /* ===================== GPIO ===================== */
    GPIO_Initialation(
        pin_button_left,
        pin_button_center,
        pin_button_right
    );

    alarm_init();

    /* ===================== HX711 INIT ===================== */
    gpio_num_t dt_pins[] = {
    #if CONFIG_NUM_LOADCELL >= 1
        pin_dt_hx711,
    #endif
    #if CONFIG_NUM_LOADCELL >= 2
        pin_dt_hx711_2,
    #endif
    #if CONFIG_NUM_LOADCELL >= 3
        pin_dt_hx711_3,
    #endif
    #if CONFIG_NUM_LOADCELL >= 4
        pin_dt_hx711_4,
    #endif
    };

    gpio_num_t sck_pins[] = {
    #if CONFIG_NUM_LOADCELL >= 1
        pin_sck_hx711,
    #endif
    #if CONFIG_NUM_LOADCELL >= 2
        pin_sck_hx711_2,
    #endif
    #if CONFIG_NUM_LOADCELL >= 3
        pin_sck_hx711_3,
    #endif
    #if CONFIG_NUM_LOADCELL >= 4
        pin_sck_hx711_4,
    #endif
    };

    for (int i = 0; i < CONFIG_NUM_LOADCELL; i++)
    {
        scale[i].dout = dt_pins[i];
        scale[i].pd_sck = sck_pins[i];
        scale[i].gain = HX711_GAIN_A_128;

        ESP_ERROR_CHECK(hx711_init(&scale[i]));

        ctx[i].scale = &scale[i];
        ctx[i].index = i;
    }

    /* ===================== LCD ===================== */
    lcd_init(pin_sda_lcd, pin_scl_lcd);

    /* ===================== APP INIT ===================== */
    editor_init(&app.editor, nvs_load_i32("editor", 0));

    app.screen = APP_IDLE;
    app.system_ready = true;
    app.lc_index = 0;

    char key [36];

    for (int i = 0; i < CONFIG_NUM_LOADCELL; i++)
    {
        make_nvs_key(key, sizeof(key), "tare", i);
        app.lc[i].tare = nvs_load_i32(key, 0);

        make_nvs_key(key, sizeof(key), "calib", i);
        app.lc[i].calib = nvs_load_i32(key, 1000);

        app.lc[i].raw = 0;
        app.lc[i].weight = 0;
    }

    app.current_lc = 0;

    /* ===================== BUTTON ===================== */
    button_config_t cfg = {
        .debounce_time = 10000,
        .short_press_time = 50000,
        .long_press_time = 1000000,
        .very_long_press_time = 5000000
    };

    button_ctx_t buttons[] = {
        {pin_button_left,   {.state = BTN_IDLE}, cfg, left_button_handler},
        {pin_button_center, {.state = BTN_IDLE}, cfg, center_button_handler},
        {pin_button_right,  {.state = BTN_IDLE}, cfg, right_button_handler}
    };

    uint8_t button_count = sizeof(buttons) / sizeof(button_ctx_t);

    button_group_t btn_group = {
    .buttons = buttons,
    .count = button_count
    };

    /* ===================== TASK ===================== */
    xTaskCreate(app_task, "app", 4096, NULL, 6, NULL);
    xTaskCreate(button_task, "btn", 2048, &btn_group, 5, NULL);

    for (int i = 0; i < CONFIG_NUM_LOADCELL; i++)
    {
        char name[8];
        snprintf(name, sizeof(name), "hx%d", i);

        xTaskCreate(hx711_task, name, 2048, &ctx[i], 6, NULL);
    }

    xTaskCreate(lcd_task, "lcd", 4096, NULL, 3, NULL);

    vTaskDelete(NULL);
}