#include "input.h"

DisplayMode nextDisplayMode(DisplayMode current)
{
    return (DisplayMode)(
        ((int)current + 1) % (int)MODE_COUNT
    );
}

DisplayMode previousDisplayMode(DisplayMode current)
{
    if (current == MODE_TEMPERATURE)
    {
        return (DisplayMode)((int)MODE_COUNT - 1);
    }

    return (DisplayMode)((int)current - 1);
}

#ifndef UNIT_TESTING

#include <stdio.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "rtos_objects.h"

void input_task(void *pvParameters)
{
    gpio_reset_pin((gpio_num_t)ENCODER_CLK);
    gpio_reset_pin((gpio_num_t)ENCODER_DT);

    gpio_set_direction(
        (gpio_num_t)ENCODER_CLK,
        GPIO_MODE_INPUT
    );

    gpio_set_pull_mode(
        (gpio_num_t)ENCODER_CLK,
        GPIO_PULLUP_ONLY
    );

    gpio_set_direction(
        (gpio_num_t)ENCODER_DT,
        GPIO_MODE_INPUT
    );

    gpio_set_pull_mode(
        (gpio_num_t)ENCODER_DT,
        GPIO_PULLUP_ONLY
    );

    int last_clk =
        gpio_get_level((gpio_num_t)ENCODER_CLK);

    DisplayMode current_mode = MODE_TEMPERATURE;

    char log_buf[64];

    if (modeQueue != NULL)
    {
        xQueueOverwrite(
            modeQueue,
            &current_mode
        );
    }

    for (;;)
    {
        int current_clk =
            gpio_get_level((gpio_num_t)ENCODER_CLK);

        if (last_clk == 1 && current_clk == 0)
        {
            int dt_val =
                gpio_get_level((gpio_num_t)ENCODER_DT);

            if (dt_val == 1)
            {
                current_mode =
                    nextDisplayMode(current_mode);

                snprintf(
                    log_buf,
                    sizeof(log_buf),
                    "[InputTask] CW -> Mode: %d",
                    (int)current_mode
                );
            }
            else
            {
                current_mode =
                    previousDisplayMode(current_mode);

                snprintf(
                    log_buf,
                    sizeof(log_buf),
                    "[InputTask] CCW -> Mode: %d",
                    (int)current_mode
                );
            }

            safe_log(log_buf);

            if (modeQueue != NULL)
            {
                xQueueOverwrite(
                    modeQueue,
                    &current_mode
                );
            }

            vTaskDelay(
                pdMS_TO_TICKS(150)
            );

            last_clk =
                gpio_get_level(
                    (gpio_num_t)ENCODER_CLK
                );
        }
        else
        {
            last_clk = current_clk;

            vTaskDelay(
                pdMS_TO_TICKS(10)
            );
        }
    }
}

#endif