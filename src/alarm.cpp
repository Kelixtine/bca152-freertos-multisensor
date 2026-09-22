#include "alarm.h"
#include "rtos_objects.h"
#include "sensors.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"

#define BUZZER_PIN GPIO_NUM_14

void alarm_task(void *pvParameters)
{
    gpio_config_t io_conf = {};
    io_conf.pin_bit_mask = (1ULL << BUZZER_PIN);
    io_conf.mode = GPIO_MODE_OUTPUT;
    gpio_config(&io_conf);

    SensorData data = {};
    bool alarmActive = false;
    bool lastState = false;

    while (true)
    {
        if (sensorQueue != NULL)
            xQueueReceive(sensorQueue, &data, 0);

        // Alarm condition
        alarmActive =
            (data.temperature >= 30.0f) ||
            (data.temperature <= 18.0f);

        // Buzzer ON/OFF immediately
        gpio_set_level(BUZZER_PIN, alarmActive ? 1 : 0);

        // Print only when state changes
        if (alarmActive != lastState)
        {
            if (alarmActive)
                safe_log("[AlarmTask] Activated");
            else
                safe_log("[AlarmTask] Deactivated");

            lastState = alarmActive;
        }

        vTaskDelay(pdMS_TO_TICKS(100));
    }
}