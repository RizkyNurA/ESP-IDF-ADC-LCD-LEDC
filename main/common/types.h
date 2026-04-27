#ifndef TYPES_H
#define TYPES_H

#include <stdbool.h>
#include "editor.h"
#include "hx711_driver.h"

typedef enum {
    APP_LOADING,
    APP_IDLE,
    APP_MENU,
    APP_CALIB_TARE,
    APP_CALIB_TARE_WAIT,
    APP_CALIB_INPUT,
    APP_CALIB_INPUT_WAIT,
    APP_CALIB_DONE
} app_screen_t;

typedef struct {
    uint32_t duty;
    int32_t raw;
    editor_t editor;
    app_screen_t screen;
    bool system_ready;
    int32_t tare;
    int32_t calib;
    int32_t wait_counter;
    float weight;

} app_state_t;

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