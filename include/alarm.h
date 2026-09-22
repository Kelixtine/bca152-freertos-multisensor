#ifndef ALARM_H
#define ALARM_H

enum class AlarmState {
    NORMAL,
    LOW_TEMP,
    HIGH_TEMP
};

constexpr float TEMP_THRESHOLD_LOW = 18.0f;
constexpr float TEMP_THRESHOLD_HIGH = 30.0f;

AlarmState evaluateTemperature(float temp);
void vAlarmTask(void *pvParameters);

#endif