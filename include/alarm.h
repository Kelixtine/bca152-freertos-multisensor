#ifndef ALARM_H
#define ALARM_H

#include <stdbool.h>

#if __has_include("driver/gpio.h")
#include "driver/gpio.h"
#ifndef BUZZER_PIN
#define BUZZER_PIN GPIO_NUM_27
#endif
#endif

#define TEMP_LOW_THRESHOLD  18.0f
#define TEMP_HIGH_THRESHOLD 30.0f

typedef enum {
    ALARM_NORMAL = 0,
    ALARM_LOW_TEMPERATURE,
    ALARM_HIGH_TEMPERATURE
} AlarmState;

#ifdef __cplusplus
extern "C" {
#endif

AlarmState evaluateTemperature(float temperature);
void alarm_task(void *pvParameters);

#ifdef __cplusplus
}
#endif

#endif 