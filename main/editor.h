#ifndef EDITOR_H
#define EDITOR_H

#include <stdint.h>
#include <stdbool.h>

typedef struct {
    uint16_t value;
    uint8_t cursor_col;
    bool edit_mode;
} editor_t;

void editor_init(editor_t *e, uint16_t initial);

void editor_move_left(editor_t *e);
void editor_move_right(editor_t *e);
void editor_toggle_mode(editor_t *e);

uint16_t editor_get_value(editor_t *e);

#endif