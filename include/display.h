#ifndef DISPLAY_H
#define DISPLAY_H

#if __has_include("driver/gpio.h")
#include "driver/gpio.h"
#ifndef I2C_SDA_PIN
#define I2C_SDA_PIN GPIO_NUM_21
#endif
#ifndef I2C_SCL_PIN
#define I2C_SCL_PIN GPIO_NUM_22
#endif
#endif

#include "input.h"

#ifdef __cplusplus
extern "C" {
#endif

void display_task(void *pvParameters);

#ifdef __cplusplus
}
#endif

#endif 