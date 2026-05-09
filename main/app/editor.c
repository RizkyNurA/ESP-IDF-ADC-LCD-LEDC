#include "editor.h"

#define DIGIT_MIN   0
#define DIGIT_MAX   5
#define DIGIT_COUNT (DIGIT_MAX + 1)

#define VALUE_MAX   999999

static int get_place(uint8_t index)
{
    int place = 1;
    for (int i = 0; i < (DIGIT_COUNT - 1 - index); i++) {
        place *= 10;
    }
    return place;
}

void editor_init(editor_t *e, uint32_t initial)
{
    if (initial > VALUE_MAX)
        initial = VALUE_MAX;

    e->value = initial;
    e->cursor_index = 0;
    e->state = UI_NAV;
}

void editor_move_left(editor_t *e)
{
    switch (e->state)
    {
        case UI_NAV:
            if (e->cursor_index > DIGIT_MIN)
                e->cursor_index--;
            break;

        case UI_EDIT:
            editor_dec_digit(e);
            break;

        case UI_SAVE:
            break;
    }
}

void editor_move_right(editor_t *e)
{
    switch (e->state)
    {
        case UI_NAV:
            if (e->cursor_index < DIGIT_MAX)
                e->cursor_index++;
            break;

        case UI_EDIT:
            editor_inc_digit(e);
            break;

        case UI_SAVE:
            break;
    }
}

void editor_toggle_mode(editor_t *e)
{
    if (e->state == UI_NAV)
        e->state = UI_EDIT;
    else if (e->state == UI_EDIT)
        e->state = UI_NAV;
}

uint32_t editor_get_value(editor_t *e)
{
    return e->value;
}

void editor_set_value(editor_t *e, uint32_t value)
{
    if (value > VALUE_MAX)
        value = VALUE_MAX;

    e->value = value;
    e->cursor_index = 0;
    e->state = UI_NAV;
}

void editor_dec_digit(editor_t *e)
{
    if (e->cursor_index > DIGIT_MAX)
        return;

    int place = get_place(e->cursor_index);
    int digit = (e->value / place) % 10;

    digit = (digit == 0) ? 9 : digit - 1;

    e->value = e->value - ((e->value / place % 10) * place)
                         + (digit * place);

    if (e->value > VALUE_MAX)
        e->value = VALUE_MAX;
}

void editor_inc_digit(editor_t *e)
{
    if (e->cursor_index > DIGIT_MAX)
        return;

    int place = get_place(e->cursor_index);
    int digit = (e->value / place) % 10;

    digit = (digit == 9) ? 0 : digit + 1;

    e->value = e->value - ((e->value / place % 10) * place)
                         + (digit * place);

    if (e->value > VALUE_MAX)
        e->value = VALUE_MAX;
}

uint8_t editor_get_digit(editor_t *e, uint8_t index)
{
    if (index > DIGIT_MAX)
        return 0;

    int temp = e->value;

    for (int i = 0; i < (DIGIT_COUNT - 1 - index); i++)
        temp /= 10;

    return temp % 10;
}

uint8_t editor_get_cursor_index(editor_t *e)
{
    return e->cursor_index;
}

bool editor_should_blink(editor_t *e, uint8_t index)
{
    if (index != e->cursor_index)
        return false;

    return (e->state == UI_NAV || e->state == UI_EDIT);
}