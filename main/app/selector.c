#include "selector.h"
#include "types.h"
#include <stdint.h>
#include <stdbool.h>



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

void selector_handle_event(selector_t *s, app_event_t evt)
{
    if (evt == EVT_LEFT_SHORT)
    {
        if (s->selected > 0)
            s->selected--;
    }
    else if (evt == EVT_RIGHT_SHORT)
    {
        if (s->selected < (s->count - 1))
            s->selected++;
    }
}