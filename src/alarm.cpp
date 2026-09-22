#include "alarm.h"
#include "rtos_objects.h"
#include "sensors.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/ledc.h"

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
    ledc_timer_config_t timer = {};
    timer.speed_mode = LEDC_LOW_SPEED_MODE;
    timer.timer_num = LEDC_TIMER_0;
    timer.duty_resolution = LEDC_TIMER_10_BIT;
    timer.freq_hz = 2000;
    timer.clk_cfg = LEDC_AUTO_CLK;
    ledc_timer_config(&timer);

    ledc_channel_config_t channel = {};
    channel.gpio_num = BUZZER_PIN;
    channel.speed_mode = LEDC_LOW_SPEED_MODE;
    channel.channel = LEDC_CHANNEL_0;
    channel.intr_type = LEDC_INTR_DISABLE;
    channel.timer_sel = LEDC_TIMER_0;
    channel.duty = 0;
    channel.hpoint = 0;
    ledc_channel_config(&channel);

    SensorData data = {};
    bool lastState = false;

    while (true)
    {
        if (sensorQueue != NULL)
            xQueueReceive(sensorQueue, &data, 0);

        AlarmState state = evaluateTemperature(data.temperature);
        bool alarmActive = (state != ALARM_NORMAL);

        if (alarmActive)
        {
            ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, 512);
            ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0);
        }
        else
        {
            ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, 0);
            ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0);
        }

        if (alarmActive != lastState)
        {
            if (alarmActive)
                safe_log("[AlarmTask] Activated");
            else
                safe_log("[AlarmTask] Deactivated");

            lastState = alarmActive;
        }

        vTaskDelay(pdMS_TO_TICKS(50));
    }
}