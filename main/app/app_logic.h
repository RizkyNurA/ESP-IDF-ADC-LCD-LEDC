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

#endif // APP_LOGIC_H