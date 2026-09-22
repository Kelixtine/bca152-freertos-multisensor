#ifndef INPUT_H
#define INPUT_H

#include "driver/gpio.h"

#define ENCODER_CLK GPIO_NUM_25
#define ENCODER_DT  GPIO_NUM_26
#define ENCODER_SW  GPIO_NUM_27

enum class NavDirection {
    PREVIOUS = -1,
    NEXT = 1
};

void vInputTask(void *pvParameters);

#endif