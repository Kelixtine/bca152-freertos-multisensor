#include "display.h"
#include "sensors.h"
#include "input.h"
#include "rtos_objects.h"

#include <stdio.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "rom/ets_sys.h"

#define I2C_SDA_PIN GPIO_NUM_21
#define I2C_SCL_PIN GPIO_NUM_22
#define SSD1306_ADDR 0x78

DisplayMode currentDisplayMode = MODE_TEMPERATURE;

/* KEEP YOUR EXISTING FONT + I2C FUNCTIONS HERE
   get_glyph()
   i2c_bus_init()
   i2c_bus_start()
   i2c_bus_stop()
   i2c_bus_write_byte()
   oled_write_command()
   oled_init()
   oled_clear()
*/

static void oled_render_text(const char *line1,
                             const char *line2,
                             const char *line3)
{
    uint8_t buffer[1024];
    memset(buffer, 0, sizeof(buffer));

    const char *lines[3] = { line1, line2, line3 };
    const int pages[3] = { 1, 3, 5 };

    for (int l = 0; l < 3; l++)
    {
        int x = 8;

        for (int i = 0; i < strlen(lines[l]); i++)
        {
            const uint8_t *g = get_glyph(lines[l][i]);

            for (int c = 0; c < 5; c++)
                buffer[pages[l] * 128 + x + c] = g[c];

            x += 6;
        }
    }

    oled_write_command(0x21);
    oled_write_command(0);
    oled_write_command(127);
    oled_write_command(0x22);
    oled_write_command(0);
    oled_write_command(7);

    for (int i = 0; i < 1024; i += 16)
    {
        i2c_bus_start();
        i2c_bus_write_byte(SSD1306_ADDR);
        i2c_bus_write_byte(0x40);

        for (int j = 0; j < 16; j++)
            i2c_bus_write_byte(buffer[i + j]);

        i2c_bus_stop();
    }
}

void display_task(void *pvParameters)
{
    oled_init();
    oled_clear();

    SensorData data = {};
    char line1[32];
    char line2[32];
    char line3[32];

    bool oledOn = true;

    while (1)
    {
        if (sensorQueue)
            xQueueReceive(sensorQueue, &data, 0);

        int mode;
        if (modeQueue &&
            xQueueReceive(modeQueue, &mode, 0) == pdTRUE)
        {
            currentDisplayMode = (DisplayMode)mode;
        }

        bool active = true;

        if (systemEvents)
        {
            EventBits_t bits = xEventGroupGetBits(systemEvents);
            active = bits & EVENT_ACTIVE;
        }

        if (!active)
        {
            if (oledOn)
            {
                oled_clear();
                oled_write_command(0xAE);
                oledOn = false;
            }

            vTaskDelay(pdMS_TO_TICKS(100));
            continue;
        }

        if (!oledOn)
        {
            oled_write_command(0xAF);
            oledOn = true;
        }

        strcpy(line1, "ROOM MONITOR");

        switch (currentDisplayMode)
        {
        case MODE_TEMPERATURE:
            strcpy(line2, "Page: Temp");
            snprintf(line3, sizeof(line3),
                     "Val: %.1f C", data.temperature);
            break;

        case MODE_HUMIDITY:
            strcpy(line2, "Page: Humidity");
            snprintf(line3, sizeof(line3),
                     "Val: %.1f %%", data.humidity);
            break;

        case MODE_LIGHT:
            strcpy(line2, "Page: Light");
            snprintf(line3, sizeof(line3),
                     "Val: %d %%", data.lightLevel);
            break;

        case MODE_MOTION:
            strcpy(line2, "Page: Motion");
            snprintf(line3, sizeof(line3),
                     "Val: %s",
                     data.motionDetected ? "DETECTED" : "CLEAR");
            break;
        }

        oled_render_text(line1, line2, line3);

        vTaskDelay(pdMS_TO_TICKS(100));
    }
}