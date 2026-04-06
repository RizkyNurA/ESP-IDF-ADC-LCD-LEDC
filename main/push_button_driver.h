#ifndef PUSH_BUTTON_DRIVER_H
#define PUSH_BUTTON_DRIVER_H

#include "driver/gpio.h"

typedef enum {
    EDGE_NONE = 0,
    EDGE_RISING,
    EDGE_FALLING
} edge_t;

typedef struct {
    int64_t debounce_time;
    int64_t short_press_time;
    int64_t long_press_time;
} button_config_t;

typedef enum {
    PRESS_NONE = 0,
    PRESS_SHORT,   // ~1 detik
    PRESS_LONG     // ~2 detik
} press_type_t;

typedef enum {
    BTN_IDLE = 0,     // tidak ditekan
    BTN_DEBOUNCE,     // lagi stabilisasi
    BTN_PRESSED,      // sedang ditekan
    BTN_HOLD,         // sudah lama ditekan
} btn_state_t;

typedef struct {
    btn_state_t state;
    int last_level;
    int64_t timestamp;
} button_t;

typedef struct {
    gpio_num_t pin;          // nomor GPIO
    button_t state;          // state machine tombol
    button_config_t cfg;     // konfigurasi timing (debounce)
    void (*callback)(press_type_t event); // memisahkan event
} button_ctx_t;

typedef struct {
    button_ctx_t *buttons;
    int count;
} button_group_t;

press_type_t button_update(button_t *btn,
                           gpio_num_t pin,
                           const button_config_t *cfg);

press_type_t button_get_press_type(gpio_num_t pin);

edge_t gpio_detect_edge(gpio_num_t pin);

#endif