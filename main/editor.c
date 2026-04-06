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
    e->edit_mode = true;
}

void editor_move_left(editor_t *e)
{
    if (e->edit_mode)
    {
        if (e->cursor_col > COL_MIN)
            e->cursor_col--;
    }
    else
    {
        if (e->cursor_col < COL_MIN || e->cursor_col > COL_MAX)
            return;

        int place = get_place(e->cursor_col);
        int digit = (e->value / place) % 10;

        digit = (digit == 0) ? 5 : digit - 1;

        e->value = e->value - ((e->value / place % 10) * place)
                             + (digit * place);
    }
}

void editor_move_right(editor_t *e)
{
    if (e->edit_mode)
    {
        if (e->cursor_col < COL_MAX)
            e->cursor_col++;
    }
    else
    {
        if (e->cursor_col < COL_MIN || e->cursor_col > COL_MAX)
            return;

        int place = get_place(e->cursor_col);
        int digit = (e->value / place) % 10;

        digit = (digit == 5) ? 0 : digit + 1;

        e->value = e->value - ((e->value / place % 10) * place)
                             + (digit * place);
    }
}

void editor_toggle_mode(editor_t *e)
{
    e->edit_mode = !e->edit_mode;
}

uint16_t editor_get_value(editor_t *e)
{
    return e->value;
}