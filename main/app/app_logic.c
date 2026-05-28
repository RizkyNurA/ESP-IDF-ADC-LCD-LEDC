#include "app_logic.h"
#include "app_context.h"
#include "nvs_flash.h"
#include "nvs.h"
#include "config.h"
#include "esp_log.h"
#include "utils.h"
#include "selector.h"
#include "types.h"

#define KNOWN_MAX 999999
#define ALARM_COUNT 3
#define WAIT_TICKS  50

static int32_t get_average_single_sensor(int index, size_t samples);

static void save_alarm_config(
    app_state_t *app,
    int idx
)
{
    char key[32];

    make_nvs_key(key, sizeof(key), "alm_en", idx);
    nvs_save_i32(key, app->alarm[idx].enabled);

    make_nvs_key(key, sizeof(key), "alm_mode", idx);
    nvs_save_i32(key, app->alarm[idx].mode);

    make_nvs_key(key, sizeof(key), "alm_low", idx);
    nvs_save_i32(key, app->alarm[idx].threshold_low);

    make_nvs_key(key, sizeof(key), "alm_high", idx);
    nvs_save_i32(key, app->alarm[idx].threshold_high);
}

static void load_alarm_editor(
    app_state_t *app,
    int idx
)
{
    app->current_alarm = idx;

    editor_set_value(
        &app->editor,
        app->alarm[idx].threshold_low
    );

    app->editor.state = UI_NAV;
}

static bool alarm_condition(
    alarm_t *a,
    int32_t value
)
{
    if (!a->enabled)
        return false;

    switch (a->mode)
    {
        case ALARM_ATAS:
            return value >= a->threshold_low;

        case ALARM_BAWAH:
            return value <= a->threshold_low;

        case ALARM_DALAM:
            return
                value >= a->threshold_low &&
                value <= a->threshold_high;

        case ALARM_LUAR:
            return
                value < a->threshold_low ||
                value > a->threshold_high;

        default:
            return false;
    }
}

static void handle_alarm_config(
    app_state_t *app,
    app_event_t evt
)
{
    alarm_t *a =
        &app->alarm[
            app->current_alarm
        ];

    // =========================
    // MODE SELECT
    // =========================

    if (
        app->ui_state ==
        ALARM_UI_SELECT
    )
    {
        selector_handle_event(
            &a->selector,
            evt
        );

        a->mode =
            (alarm_mode_t)
            a->selector.selected;

        // =========================
        // ENTER EDIT LOW
        // =========================

        if (evt == EVT_RIGHT_LONG)
        {
            app->ui_state =
                ALARM_UI_EDIT_VALUE1;

            editor_set_value(
                &app->editor,
                a->threshold_low
            );

            app->editor.state =
                UI_NAV;
        }
    }

    // =========================
    // EDIT LOW
    // =========================

    else if (
        app->ui_state ==
        ALARM_UI_EDIT_VALUE1
    )
    {
        editor_handle_event(
            &app->editor,
            evt
        );

        // realtime update
        a->threshold_low =
            editor_get_value(
                &app->editor
            );

        // =========================
        // BACK TO SELECT
        // =========================

        if (evt == EVT_LEFT_LONG)
        {
            app->ui_state =
                ALARM_UI_SELECT;

            app->editor.state =
                UI_NAV;
        }

        // =========================
        // ENTER EDIT HIGH
        // ONLY DALAM / LUAR
        // =========================

        else if (
            evt == EVT_RIGHT_LONG &&
            (
                a->mode == ALARM_DALAM ||
                a->mode == ALARM_LUAR
            )
        )
        {
            app->ui_state =
                ALARM_UI_EDIT_VALUE2;

            editor_set_value(
                &app->editor,
                a->threshold_high
            );

            app->editor.state =
                UI_NAV;
        }
    }

    // =========================
    // EDIT HIGH
    // =========================

    else if (
        app->ui_state ==
        ALARM_UI_EDIT_VALUE2
    )
    {
        editor_handle_event(
            &app->editor,
            evt
        );

        // realtime update
        a->threshold_high =
            editor_get_value(
                &app->editor
            );

        // =========================
        // BACK TO LOW
        // =========================

        if (evt == EVT_LEFT_LONG)
        {
            app->ui_state =
                ALARM_UI_EDIT_VALUE1;

            editor_set_value(
                &app->editor,
                a->threshold_low
            );

            app->editor.state =
                UI_NAV;
        }
    }

    // =========================
    // SAVE + NEXT
    // =========================

    if (evt == EVT_CENTER_LONG)
    {
        // safety normalize
        if (
            a->threshold_low >
            a->threshold_high
        )
        {
            int32_t tmp =
                a->threshold_low;

            a->threshold_low =
                a->threshold_high;

            a->threshold_high =
                tmp;
        }

        save_alarm_config(
            app,
            app->current_alarm
        );

        // =========================
        // NEXT ALARM
        // =========================

        if (
            app->current_alarm <
            (ALARM_COUNT - 1)
        )
        {
            app->current_alarm++;

            app->ui_state =
                ALARM_UI_SELECT;

            alarm_t *next =
                &app->alarm[
                    app->current_alarm
                ];

            editor_set_value(
                &app->editor,
                next->threshold_low
            );

            app->editor.state =
                UI_NAV;
        }

        // =========================
        // FINISH
        // =========================

        else
        {
            app->ui_state =
                ALARM_UI_SELECT;

            app->screen =
                APP_IDLE;
        }
    }
}

void app_update(app_state_t *app)
{
    int32_t known =
        editor_get_value(&app->editor);

    if (known < 0)
        known = 0;

    if (known > KNOWN_MAX)
        known = KNOWN_MAX;

    // =========================
    // UPDATE WEIGHT
    // =========================

    for (int i = 0; i < CONFIG_NUM_LOADCELL; i++)
    {
        app->lc[i].weight =
            calculate_weight(
                app->lc[i].raw,
                app->lc[i].tare,
                app->lc[i].calib,
                known
            );
    }

    // =========================
    // WAIT STATE
    // =========================

    switch (app->screen)
    {
        case APP_CALIB_TARE_WAIT:
        {
            app->wait_counter++;

            if (app->wait_counter < WAIT_TICKS)
                break;

            char key[16];

            for (int i = 0; i < CONFIG_NUM_LOADCELL; i++)
            {
                int32_t tare =
                    get_average_single_sensor(
                        i,
                        SAMPLE_CALIB_VALUE
                    );

                app->lc[i].tare = tare;

                make_nvs_key(
                    key,
                    sizeof(key),
                    "tare",
                    i
                );

                nvs_save_i32(key, tare);
            }

            app->screen = APP_CALIB_INPUT;
        }
        break;

        case APP_CALIB_INPUT_WAIT:
        {
            app->wait_counter++;

            if (app->wait_counter < WAIT_TICKS)
                break;

            char key[16];

            int32_t calib_value =
                editor_get_value(&app->editor);

            nvs_save_i32(
                "editor",
                calib_value
            );

            for (int i = 0; i < CONFIG_NUM_LOADCELL; i++)
            {
                int32_t calib =
                    get_average_single_sensor(
                        i,
                        SAMPLE_CALIB_VALUE
                    );

                app->lc[i].calib = calib;

                make_nvs_key(
                    key,
                    sizeof(key),
                    "calib",
                    i
                );

                nvs_save_i32(key, calib);
            }

            app->screen = APP_CALIB_DONE;
        }
        break;

        default:
            break;
    }
}

void app_handle_event(
    app_state_t *app,
    app_event_t evt
)
{
    ESP_LOGI("BTN", "event=%d", evt);

    switch (app->screen)
    {
        case APP_LOADING:
            break;

        case APP_IDLE:

            if (evt == EVT_CENTER_SHORT)
            {
                app->screen = APP_MENU;
            }

            break;

        case APP_MENU:

            if (evt == EVT_LEFT_SHORT)
            {
                app->screen = APP_MONITOR;
            }
            else if (evt == EVT_CENTER_SHORT)
            {
                app->screen = APP_CALIB_TARE;
            }
            else if (evt == EVT_RIGHT_SHORT)
            {

                app->current_alarm = 0;

                load_alarm_editor(app, 0);

                app->screen = APP_CONFIG_ALARM;

                

            }
            else if (evt == EVT_CENTER_LONG)
            {
                app->screen = APP_IDLE;
            }

            break;

        case APP_CONFIG_ALARM:

            handle_alarm_config(
                app,
                evt
            );

            break;

        case APP_CALIB_TARE:

            if (
                evt == EVT_LEFT_SHORT ||
                evt == EVT_CENTER_LONG
            )
            {
                app->wait_counter = 0;

                app->screen =
                    APP_CALIB_TARE_WAIT;
            }

            break;

        case APP_CALIB_INPUT:

            editor_handle_event(
                &app->editor,
                evt
            );

            if (evt == EVT_CENTER_LONG)
            {
                app->wait_counter = 0;

                app->screen =
                    APP_CALIB_INPUT_WAIT;
            }

            break;

        case APP_CALIB_DONE:

            if (evt == EVT_CENTER_LONG)
            {
                app->screen = APP_IDLE;
            }

            break;

        default:
            break;
    }
}

void editor_handle_event(editor_t *e, app_event_t evt)
{
    switch (e->state)
    {
        case UI_NAV:
            if (evt == EVT_LEFT_SHORT && e->cursor_index > 0) {
                e->cursor_index--;
            }
            else if (evt == EVT_RIGHT_SHORT && e->cursor_index < 5) {
                e->cursor_index++;
            }
            else if (evt == EVT_CENTER_SHORT) {
                e->state = UI_EDIT;
            }
            else if (evt == EVT_CENTER_LONG) {
                e->state = UI_SAVE;
            }
            break;

        case UI_EDIT:
            if (evt == EVT_LEFT_SHORT) {
                editor_dec_digit(e);
            }
            else if (evt == EVT_RIGHT_SHORT) {
                editor_inc_digit(e);
            }
            else if (evt == EVT_CENTER_SHORT) {
                e->state = UI_NAV;
            }
            else if (evt == EVT_CENTER_LONG) {
                e->state = UI_SAVE;
            }
            break;

        case UI_SAVE:
            // tidak handle input
            break;
    }
}

int32_t nvs_load_i32(const char *key, int32_t def)
{
    nvs_handle_t handle;
    int32_t val = def;

    if (nvs_open("storage", NVS_READONLY, &handle) == ESP_OK) {
        nvs_get_i32(handle, key, &val);
        nvs_close(handle);
    }

    return val;
}

void nvs_save_i32(const char *key, int32_t val)
{
    nvs_handle_t handle;
    ESP_ERROR_CHECK(nvs_open("storage", NVS_READWRITE, &handle));
    ESP_ERROR_CHECK(nvs_set_i32(handle, key, val));
    ESP_ERROR_CHECK(nvs_commit(handle));
    nvs_close(handle);
}

static int32_t get_average_single_sensor(int index, size_t samples)
{
    int64_t sum = 0;

    for (size_t i = 0; i < samples; i++)
    {
        sum += app.lc[index].raw;  // ambil raw masing-masing
        vTaskDelay(pdMS_TO_TICKS(10));
    }

    return (int32_t)(sum / samples);
}

float calculate_weight(int32_t raw, int32_t tare, int32_t calib, int32_t known_weight)
{
    int32_t delta_calib = calib - tare;

    if (delta_calib == 0) return 0.0f;

    float scale = (float)known_weight / (float)delta_calib;

    return (raw - tare) * scale;
}

int32_t get_total_weight_raw(app_state_t *app)
{
    int64_t raw_sum = 0;
    int64_t tare_sum = 0;
    int64_t calib_sum = 0;

    for (int i = 0; i < CONFIG_NUM_LOADCELL; i++)
    {
        raw_sum   += app->lc[i].raw;
        tare_sum  += app->lc[i].tare;
        calib_sum += app->lc[i].calib;
    }

    int32_t delta_calib = (int32_t)(calib_sum - tare_sum);

    if (delta_calib == 0)
        return 0;

    int32_t known = editor_get_value(&app->editor);

    if (known < 0) known = 0;
    if (known > 999999) known = 999999;

    float scale = (float)known / (float)delta_calib;

    float weight = (raw_sum - tare_sum) * scale;

    return (int32_t)weight;
}

int32_t get_total_weight(app_state_t *app)
{
    float total = 0.0f;

    for (int i = 0; i < CONFIG_NUM_LOADCELL; i++)
    {
        total += app->lc[i].weight;
    }

    return (int32_t)total;
}

void alarm_update(app_state_t *app)
{
    int32_t total =
        get_total_weight(app);

    gpio_set_level(
        pin_ch1_relay,

        alarm_condition(
            &app->alarm[0],
            total
        )
    );

    gpio_set_level(
        pin_ch2_relay,

        alarm_condition(
            &app->alarm[1],
            total
        )
    );

    gpio_set_level(
        pin_ch3_relay,

        alarm_condition(
            &app->alarm[2],
            total
        )
    );
}