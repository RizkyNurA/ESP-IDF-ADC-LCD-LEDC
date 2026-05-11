#include "selector.h"
#include "types.h"
#include <stdint.h>
#include <stdbool.h>

static const char *alarm_mode_items[] =
{
    "HIGH",
    "LOW"
};

void selector_init(
    selector_t *s,
    const char **items,
    uint8_t count,
    uint8_t initial
)
{
    s->items = items;
    s->count = count;
    s->selected = initial;
}