#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "rtos_objects.h"
#include "sensors.h"
#include "display.h"
#include "input.h"
#include "alarm.h"
#include "motion.h"

#ifndef UNIT_TEST
extern "C" void app_main() {
    printf("BCA152 FreeRTOS Multisensor\n");
    printf("System starting...\n");
    vTaskDelay(1000 / portTICK_PERIOD_MS); 

    printf("Hardware initialization skipped. Creating FreeRTOS objects.\n");

    init_rtos_objects();

    printf("Objects created. Starting tasks.\n");

    xTaskCreate(vMotionTask,  "Motion Task",  2048, NULL, 3, NULL);
    xTaskCreate(vInputTask,   "Input Task",   2048, NULL, 3, NULL);
    xTaskCreate(vSensorTask,  "Sensor Task",  4096, NULL, 2, NULL); 
    xTaskCreate(vAlarmTask,   "Alarm Task",   2048, NULL, 2, NULL);
    xTaskCreate(vDisplayTask, "Display Task", 4096, NULL, 1, NULL);

    printf("All tasks started successfully!\n");
}
#endif