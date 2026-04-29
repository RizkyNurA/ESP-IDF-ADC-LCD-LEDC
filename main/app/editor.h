#ifndef EDITOR_H
#define EDITOR_H

#include <stdint.h>
#include <stdbool.h>

typedef enum {
    UI_NAV,
    UI_EDIT,
    UI_SAVE
} ui_state_t;

typedef struct {
    uint32_t value;          // FIX: harus 32-bit
    uint8_t cursor_index;
    ui_state_t state;
} editor_t;

// FIX: parameter 32-bit
void editor_init(editor_t *e, uint32_t initial);

void editor_move_left(editor_t *e);
void editor_move_right(editor_t *e);
void editor_toggle_mode(editor_t *e);

// FIX: return 32-bit
uint32_t editor_get_value(editor_t *e);

void editor_dec_digit(editor_t *e);
void editor_inc_digit(editor_t *e);

uint8_t editor_get_digit(editor_t *e, uint8_t index);

uint8_t editor_get_cursor_index(editor_t *e);

bool editor_should_blink(editor_t *e, uint8_t index);

#endif