#ifndef RTOS_OBJECTS_H
#define RTOS_OBJECTS_H

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"
#include "freertos/event_groups.h"
#include "sensors.h"

#ifdef __cplusplus
extern "C" {
#endif

#define EVENT_ACTIVE (1 << 0)
#define EVENT_MOTION (1 << 1)
#define EVENT_ALARM  (1 << 2)

#define STATE_ACTIVE_BIT EVENT_ACTIVE

extern QueueHandle_t sensorQueue;
extern QueueHandle_t modeQueue;
extern SemaphoreHandle_t serialMutex;
extern EventGroupHandle_t systemEvents;
extern EventGroupHandle_t systemStateEventGroup;

void safe_log(const char *msg);
void rtos_objects_init(void);

#ifdef __cplusplus
}
#endif

#endif 