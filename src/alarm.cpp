#include "alarm.h"
#include "rtos_objects.h"

#include "driver/ledc.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"

#include <stdio.h>

#define BUZZER_PIN GPIO_NUM_14

AlarmState evaluateTemperature(float t)
{
    if (t < TEMP_THRESHOLD_LOW) return AlarmState::LOW_TEMP;
    if (t > TEMP_THRESHOLD_HIGH) return AlarmState::HIGH_TEMP;
    return AlarmState::NORMAL;
}

void vAlarmTask(void *pvParameters)
{
    ledc_timer_config_t timer = {
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .timer_num = LEDC_TIMER_0,
        .duty_resolution = LEDC_TIMER_10_BIT,
        .freq_hz = 2000,
        .clk_cfg = LEDC_AUTO_CLK
    };
    ledc_timer_config(&timer);

    ledc_channel_config_t ch = {
        .gpio_num = BUZZER_PIN,
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .channel = LEDC_CHANNEL_0,
        .timer_sel = LEDC_TIMER_0,
        .duty = 0,
        .hpoint = 0
    };
    ledc_channel_config(&ch);

    bool previous = false;

    while (1)
    {
        bool alarm = xEventGroupGetBits(g_systemEvents) & EVENT_ALARM;

        ledc_set_duty(LEDC_LOW_SPEED_MODE,
                      LEDC_CHANNEL_0,
                      alarm ? 512 : 0);

        ledc_update_duty(LEDC_LOW_SPEED_MODE,
                         LEDC_CHANNEL_0);

        if (alarm != previous)
        {
            printf("[BUZZER] %s\n",
                   alarm ? "Activated" : "Deactivated");
            previous = alarm;
        }

        vTaskDelay(pdMS_TO_TICKS(50));
    }
}