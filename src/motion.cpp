#include "motion.h"
#include "rtos_objects.h"
#include "system_state.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"

#include "driver/gpio.h"

#define PIR_SENSOR_PIN GPIO_NUM_33

void vMotionTask(void *pvParameters)
{
    gpio_config_t pir = {};
    pir.pin_bit_mask = (1ULL << PIR_SENSOR_PIN);
    pir.mode = GPIO_MODE_INPUT;
    pir.pull_down_en = GPIO_PULLDOWN_ENABLE;
    gpio_config(&pir);

    TickType_t lastActivity = xTaskGetTickCount();
    const TickType_t timeout = pdMS_TO_TICKS(15000);

    while (1)
    {
        EventBits_t bits = xEventGroupGetBits(g_systemEvents);

        if (gpio_get_level(PIR_SENSOR_PIN))
        {
            lastActivity = xTaskGetTickCount();
            g_systemState = SystemState::ACTIVE;

            xEventGroupSetBits(g_systemEvents,
                               EVENT_ACTIVE | EVENT_MOTION);
        }
        else
        {
            xEventGroupClearBits(g_systemEvents, EVENT_MOTION);
        }

        // Any user interaction refreshes timeout
        if (bits & EVENT_ACTIVE)
            lastActivity = xTaskGetTickCount();

        if ((xTaskGetTickCount() - lastActivity) >= timeout)
        {
            g_systemState = SystemState::INACTIVE;

            xEventGroupClearBits(g_systemEvents,
                                 EVENT_ACTIVE | EVENT_MOTION);
        }

        vTaskDelay(pdMS_TO_TICKS(100));
    }
}