#include "display.h"
#include "input.h"
#include "rtos_objects.h"
#include "system_state.h"
#include "sensors.h"
#include "ssd1306.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/event_groups.h"

#include <stdio.h>
#include <string.h>

void vDisplayTask(void *pvParameters)
{
    SSD1306_t dev;

    i2c_master_init(&dev, CONFIG_SDA_GPIO, CONFIG_SCL_GPIO, CONFIG_RESET_GPIO);
    ssd1306_init(&dev, 128, 64);
    ssd1306_clear_screen(&dev, false);
    ssd1306_contrast(&dev, 0xff);

    SensorData sensorData = {24.0f, 40.0f, 50, false};
    DisplayMode currentMode = DisplayMode::TEMPERATURE;
    DisplayMode lastMode = DisplayMode::MOTION;

    SensorData lastDrawn = {-999, -999, -1, false};

    NavDirection navDir;

    char titleBuf[32];
    char valueBuf[32];

    bool screenOn = true;

    while (1)
    {
        bool dirty = false;

        if (xQueueReceive(navQueue, &navDir, 0) == pdTRUE)
        {
            if (navDir == NavDirection::NEXT)
                currentMode = getNextDisplayMode(currentMode);
            else
                currentMode = getPreviousDisplayMode(currentMode);

            dirty = true;
        }

        if (xQueueReceive(displayQueue, &sensorData, 0) == pdTRUE)
        {
            dirty = true;
        }

        EventBits_t bits = xEventGroupGetBits(g_systemEvents);

        if ((bits & EVENT_ACTIVE) == 0)
        {
            if (screenOn)
            {
                ssd1306_clear_screen(&dev, false);
                screenOn = false;
            }

            vTaskDelay(pdMS_TO_TICKS(100));
            continue;
        }

        screenOn = true;

        if (lastMode != currentMode)
        {
            lastMode = currentMode;
            dirty = true;
        }

        if (memcmp(&lastDrawn, &sensorData, sizeof(SensorData)) != 0)
        {
            dirty = true;
        }

        if (dirty)
        {
            lastDrawn = sensorData;

            ssd1306_clear_screen(&dev, false);

            ssd1306_display_text(&dev, 0, " ROOM MONITOR ", 14, false);

            switch (currentMode)
            {
                case DisplayMode::TEMPERATURE:
                    snprintf(titleBuf, sizeof(titleBuf), "Page: Temp");
                    snprintf(valueBuf, sizeof(valueBuf), "Val: %.1f C", sensorData.temperature);
                    break;

                case DisplayMode::HUMIDITY:
                    snprintf(titleBuf, sizeof(titleBuf), "Page: Humidity");
                    snprintf(valueBuf, sizeof(valueBuf), "Val: %.1f %%", sensorData.humidity);
                    break;

                case DisplayMode::LIGHT:
                    snprintf(titleBuf, sizeof(titleBuf), "Page: Light");
                    snprintf(valueBuf, sizeof(valueBuf), "Val: %d", sensorData.lightLevel);
                    break;

                case DisplayMode::MOTION:
                    snprintf(titleBuf, sizeof(titleBuf), "Page: Motion");
                    snprintf(valueBuf, sizeof(valueBuf), "Val: %s",
                             sensorData.motionDetected ? "DETECTED" : "CLEAR");
                    break;
            }

            ssd1306_display_text(&dev, 2, titleBuf, strlen(titleBuf), false);
            ssd1306_display_text(&dev, 4, valueBuf, strlen(valueBuf), false);
        }

        vTaskDelay(pdMS_TO_TICKS(50));
    }
}