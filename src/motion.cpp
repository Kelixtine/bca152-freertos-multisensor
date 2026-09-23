#include "motion.h"
#include "rtos_objects.h"
#include "sensors.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "driver/gpio.h"

#define INACTIVITY_MS 15000

static TickType_t lastMotionTick = 0;

void motion_task(void *pvParameters)
{
    gpio_config_t io_conf = {};
    io_conf.pin_bit_mask = (1ULL << PIR_PIN);
    io_conf.mode = GPIO_MODE_INPUT;
    io_conf.pull_down_en = GPIO_PULLDOWN_ENABLE;
    gpio_config(&io_conf);

    lastMotionTick = xTaskGetTickCount();

    xEventGroupSetBits(systemEvents, EVENT_ACTIVE);

    safe_log("[MotionTask] System ACTIVE");

    while (true)
    {
        bool motion = gpio_get_level(PIR_PIN);

        if (motion)
        {
            lastMotionTick = xTaskGetTickCount();

            xEventGroupSetBits(
                systemEvents,
                EVENT_ACTIVE | EVENT_MOTION
            );
        }
        else
        {
            xEventGroupClearBits(
                systemEvents,
                EVENT_MOTION
            );

            TickType_t elapsed =
                xTaskGetTickCount() - lastMotionTick;

            if (elapsed >= pdMS_TO_TICKS(INACTIVITY_MS))
            {
                EventBits_t currentBits =
                    xEventGroupGetBits(systemEvents);

                if (currentBits & EVENT_ACTIVE)
                {
                    xEventGroupClearBits(
                        systemEvents,
                        EVENT_ACTIVE
                    );

                    safe_log(
                        "[MotionTask] Inactivity timeout -> INACTIVE"
                    );
                }
            }
        }

        vTaskDelay(pdMS_TO_TICKS(100));
    }
}