#include "app_context.h"

app_state_t app = {0};
SemaphoreHandle_t app_mutex;
QueueHandle_t app_queue;
system_ctx_t sys;
SemaphoreHandle_t sys_mutex;
volatile bool toggle_state = 0;