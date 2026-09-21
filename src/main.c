
#include <stdio.h>
#include <string.h>
#include <stdbool.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"

#include "driver/gpio.h"
#include "driver/i2c.h"
#include "driver/ledc.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_timer.h"

#include "ssd1306.h"

// ---------- Pins ----------
#define SDA_PIN 21
#define SCL_PIN 22

#define BUZZER_PIN 18
#define PIR_PIN 19

#define ENC_CLK 25
#define ENC_DT 26
#define ENC_SW 27

#define LDR_CHANNEL ADC_CHANNEL_6

// ---------- OLED ----------
SSD1306_t dev;

// ---------- Queue ----------
typedef struct {
    char sensor[8];
    float value1;
    float value2;
} SensorData;

QueueHandle_t sensorQueue;

// ---------- Display Pages ----------
typedef enum {
    TEMPERATURE,
    HUMIDITY,
    LIGHT,
    MOTION
} DisplayMode;

DisplayMode currentPage = TEMPERATURE;

// ---------- System State ----------
typedef enum {
    ACTIVE,
    INACTIVE
} SystemState;

SystemState systemState = ACTIVE;

// ---------- Globals ----------
adc_oneshot_unit_handle_t adc_handle;

float latestTemp = 24.0;
float latestHum = 40.0;
int latestLight = 1001;

bool motionDetected = false;
int64_t lastMotionTime = 0;

// ---------- LDR Task ----------
void ldrTask(void *pv)
{
    SensorData data;
    int light;

    while (1) {
        adc_oneshot_read(adc_handle, LDR_CHANNEL, &light);

        strcpy(data.sensor, "LDR");
        data.value1 = light;

        xQueueSend(sensorQueue, &data, portMAX_DELAY);
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

// ---------- DHT Task ----------
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

// ---------- Rotary Encoder ----------
void inputTask(void *pv)
{
    gpio_set_direction(ENC_CLK, GPIO_MODE_INPUT);
    gpio_set_direction(ENC_DT, GPIO_MODE_INPUT);

    gpio_set_pull_mode(ENC_CLK, GPIO_PULLUP_ONLY);
    gpio_set_pull_mode(ENC_DT, GPIO_PULLUP_ONLY);

    int lastCLK = gpio_get_level(ENC_CLK);

    while (1) {
        if (systemState == ACTIVE) {
            int clk = gpio_get_level(ENC_CLK);

            if (clk != lastCLK && clk == 0) {
                if (gpio_get_level(ENC_DT))
                    currentPage = (DisplayMode)((currentPage + 1) % 4);
                else
                    currentPage = (DisplayMode)((currentPage + 3) % 4);
            }

            lastCLK = clk;
        }

        // Prevent watchdog
        vTaskDelay(pdMS_TO_TICKS(20));
    }
}

// ---------- Motion Task ----------
void motionTask(void *pv)
{
    gpio_set_direction(PIR_PIN, GPIO_MODE_INPUT);

    int previousState = 0;

    while (1) {
        int pir = gpio_get_level(PIR_PIN);

        // Detect rising edge only
        if (pir == 1 && previousState == 0) {
            motionDetected = true;
            systemState = ACTIVE;
            lastMotionTime = esp_timer_get_time();
        }

        previousState = pir;

        if (motionDetected) {
            int64_t elapsed =
                (esp_timer_get_time() - lastMotionTime) / 1000000;

            if (elapsed >= 15) {
                motionDetected = false;
                systemState = INACTIVE;
            }
        }

        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

// ---------- Display ----------
void displayTask(void *pv)
{
    SensorData rx;
    char line[32];

    while (1) {
        if (xQueueReceive(sensorQueue, &rx, portMAX_DELAY)) {

            if (strcmp(rx.sensor, "LDR") == 0)
                latestLight = (int)rx.value1;
            else {
                latestTemp = rx.value1;
                latestHum = rx.value2;
            }

            if (systemState == INACTIVE) {
                ssd1306_clear_screen(&dev, false);
                continue;
            }

            ssd1306_clear_screen(&dev, false);

            switch (currentPage) {

                case TEMPERATURE:
                    ssd1306_display_text(&dev, 0, "TEMPERATURE", 11, false);
                    sprintf(line, "%.1f C", latestTemp);
                    break;

                case HUMIDITY:
                    ssd1306_display_text(&dev, 0, "HUMIDITY", 8, false);
                    sprintf(line, "%.1f %%", latestHum);
                    break;

                case LIGHT:
                    ssd1306_display_text(&dev, 0, "LIGHT", 5, false);
                    sprintf(line, "%d", latestLight);
                    break;

                case MOTION:
                    ssd1306_display_text(&dev, 0, "MOTION", 6, false);
                    sprintf(line, "%s",
                            motionDetected ? "DETECTED" : "NONE");
                    break;
            }

            ssd1306_display_text(&dev, 2, line, strlen(line), false);

            // Buzzer (Part VIII)
            if (latestLight < 500)
                ledc_set_duty(LEDC_LOW_SPEED_MODE,
                              LEDC_CHANNEL_0, 512);
            else
                ledc_set_duty(LEDC_LOW_SPEED_MODE,
                              LEDC_CHANNEL_0, 0);

            ledc_update_duty(LEDC_LOW_SPEED_MODE,
                             LEDC_CHANNEL_0);
        }
    }
}

// ---------- Main ----------
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
    adc_oneshot_config_channel(adc_handle,
                               LDR_CHANNEL,
                               &chan_cfg);

    i2c_master_init(&dev, SDA_PIN, SCL_PIN, -1);
    ssd1306_init(&dev, 128, 64);

    ledc_timer_config_t timer = {
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .timer_num = LEDC_TIMER_0,
        .duty_resolution = LEDC_TIMER_10_BIT,
        .freq_hz = 2000,
        .clk_cfg = LEDC_AUTO_CLK
    };
    ledc_timer_config(&timer);

    ledc_channel_config_t buzzer = {
        .gpio_num = BUZZER_PIN,
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .channel = LEDC_CHANNEL_0,
        .timer_sel = LEDC_TIMER_0,
        .duty = 0,
        .hpoint = 0
    };
    ledc_channel_config(&buzzer);

    lastMotionTime = esp_timer_get_time();

    printf("Part IX Started\n");

    xTaskCreate(ldrTask, "LDR", 4096, NULL, 1, NULL);
    xTaskCreate(dhtTask, "DHT", 4096, NULL, 1, NULL);
    xTaskCreate(inputTask, "INPUT", 4096, NULL, 1, NULL);
    xTaskCreate(motionTask, "PIR", 4096, NULL, 1, NULL);
    xTaskCreate(displayTask, "OLED", 4096, NULL, 1, NULL);
}