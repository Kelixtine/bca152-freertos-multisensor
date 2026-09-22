#include "motion.h"
#include "rtos_objects.h"
#include "display.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "driver/gpio.h"

#define PIR_PIN GPIO_NUM_33
#define INACTIVITY_MS 15000

static TickType_t lastActivityTick = 0;

void motion_task(void *pvParameters)
{
    gpio_config_t io = {};
    io.pin_bit_mask = (1ULL << PIR_PIN);
    io.mode = GPIO_MODE_INPUT;
    io.pull_down_en = GPIO_PULLDOWN_ENABLE;
    gpio_config(&io);

    lastActivityTick = xTaskGetTickCount();
    xEventGroupSetBits(systemEvents, EVENT_ACTIVE);

    for (;;)
    {
        bool motion = gpio_get_level(PIR_PIN);

        if (motion)
        {
            lastActivityTick = xTaskGetTickCount();
            xEventGroupSetBits(systemEvents, EVENT_ACTIVE | EVENT_MOTION);
        }
        else
        {
            xEventGroupClearBits(systemEvents, EVENT_MOTION);

            if ((xTaskGetTickCount() - lastActivityTick) >=
                pdMS_TO_TICKS(INACTIVITY_MS))
            {
                xEventGroupClearBits(systemEvents, EVENT_ACTIVE);
            }
        }

        vTaskDelay(pdMS_TO_TICKS(100));
    }
}