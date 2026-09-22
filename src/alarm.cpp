#include "alarm.h"
#include "rtos_objects.h"
#include "sensors.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"

AlarmState evaluateTemperature(float temperature)
{
    if (temperature <= TEMP_LOW_THRESHOLD)
        return ALARM_LOW_TEMPERATURE;

    if (temperature >= TEMP_HIGH_THRESHOLD)
        return ALARM_HIGH_TEMPERATURE;

    return ALARM_NORMAL;
}

void alarm_task(void *pvParameters)
{
    gpio_config_t io_conf = {};
    io_conf.pin_bit_mask = (1ULL << BUZZER_PIN);
    io_conf.mode = GPIO_MODE_OUTPUT;
    gpio_config(&io_conf);

    SensorData data = {};
    bool lastAlarmState = false;

    while (true)
    {
        if (sensorQueue != NULL)
            xQueueReceive(sensorQueue, &data, 0);

        AlarmState state = evaluateTemperature(data.temperature);
        bool alarmActive = (state != ALARM_NORMAL);

        // Active buzzer
        gpio_set_level(BUZZER_PIN, alarmActive ? 1 : 0);

        // Terminal log only when state changes
        if (alarmActive != lastAlarmState)
        {
            if (alarmActive)
                safe_log("[AlarmTask] Activated");
            else
                safe_log("[AlarmTask] Deactivated");

            lastAlarmState = alarmActive;
        }

        vTaskDelay(pdMS_TO_TICKS(100));
    }
}