#include "push_button_driver.h"
#include "driver/gpio.h"
#include "esp_timer.h"

press_type_t button_update(button_t *btn,
                           gpio_num_t pin,
                           const button_config_t *cfg)
{
    int level = gpio_get_level(pin);
    int64_t now = esp_timer_get_time();

    switch (btn->state)
    {
        case BTN_IDLE:
            if (level == 1) {
                btn->state = BTN_DEBOUNCE;
                btn->timestamp = now;
            }
            break;

        case BTN_DEBOUNCE:
            if ((now - btn->timestamp) > cfg->debounce_time) {
                if (level == 1) {
                    btn->state = BTN_PRESSED;
                    btn->timestamp = now;
                } else {
                    btn->state = BTN_IDLE;
                }
            }
            break;

        case BTN_PRESSED:
            if (level == 0) {
                int64_t duration = now - btn->timestamp;
                btn->state = BTN_IDLE;

                if (duration >= cfg->long_press_time)
                    return PRESS_LONG;
                else if (duration >= cfg->short_press_time)
                    return PRESS_SHORT;
            }
            else if ((now - btn->timestamp) >= cfg->long_press_time) {
                btn->state = BTN_HOLD;
            }
            break;

        case BTN_HOLD:
            if (level == 0) {
                btn->state = BTN_IDLE;
                return PRESS_LONG;
            }
            break;
    }

    return PRESS_NONE;
}

press_type_t button_get_press_type(gpio_num_t pin)
{
    static int last_state = 0;
    static int64_t press_time = 0;

    int current_state = gpio_get_level(pin);

    // tombol ditekan (rising edge)
    if (last_state == 0 && current_state == 1) {
        press_time = esp_timer_get_time(); // microsecond
    }

    // tombol dilepas (falling edge)
    if (last_state == 1 && current_state == 0) {
        int64_t duration = esp_timer_get_time() - press_time;

        // konversi ke ms
        duration /= 1000;

        if (duration >= 2000) {
            last_state = current_state;
            return PRESS_LONG;
        } 
        else if (duration >= 1000) {
            last_state = current_state;
            return PRESS_SHORT;
        }
    }

    last_state = current_state;
    return PRESS_NONE;
}

edge_t gpio_detect_edge(gpio_num_t pin)
{
    static int last_state = -1;  // -1 = uninitialized
    int current_state = gpio_get_level(pin);

    if (last_state == -1) {
        last_state = current_state;
        return EDGE_NONE;
    }

    edge_t result = EDGE_NONE;

    if (last_state == 0 && current_state == 1) {
        result = EDGE_RISING;
    } 
    else if (last_state == 1 && current_state == 0) {
        result = EDGE_FALLING;
    } 
    else {
    }

    last_state = current_state;
    return result;
}