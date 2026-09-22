#ifndef RTOS_OBJECTS_H
#define RTOS_OBJECTS_H

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/event_groups.h"
#include "freertos/semphr.h"

#define EVENT_ACTIVE (1 << 0)
#define EVENT_MOTION (1 << 1)
#define EVENT_ALARM  (1 << 2)

extern QueueHandle_t displayQueue;
extern QueueHandle_t navQueue;
extern EventGroupHandle_t g_systemEvents;
extern SemaphoreHandle_t serialMutex;

void init_rtos_objects();

#endif