#ifndef SELECTOR_H
#define SELECTOR_H

#include "types.h"
#include <stdint.h>
#include <stdbool.h>

void selector_init(
    selector_t *s,
    const char **items,
    uint8_t count,
    uint8_t initial
);

void selector_handle_event(
    selector_t *s,
    app_event_t evt
);

#endif