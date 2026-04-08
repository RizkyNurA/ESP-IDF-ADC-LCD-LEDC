#ifndef APP_CONTEXT_H
#define APP_CONTEXT_H

#include "types.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/queue.h"

extern app_state_t app;
extern SemaphoreHandle_t app_mutex;
extern QueueHandle_t app_queue;

extern system_ctx_t sys;
extern SemaphoreHandle_t sys_mutex;

extern volatile bool toggle_state;

#endif