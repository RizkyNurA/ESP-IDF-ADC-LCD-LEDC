#include "editor.h"

#define DIGIT_MIN 0
#define DIGIT_MAX 3

static int get_place(uint8_t index)
{
    int place = 1;
    for (int i = 0; i < (3 - index); i++) {
        place *= 10;
    }
    return place;
}

void editor_init(editor_t *e, uint16_t initial)
{
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
        {
            editor_dec_digit(e);
            break;
        }

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
        {
            editor_inc_digit(e);
            break;
        }

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

uint16_t editor_get_value(editor_t *e)
{
    return e->value;
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
}

void editor_inc_digit(editor_t *e)
{
    if (e->cursor_index > DIGIT_MAX)
        return;

    int place = get_place(e->cursor_index);
    int digit = (e->value / place) % 10;

    digit = (digit == 9) ? 0 : digit + 1;

    e->value = e->value - ((e->value / place % 10) * place) + (digit * place);
}

uint8_t editor_get_digit(editor_t *e, uint8_t index)
{
    int temp = e->value;

    for (int i = 0; i < (3 - index); i++)
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

    if (e->state == UI_NAV || e->state == UI_EDIT)
        return true;
    
    return false;
}