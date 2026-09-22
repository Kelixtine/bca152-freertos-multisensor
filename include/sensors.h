#ifndef SENSORS_H
#define SENSORS_H

#include <stdbool.h>

#if __has_include("driver/gpio.h")
#include "driver/gpio.h"
#ifndef DHT_PIN
#define DHT_PIN GPIO_NUM_4
#endif
#ifndef LDR_PIN
#define LDR_PIN GPIO_NUM_34
#endif
#ifndef PIR_PIN
#define PIR_PIN GPIO_NUM_13
#endif
#endif

typedef struct {
    float temperature;
    float humidity;
    int lightLevel;
    bool motionDetected;
} SensorData;

#ifdef __cplusplus
extern "C" {
#endif

void sensors_init(void);
void sensor_task(void *pvParameters);

#ifdef __cplusplus
}
#endif

#endif 