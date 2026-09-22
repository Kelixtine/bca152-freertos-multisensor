#include "motion.h"
#include "rtos_objects.h"
#include "system_state.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "driver/gpio.h"

#define PIR_SENSOR_PIN GPIO_NUM_33 

void vMotionTask(void *pvParameters) {
    vTaskDelay(pdMS_TO_TICKS(100));

    gpio_config_t pir_conf = {};
    pir_conf.pin_bit_mask = (1ULL << PIR_SENSOR_PIN);
    pir_conf.mode = GPIO_MODE_INPUT;
    pir_conf.pull_up_en = GPIO_PULLUP_DISABLE;
    pir_conf.pull_down_en = GPIO_PULLDOWN_ENABLE;
    pir_conf.intr_type = GPIO_INTR_DISABLE;
    gpio_config(&pir_conf);

    TickType_t xLastMotionTime = xTaskGetTickCount();
    const TickType_t xInactivityTimeout = pdMS_TO_TICKS(15000);

    while (1) {
        int motionState = gpio_get_level(PIR_SENSOR_PIN);

        if (motionState == 1) {
            xLastMotionTime = xTaskGetTickCount();
            
            if (g_systemState == SystemState::INACTIVE) {
                g_systemState = SystemState::ACTIVE;
            }

            xEventGroupSetBits(g_systemEvents, EVENT_ACTIVE | EVENT_MOTION);
        } else {
            xEventGroupClearBits(g_systemEvents, EVENT_MOTION);
        }

        if (g_systemState == SystemState::ACTIVE && 
           (xTaskGetTickCount() - xLastMotionTime >= xInactivityTimeout)) {
            
            g_systemState = SystemState::INACTIVE;
            xEventGroupClearBits(g_systemEvents, EVENT_ACTIVE);
        }

        vTaskDelay(pdMS_TO_TICKS(100)); 
    }
}