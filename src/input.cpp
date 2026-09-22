#include "input.h"
#include "rtos_objects.h"
#include "system_state.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"

void vInputTask(void *pvParameters)
{
    vTaskDelay(pdMS_TO_TICKS(100));

    gpio_config_t io = {};
    io.pin_bit_mask =
        (1ULL << ENCODER_CLK) |
        (1ULL << ENCODER_DT) |
        (1ULL << ENCODER_SW);

    io.mode = GPIO_MODE_INPUT;
    io.pull_up_en = GPIO_PULLUP_ENABLE;
    gpio_config(&io);

    int lastCLK = gpio_get_level(ENCODER_CLK);

    while (1)
    {
        int clk = gpio_get_level(ENCODER_CLK);

        if (lastCLK == 1 && clk == 0)
        {
            NavDirection dir =
                (gpio_get_level(ENCODER_DT) != clk)
                    ? NavDirection::NEXT
                    : NavDirection::PREVIOUS;

            gLastActivityTick = xTaskGetTickCount();
            g_systemState = SystemState::ACTIVE;
            xEventGroupSetBits(g_systemEvents, EVENT_ACTIVE);

            xQueueSend(navQueue, &dir, 0);
        }

        lastCLK = clk;
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}