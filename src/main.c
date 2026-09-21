#include <stdio.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"

#include "esp_adc/adc_oneshot.h"
#include "driver/i2c.h"
#include "driver/gpio.h"
#include "driver/ledc.h"

#include "ssd1306.h"

#define SDA_PIN 21
#define SCL_PIN 22

#define BUZZER_PIN 18
#define ENC_CLK 25
#define ENC_DT 26
#define ENC_SW 27

typedef struct {
    char sensor[8];
    float value1;
    float value2;
} SensorData;

typedef enum {
    TEMPERATURE,
    HUMIDITY,
    LIGHT,
    MOTION
} DisplayMode;

QueueHandle_t sensorQueue;
adc_oneshot_unit_handle_t adc_handle;
SSD1306_t dev;

DisplayMode currentPage = TEMPERATURE;

float latestTemp = 24.0;
float latestHum = 40.0;
int latestLight = 1001;
int motionDetected = 0;

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
        strcpy(data.sensor, "DHT");
        data.value1 = 24.0;
        data.value2 = 40.0;

        xQueueSend(sensorQueue, &data, portMAX_DELAY);

        vTaskDelay(pdMS_TO_TICKS(2000));
    }
}

// ---------------- ROTARY ENCODER ----------------
void inputTask(void *pv)
{
    gpio_set_direction(ENC_CLK, GPIO_MODE_INPUT);
    gpio_set_direction(ENC_DT, GPIO_MODE_INPUT);
    gpio_set_direction(ENC_SW, GPIO_MODE_INPUT);

    gpio_set_pull_mode(ENC_CLK, GPIO_PULLUP_ONLY);
    gpio_set_pull_mode(ENC_DT, GPIO_PULLUP_ONLY);
    gpio_set_pull_mode(ENC_SW, GPIO_PULLUP_ONLY);

    int lastCLK = gpio_get_level(ENC_CLK);

    while (1)
    {
        int clk = gpio_get_level(ENC_CLK);

        if (clk != lastCLK && clk == 0)
        {
            if (gpio_get_level(ENC_DT))
                currentPage = (currentPage + 1) % 4;
            else
                currentPage = (currentPage + 3) % 4;
        }

        lastCLK = clk;
        vTaskDelay(pdMS_TO_TICKS(5));
    }
}

// ---------------- DISPLAY TASK ----------------
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

            ssd1306_clear_screen(&dev, false);

            switch (currentPage)
            {
                case TEMPERATURE:
                    sprintf(line, "TEMPERATURE");
                    ssd1306_display_text(&dev, 0, line, strlen(line), false);
                    sprintf(line, "%.1f C", latestTemp);
                    ssd1306_display_text(&dev, 2, line, strlen(line), false);
                    break;

                case HUMIDITY:
                    sprintf(line, "HUMIDITY");
                    ssd1306_display_text(&dev, 0, line, strlen(line), false);
                    sprintf(line, "%.1f %%", latestHum);
                    ssd1306_display_text(&dev, 2, line, strlen(line), false);
                    break;

                case LIGHT:
                    sprintf(line, "LIGHT");
                    ssd1306_display_text(&dev, 0, line, strlen(line), false);
                    sprintf(line, "%d", latestLight);
                    ssd1306_display_text(&dev, 2, line, strlen(line), false);
                    break;

                case MOTION:
                    sprintf(line, "MOTION");
                    ssd1306_display_text(&dev, 0, line, strlen(line), false);
                    sprintf(line, motionDetected ? "DETECTED" : "NONE");
                    ssd1306_display_text(&dev, 2, line, strlen(line), false);
                    break;
            }

            // Buzzer alarm (Part VIII kept)
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

    adc_oneshot_unit_init_cfg_t init_cfg = {
        .unit_id = ADC_UNIT_1
    };
    adc_oneshot_new_unit(&init_cfg, &adc_handle);

    adc_oneshot_chan_cfg_t chan_cfg = {
        .bitwidth = ADC_BITWIDTH_DEFAULT,
        .atten = ADC_ATTEN_DB_12
    };
    adc_oneshot_config_channel(adc_handle, ADC_CHANNEL_6, &chan_cfg);

    i2c_master_init(&dev, SDA_PIN, SCL_PIN, -1);
    ssd1306_init(&dev, 128, 64);

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

    printf("Part VII - Rotary Encoder Started\n");

    xTaskCreate(ldrTask, "LDR", 4096, NULL, 1, NULL);
    xTaskCreate(dhtTask, "DHT", 4096, NULL, 1, NULL);
    xTaskCreate(inputTask, "INPUT", 4096, NULL, 1, NULL);
    xTaskCreate(displayTask, "OLED", 4096, NULL, 1, NULL);
}