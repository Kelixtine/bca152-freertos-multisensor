#ifndef INPUT_H
#define INPUT_H

#ifdef UNIT_TESTING

typedef int gpio_num_t;

#define GPIO_NUM_18 18
#define GPIO_NUM_19 19

#define ENCODER_CLK GPIO_NUM_18
#define ENCODER_DT GPIO_NUM_19

#else

#include "driver/gpio.h"

#ifndef ENCODER_CLK
#define ENCODER_CLK GPIO_NUM_18
#endif

#ifndef ENCODER_DT
#define ENCODER_DT GPIO_NUM_19
#endif

#endif

typedef enum {
    MODE_TEMPERATURE = 0,
    MODE_HUMIDITY = 1,
    MODE_LIGHT = 2,
    MODE_MOTION = 3,
    MODE_COUNT = 4
} DisplayMode;

#ifdef __cplusplus
extern "C" {
#endif

DisplayMode nextDisplayMode(DisplayMode current);

DisplayMode previousDisplayMode(DisplayMode current);

void input_task(void *pvParameters);

#ifdef __cplusplus
}
#endif

#endif