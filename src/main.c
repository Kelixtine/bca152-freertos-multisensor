#include <stdio.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"

#include "esp_adc/adc_oneshot.h"
#include "driver/ledc.h"
#include "driver/i2c.h"

#include "ssd1306.h"

#define SDA_PIN 21
#define SCL_PIN 22
#define BUZZER_PIN 18

typedef struct {
    char sensor[8];
    float value1;
    float value2;
} SensorData;

QueueHandle_t sensorQueue;
adc_oneshot_unit_handle_t adc_handle;
SSD1306_t dev;

float latestTemp = 24.0;
float latestHum = 40.0;
int latestLight = 1001;

// ---------------- LDR TASK ----------------
void ldrTask(void *pv)
{
    SensorData data;
    int light;

    while (1)
    {
        adc_oneshot_read(adc_handle, ADC_CHANNEL_6, &light);

        strcpy(data.sensor, "LDR");
        data.value1 = light;

        xQueueSend(sensorQueue, &data, portMAX_DELAY);

        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

// ---------------- DHT TASK ----------------
void dhtTask(void *pv)
{
    SensorData data;

    while (1)
    {
        // Simulated values for Wokwi
        strcpy(data.sensor, "DHT");
        data.value1 = 24.0;
        data.value2 = 40.0;

        xQueueSend(sensorQueue, &data, portMAX_DELAY);

        vTaskDelay(pdMS_TO_TICKS(2000));
    }
}

// ---------------- DISPLAY + BUZZER ----------------
void displayTask(void *pv)
{
    SensorData rx;
    char line[32];

    while (1)
    {
        if (xQueueReceive(sensorQueue, &rx, portMAX_DELAY))
        {
            if (strcmp(rx.sensor, "LDR") == 0)
                latestLight = (int)rx.value1;
            else
            {
                latestTemp = rx.value1;
                latestHum = rx.value2;
            }

            // OLED
            ssd1306_clear_screen(&dev, false);

            sprintf(line, "Temp: %.1f C", latestTemp);
            ssd1306_display_text(&dev, 0, line, strlen(line), false);

            sprintf(line, "Hum : %.1f %%", latestHum);
            ssd1306_display_text(&dev, 2, line, strlen(line), false);

            sprintf(line, "Light: %d", latestLight);
            ssd1306_display_text(&dev, 4, line, strlen(line), false);

            // BUZZER
            if (latestLight < 500)
            {
                ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, 512);
                ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0);
            }
            else
            {
                ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, 0);
                ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0);
            }
        }
    }
}

// ---------------- APP MAIN ----------------
void app_main(void)
{
    sensorQueue = xQueueCreate(5, sizeof(SensorData));

    // ADC (LDR on GPIO34)
    adc_oneshot_unit_init_cfg_t init_cfg = {
        .unit_id = ADC_UNIT_1
    };
    adc_oneshot_new_unit(&init_cfg, &adc_handle);

    adc_oneshot_chan_cfg_t chan_cfg = {
        .bitwidth = ADC_BITWIDTH_DEFAULT,
        .atten = ADC_ATTEN_DB_12
    };
    adc_oneshot_config_channel(adc_handle, ADC_CHANNEL_6, &chan_cfg);

    // OLED
    i2c_master_init(&dev, SDA_PIN, SCL_PIN, -1);
    ssd1306_init(&dev, 128, 64);

    // BUZZER PWM
    ledc_timer_config_t timer = {
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .duty_resolution = LEDC_TIMER_10_BIT,
        .timer_num = LEDC_TIMER_0,
        .freq_hz = 2000,
        .clk_cfg = LEDC_AUTO_CLK
    };
    ledc_timer_config(&timer);

    ledc_channel_config_t channel = {
        .gpio_num = BUZZER_PIN,
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .channel = LEDC_CHANNEL_0,
        .timer_sel = LEDC_TIMER_0,
        .duty = 0,
        .hpoint = 0
    };
    ledc_channel_config(&channel);

    printf("Multi-Sensor System Started\n");

    xTaskCreate(ldrTask, "LDR", 4096, NULL, 1, NULL);
    xTaskCreate(dhtTask, "DHT", 4096, NULL, 1, NULL);
    xTaskCreate(displayTask, "OLED", 4096, NULL, 1, NULL);
}