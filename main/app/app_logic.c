#include "app_logic.h"
#include "app_context.h"
#include "nvs_flash.h"
#include "nvs.h"
#include "config.h"
#include "esp_log.h"

void app_handle_event(app_state_t *app, app_event_t evt)
{
    void save_editor_value(int val);
    int load_editor_value(void);
    
    switch (app->screen)
    {
        case APP_LOADING:
            break;
            
        case APP_IDLE:
            if (evt == EVT_CENTER_SHORT)
                app->screen = APP_MENU;
            break;

        case APP_MENU:
            if (evt == EVT_CENTER_SHORT)
                app->screen = APP_CALIB_TARE;
            else if (evt == EVT_CENTER_LONG)
                app->screen = APP_IDLE;
            break;

        case APP_CALIB_TARE:
            if (evt == EVT_CENTER_LONG)
            {
                int32_t tare = get_tare_average_from_app(SAMPLE_CALIB_VALUE);
                ESP_LOGI("tare", "get tare %d", tare);
                app->tare = tare;
                ESP_LOGI("tare", "save tare %d in global", tare);
                save_tare_value(app->tare);
                ESP_LOGI("tare", "save tare %d in nvs", tare);
                app->screen = APP_CALIB_INPUT;
            }
            break;

        case APP_CALIB_INPUT:
            if (evt == EVT_LEFT)
                editor_move_left(&app->editor);

            else if (evt == EVT_RIGHT)
                editor_move_right(&app->editor);

            else if (evt == EVT_CENTER_SHORT)
                editor_toggle_mode(&app->editor);

            else if (evt == EVT_CENTER_LONG)
            {
                int32_t calib = editor_get_value(&app->editor);
                app->calib = calib;
                save_editor_value(app->calib);
                app->screen = APP_CALIB_DONE;
            }
            break;

        case APP_CALIB_DONE:
            if (evt == EVT_CENTER_LONG)
            app->screen = APP_IDLE;
            break;
    }
}

void editor_handle_event(editor_t *e, app_event_t evt)
{
    switch (e->state)
    {
        case UI_NAV:
            if (evt == EVT_LEFT && e->cursor_index > 12) {
                e->cursor_index--;
            }
            else if (evt == EVT_RIGHT && e->cursor_index < 15) {
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
            if (evt == EVT_LEFT) {
                editor_dec_digit(e);
            }
            else if (evt == EVT_RIGHT) {
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

void save_tare_value(int32_t raw)
{
    nvs_handle_t handle;
    ESP_ERROR_CHECK(nvs_open("storage", NVS_READWRITE, &handle));

    ESP_ERROR_CHECK(nvs_set_i32(handle, "tare_offset", raw));

    ESP_ERROR_CHECK(nvs_commit(handle));
    nvs_close(handle);
}

int32_t get_tare_average_from_app(size_t samples)
{
    int64_t sum = 0;

    for (size_t i = 0; i < samples; i++)
    {
        int32_t val = app.raw;

        sum += val;
        vTaskDelay(pdMS_TO_TICKS(10));
    }

    return (int32_t)(sum / samples);
}