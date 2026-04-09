#ifndef APP_LOGIC_H
#define APP_LOGIC_H

#include "types.h"

// App events
void app_handle_event(app_state_t *app, app_event_t evt);

// Editor events
void editor_handle_event(editor_t *e, app_event_t evt);

// NVS Storage
void save_editor_value(int val);
int load_editor_value(void);

void nvs_save_i32(const char *key, int32_t def);
int32_t nvs_load_i32(const char *key, int32_t def);


void save_tare_value(int32_t raw);


int32_t get_tare_average_from_app(size_t samples);

#endif // APP_LOGIC_H