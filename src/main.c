#include <stdio.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"

#include "esp_adc/adc_oneshot.h"
#include "driver/i2c_master.h"
#include "ssd1306.h"

#define SDA_PIN 21
#define SCL_PIN 22

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
int latestLight = 0;

void ldrTask(void *pv)
{
    SensorData data;
    int light;

    while (1) {
        adc_oneshot_read(adc_handle, ADC_CHANNEL_6, &light);

        strcpy(data.sensor, "LDR");
        data.value1 = (float)light;
        data.value2 = 0;

        xQueueSend(sensorQueue, &data, portMAX_DELAY);
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

void dhtTask(void *pv)
{
    SensorData data;

    while (1) {
        strcpy(data.sensor, "DHT");
        data.value1 = 24.0;
        data.value2 = 40.0;

        xQueueSend(sensorQueue, &data, portMAX_DELAY);
        vTaskDelay(pdMS_TO_TICKS(2000));
    }
}

void displayTask(void *pv)
{
    SensorData rx;
    char line[32];

    while (1) {
        if (xQueueReceive(sensorQueue, &rx, portMAX_DELAY)) {

            if (strcmp(rx.sensor, "LDR") == 0) {
                latestLight = (int)rx.value1;
            } else {
                latestTemp = rx.value1;
                latestHum = rx.value2;
            }

            ssd1306_clear_screen(&dev, false);

            sprintf(line, "Temp: %.1f C", latestTemp);
            ssd1306_display_text(&dev, 0, line, strlen(line), false);

            sprintf(line, "Hum : %.1f %%", latestHum);
            ssd1306_display_text(&dev, 2, line, strlen(line), false);

            sprintf(line, "Light: %d", latestLight);
            ssd1306_display_text(&dev, 4, line, strlen(line), false);
        }
    }
}

void app_main(void)
{
    sensorQueue = xQueueCreate(5, sizeof(SensorData));

    adc_oneshot_unit_init_cfg_t init_cfg = {
        .unit_id = ADC_UNIT_1
    };
    adc_oneshot_new_unit(&init_cfg, &adc_handle);

    adc_oneshot_chan_cfg_t chan_cfg = {
        .atten = ADC_ATTEN_DB_12,
        .bitwidth = ADC_BITWIDTH_DEFAULT
    };
    adc_oneshot_config_channel(adc_handle, ADC_CHANNEL_6, &chan_cfg);

    i2c_master_init(&dev, SDA_PIN, SCL_PIN, -1);
    ssd1306_init(&dev, 128, 64);

    xTaskCreate(ldrTask, "LDR", 4096, NULL, 1, NULL);
    xTaskCreate(dhtTask, "DHT", 4096, NULL, 1, NULL);
    xTaskCreate(displayTask, "OLED", 4096, NULL, 1, NULL);

    printf("Multi-Sensor System Started\n");
}