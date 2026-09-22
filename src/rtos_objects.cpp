#include "rtos_objects.h"
#include "input.h"
#include <stdio.h>

QueueHandle_t sensorQueue = NULL;
QueueHandle_t modeQueue = NULL;
SemaphoreHandle_t serialMutex = NULL;
EventGroupHandle_t systemEvents = NULL;
EventGroupHandle_t systemStateEventGroup = NULL;

void safe_log(const char *msg) {
    if (msg == NULL) return;
    if (serialMutex != NULL) {
        if (xSemaphoreTake(serialMutex, pdMS_TO_TICKS(200)) == pdTRUE) {
            printf("%s\n", msg);
            xSemaphoreGive(serialMutex);
        }
    } else {
        printf("%s\n", msg);
    }
}

void rtos_objects_init(void) {
    if (sensorQueue == NULL) {
        sensorQueue = xQueueCreate(1, sizeof(SensorData));
    }
    if (modeQueue == NULL) {
        modeQueue = xQueueCreate(1, sizeof(DisplayMode));
    }
    if (serialMutex == NULL) {
        serialMutex = xSemaphoreCreateMutex();
    }
    if (systemEvents == NULL) {
        systemEvents = xEventGroupCreate();
        if (systemEvents != NULL) {
            xEventGroupSetBits(systemEvents, EVENT_ACTIVE);
        }
    }
    systemStateEventGroup = systemEvents;
}