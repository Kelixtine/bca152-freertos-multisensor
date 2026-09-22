#include "alarm.h"
#include "rtos_objects.h"
#include "driver/gpio.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"

#include <stdio.h>

#define BUZZER_PIN GPIO_NUM_14

AlarmState evaluateTemperature(float temp)
{
    if (temp < TEMP_THRESHOLD_LOW)
        return AlarmState::LOW_TEMP;

    if (temp > TEMP_THRESHOLD_HIGH)
        return AlarmState::HIGH_TEMP;

    return AlarmState::NORMAL;
}

void vAlarmTask(void *pvParameters)
{
    gpio_config_t io = {};
    io.pin_bit_mask = (1ULL << BUZZER_PIN);
    io.mode = GPIO_MODE_OUTPUT;
    gpio_config(&io);

    bool previous = false;

    while (1)
    {
        bool active = xEventGroupGetBits(g_systemEvents) & EVENT_ALARM;

        gpio_set_level(BUZZER_PIN, active);

        if (active != previous)
        {
            printf("[BUZZER] %s\n",
                   active ? "Activated" : "Deactivated");
            previous = active;
        }

        vTaskDelay(pdMS_TO_TICKS(50));
    }
}