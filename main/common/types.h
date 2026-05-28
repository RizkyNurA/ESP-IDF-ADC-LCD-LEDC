#ifndef TYPES_H
#define TYPES_H

#include <stdbool.h>
#include "editor.h"
#include "hx711_driver.h"
#include "config.h"

typedef enum {
    APP_LOADING,
    APP_IDLE,
    APP_MENU,
    APP_MONITOR,
    APP_CONFIG_ALARM,
    APP_CALIB_TARE,
    APP_CALIB_TARE_WAIT,
    APP_CALIB_INPUT,
    APP_CALIB_INPUT_WAIT,
    APP_CALIB_DONE
} app_screen_t;

typedef enum
{
    ALARM_ATAS,
    ALARM_BAWAH,
    ALARM_DALAM,
    ALARM_LUAR

} alarm_mode_t;

typedef enum
{
    ALARM_UI_SELECT,
    ALARM_UI_EDIT_VALUE1,
    ALARM_UI_EDIT_VALUE2,
    ALARM_UI_EDIT_MODE,
    ALARM_UI_EDIT_ENABLE

} alarm_ui_state_t;


typedef struct {
    int32_t raw;
    int32_t tare;
    int32_t calib;
    float   weight;
} loadcell_t;

typedef struct
{
    uint8_t selected;
    uint8_t count;

    const char **items;

} selector_t;

typedef struct
{
    bool enabled;

    alarm_mode_t mode;

    int32_t threshold_low;
    int32_t threshold_high;
    bool relay_state;

    selector_t selector;

} alarm_t;
typedef struct {
    uint32_t duty;
    editor_t editor;
    app_screen_t screen;
    bool system_ready;
    int32_t wait_counter;
    loadcell_t lc[CONFIG_NUM_LOADCELL];
    uint8_t lc_index;
    int current_lc;
    alarm_t alarm[ALARM_COUNT];
    alarm_ui_state_t ui_state;
    uint8_t current_alarm;
    selector_t menu_selector;

} app_state_t;
typedef struct {
    hx711_t *scale;
    int index; // 0 atau 1
} hx711_ctx_t;

typedef enum {
    EVT_LEFT_SHORT,
    EVT_LEFT_LONG,
    EVT_LEFT_VERY_LONG,

    EVT_RIGHT_SHORT,
    EVT_RIGHT_LONG,
    EVT_RIGHT_VERY_LONG,

    EVT_CENTER_SHORT,
    EVT_CENTER_LONG,
    EVT_CENTER_VERY_LONG
} app_event_t;

typedef struct {
    hx711_t scale;
} system_ctx_t;

#endif // TYPES_H