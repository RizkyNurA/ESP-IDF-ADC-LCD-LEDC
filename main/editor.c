#include "editor.h"

#define COL_MIN 12
#define COL_MAX 15

static int get_place(uint8_t col)
{
    int digit_index = col - COL_MIN;

    int place = 1;
    for (int i = 0; i < 3 - digit_index; i++) {
        place *= 10;
    }
    return place;
}

void editor_init(editor_t *e, uint16_t initial)
{
    e->value = initial;
    e->cursor_col = COL_MIN;
    e->state = UI_NAV;
}

void editor_move_left(editor_t *e)
{
    switch (e->state)
    {
        case UI_NAV:
            if (e->cursor_col > COL_MIN)
                e->cursor_col--;
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
            if (e->cursor_col < COL_MAX)
                e->cursor_col++;
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
    if (e->cursor_col < COL_MIN || e->cursor_col > COL_MAX)
        return;

    int place = get_place(e->cursor_col);
    int digit = (e->value / place) % 10;

    digit = (digit == 0) ? 9 : digit - 1;

    e->value = e->value - ((e->value / place % 10) * place)
                         + (digit * place);
}

void editor_inc_digit(editor_t *e)
{
    if (e->cursor_col < COL_MIN || e->cursor_col > COL_MAX)
        return;

    int place = get_place(e->cursor_col);
    int digit = (e->value / place) % 10;

    digit = (digit == 9) ? 0 : digit + 1;

    e->value = e->value - ((e->value / place % 10) * place) + (digit * place);
}