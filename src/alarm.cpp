#include "alarm.h"
#include "rtos_objects.h"
#include "sensors.h"
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"

#ifndef BUZZER_PIN
#define BUZZER_PIN GPIO_NUM_27
#endif

AlarmState evaluateTemperature(float temperature) {
    if (temperature < TEMP_LOW_THRESHOLD) {
        return ALARM_LOW_TEMPERATURE;
    } else if (temperature > TEMP_HIGH_THRESHOLD) {
        return ALARM_HIGH_TEMPERATURE;
    }
    return ALARM_NORMAL;
}

void alarm_task(void *pvParameters) {
    gpio_reset_pin(BUZZER_PIN);
    gpio_set_direction(BUZZER_PIN, GPIO_MODE_OUTPUT);
    gpio_set_level(BUZZER_PIN, 0);

    SensorData data;
    memset(&data, 0, sizeof(SensorData));

    for (;;) {
        if (sensorQueue != NULL) {
            xQueuePeek(sensorQueue, &data, portMAX_DELAY);
        }

        bool is_active = true;
        if (systemEvents != NULL) {
            EventBits_t bits = xEventGroupGetBits(systemEvents);
            is_active = (bits & EVENT_ACTIVE) != 0;
        }

        AlarmState state = evaluateTemperature(data.temperature);

        if (is_active && (state != ALARM_NORMAL)) {
            gpio_set_level(BUZZER_PIN, 1);
            vTaskDelay(pdMS_TO_TICKS(150));
            gpio_set_level(BUZZER_PIN, 0);
            vTaskDelay(pdMS_TO_TICKS(150));
        } else {
            gpio_set_level(BUZZER_PIN, 0);
            vTaskDelay(pdMS_TO_TICKS(500));
        }
    }
}