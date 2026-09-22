#ifndef SENSORS_H
#define SENSORS_H

#include "driver/gpio.h"

#define DHT_PIN GPIO_NUM_4

struct SensorData {
    float temperature;
    float humidity;
    int lightLevel;
    bool motionDetected;
};

bool read_dht22(float *temp, float *humidity);
void vSensorTask(void *pvParameters);
void vMotionTask(void *pvParameters);

#endif