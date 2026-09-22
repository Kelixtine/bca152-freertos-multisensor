#include "alarm.h"
#include "rtos_objects.h"
#include "sensors.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "driver/gpio.h"
#include <stdio.h>

#define BUZZER_PIN GPIO_NUM_14

AlarmState evaluateTemperature(float temp) {
    if (temp < TEMP_THRESHOLD_LOW) {
        return AlarmState::LOW_TEMP;
    } else if (temp > TEMP_THRESHOLD_HIGH) {
        return AlarmState::HIGH_TEMP;
    }
    return AlarmState::NORMAL;
}

void vAlarmTask(void *pvParameters) {
    gpio_config_t buzzer_conf = {};
    buzzer_conf.pin_bit_mask = (1ULL << BUZZER_PIN);
    buzzer_conf.mode = GPIO_MODE_OUTPUT;
    gpio_config(&buzzer_conf);

    bool lastAlarmActive = false;

    for (;;) {
        EventBits_t bits = xEventGroupGetBits(g_systemEvents);
        bool alarmActive = (bits & EVENT_ALARM) != 0;

        if (alarmActive != lastAlarmActive) {
            printf("[AlarmTask] Alarm state changed -> %s\n", alarmActive ? "ACTIVE" : "NORMAL");
            lastAlarmActive = alarmActive;
        }

        gpio_set_level(BUZZER_PIN, alarmActive ? 1 : 0);
        vTaskDelay(pdMS_TO_TICKS(200));
    }
}