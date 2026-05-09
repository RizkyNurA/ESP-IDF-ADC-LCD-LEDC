#ifndef APP_LOGIC_H
#define APP_LOGIC_H

#include "types.h"

// App events
void app_handle_event(app_state_t *app, app_event_t evt);

// Editor events
void editor_handle_event(editor_t *e, app_event_t evt);

// NVS Storage
void nvs_save_i32(const char *key, int32_t def);

int32_t nvs_load_i32(const char *key, int32_t def);

void app_update(app_state_t *app);

float calculate_weight(int32_t raw, int32_t tare, int32_t calib, int32_t known_weight);

int32_t get_total_weight_raw(app_state_t *app);

int32_t get_total_weight(app_state_t *app);

void alarm_update(app_state_t *app);

#endif // APP_LOGIC_H