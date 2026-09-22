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

void vDisplayTask(void *pvParameters) {
    SSD1306_t dev;
    i2c_master_init(&dev, CONFIG_SDA_GPIO, CONFIG_SCL_GPIO, CONFIG_RESET_GPIO);
    ssd1306_init(&dev, 128, 64);
    ssd1306_clear_screen(&dev, false);
    ssd1306_contrast(&dev, 0xff);

    SensorData sensorData = {24.0f, 55.0f, 50, false};
    DisplayMode currentMode = DisplayMode::TEMPERATURE;
    NavDirection navDir;

    char titleBuf[32];
    char valueBuf[32];

    while (1) {
        if (xQueueReceive(navQueue, &navDir, 0) == pdTRUE) {
            if (navDir == NavDirection::NEXT) {
                currentMode = getNextDisplayMode(currentMode);
            } else if (navDir == NavDirection::PREVIOUS) {
                currentMode = getPreviousDisplayMode(currentMode);
            }
        }

        xQueueReceive(displayQueue, &sensorData, 0);

        EventBits_t uxBits = xEventGroupGetBits(g_systemEvents);
        if ((uxBits & EVENT_ACTIVE) == 0) {
            ssd1306_clear_screen(&dev, false);
            vTaskDelay(pdMS_TO_TICKS(200));
            continue;
        }

        ssd1306_clear_screen(&dev, false);
        ssd1306_display_text(&dev, 0, " ROOM MONITOR  ", 15, false);

        switch (currentMode) {
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
                snprintf(valueBuf, sizeof(valueBuf), "Val: %d %%", sensorData.lightLevel);
                break;
            case DisplayMode::MOTION:
                snprintf(titleBuf, sizeof(titleBuf), "Page: Motion");
                snprintf(valueBuf, sizeof(valueBuf), "Val: %s", sensorData.motionDetected ? "DETECTED" : "CLEAR");
                break;
        }

        ssd1306_display_text(&dev, 2, titleBuf, 15, false);
        ssd1306_display_text(&dev, 4, valueBuf, 15, false);

        vTaskDelay(pdMS_TO_TICKS(100));
    }
}