#include "rtos_objects.h"
#include "sensors.h"      
#include "display.h"      
#include "system_state.h" 
#include "input.h"

QueueHandle_t displayQueue = NULL;
QueueHandle_t navQueue = NULL;
EventGroupHandle_t g_systemEvents = NULL;
SemaphoreHandle_t serialMutex = NULL;

void init_rtos_objects() {
    displayQueue = xQueueCreate(10, sizeof(SensorData));
    navQueue = xQueueCreate(10, sizeof(NavDirection));
    g_systemEvents = xEventGroupCreate();
    if (g_systemEvents != NULL) {
        xEventGroupSetBits(g_systemEvents, EVENT_ACTIVE);
    }
    serialMutex = xSemaphoreCreateMutex();
}