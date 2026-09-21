#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

void taskA(void *pv) {
    while (1) {
        printf("Task A running\n");
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

void taskB(void *pv) {
    while (1) {
        printf("Task B running\n");
        vTaskDelay(pdMS_TO_TICKS(1500));
    }
}

void app_main(void) {
    printf("System starting...\n");
    xTaskCreate(taskA, "A", 2048, NULL, 1, NULL);
    xTaskCreate(taskB, "B", 2048, NULL, 1, NULL);
}