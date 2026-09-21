#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <stdlib.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/event_groups.h"
#include "freertos/semphr.h"

#include "driver/gpio.h"
#include "driver/i2c.h"
#include "driver/ledc.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_timer.h"

#include "ssd1306.h"

// ---------------- PINS ----------------
#define SDA_PIN 21
#define SCL_PIN 22
#define BUZZER_PIN 18
#define PIR_PIN 19
#define ENC_CLK 25
#define ENC_DT 26
#define LDR_CHANNEL ADC_CHANNEL_6

// ---------------- EVENT GROUP ----------------
#define EVENT_ACTIVE BIT0
#define EVENT_MOTION BIT1
#define EVENT_ALARM  BIT2

EventGroupHandle_t eventGroup;
SemaphoreHandle_t serialMutex;

// ---------------- OLED ----------------
SSD1306_t dev;

// ---------------- SENSOR STRUCT ----------------
typedef struct {
    char sensor[8];
    float value1;
    float value2;
} SensorData;

QueueHandle_t sensorQueue;

// ---------------- DISPLAY ----------------
typedef enum {
    TEMPERATURE,
    HUMIDITY,
    LIGHT,
    MOTION
} DisplayMode;

DisplayMode currentPage = TEMPERATURE;

// ---------------- GLOBALS ----------------
adc_oneshot_unit_handle_t adc_handle;

float latestTemp = 24.0;
float latestHum = 40.0;
int latestLight = 1001;

bool motionDetected = false;
int64_t lastMotionTime = 0;

// =====================================================
// LDR TASK
// =====================================================
void ldrTask(void *pv)
{
    SensorData data;
    int light;
    int previous = -1;

    while (1)
    {
        adc_oneshot_read(adc_handle, LDR_CHANNEL, &light);

        if (previous != -1 && abs(light - previous) > 8)
        {
            lastMotionTime = esp_timer_get_time();
            xEventGroupSetBits(eventGroup, EVENT_ACTIVE);

            // Print ONLY when light changes
            xSemaphoreTake(serialMutex, portMAX_DELAY);
            printf("[LDR] %d\n", light);
            xSemaphoreGive(serialMutex);
        }

        previous = light;

        strcpy(data.sensor, "LDR");
        data.value1 = light;

        xQueueSend(sensorQueue, &data, portMAX_DELAY);

       vTaskDelay(pdMS_TO_TICKS(50));
    }
}

// =====================================================
// DHT TASK
// =====================================================
void dhtTask(void *pv)
{
    SensorData data;

    while (1)
    {
        strcpy(data.sensor, "DHT");
        data.value1 = 24.0;
        data.value2 = 40.0;

        xQueueSend(sensorQueue, &data, portMAX_DELAY);

        xSemaphoreTake(serialMutex, portMAX_DELAY);
        printf("[DHT] Temp: %.1f  Hum: %.1f\n",
               data.value1,
               data.value2);
        xSemaphoreGive(serialMutex);

        vTaskDelay(pdMS_TO_TICKS(2000));
    }
}

// =====================================================
// ROTARY ENCODER
// =====================================================
void inputTask(void *pv)
{
    gpio_set_direction(ENC_CLK, GPIO_MODE_INPUT);
    gpio_set_direction(ENC_DT, GPIO_MODE_INPUT);

    gpio_set_pull_mode(ENC_CLK, GPIO_PULLUP_ONLY);
    gpio_set_pull_mode(ENC_DT, GPIO_PULLUP_ONLY);

    int lastCLK = gpio_get_level(ENC_CLK);

    while (1)
    {
        EventBits_t bits = xEventGroupGetBits(eventGroup);

        if (bits & EVENT_ACTIVE)
        {
            int clk = gpio_get_level(ENC_CLK);

            if (clk != lastCLK && clk == 0)
            {
                if (gpio_get_level(ENC_DT))
                    currentPage = (DisplayMode)((currentPage + 1) % 4);
                else
                    currentPage = (DisplayMode)((currentPage + 3) % 4);

                lastMotionTime = esp_timer_get_time();
                xEventGroupSetBits(eventGroup, EVENT_ACTIVE);
            }

            lastCLK = clk;
        }

        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

// =====================================================
// PIR MOTION
// =====================================================
void motionTask(void *pv)
{
    gpio_set_direction(PIR_PIN, GPIO_MODE_INPUT);

    int previous = 0;

    while (1)
    {
        int pir = gpio_get_level(PIR_PIN);

        if (pir == 1 && previous == 0)
        {
            motionDetected = true;
            lastMotionTime = esp_timer_get_time();

            xEventGroupSetBits(eventGroup,
                               EVENT_ACTIVE |
                               EVENT_MOTION);

            xSemaphoreTake(serialMutex, portMAX_DELAY);
            printf("[PIR] Motion detected\n");
            xSemaphoreGive(serialMutex);
        }

        previous = pir;

        if (motionDetected)
        {
            int64_t elapsed =
                (esp_timer_get_time() - lastMotionTime) / 1000000;

            if (elapsed >= 15)
            {
                motionDetected = false;

                xEventGroupClearBits(eventGroup,
                                     EVENT_ACTIVE |
                                     EVENT_MOTION);

                xSemaphoreTake(serialMutex, portMAX_DELAY);
                printf("[PIR] System inactive\n");
                xSemaphoreGive(serialMutex);
            }
        }

        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

// =====================================================
// DISPLAY
// =====================================================
void displayTask(void *pv)
{
    SensorData rx;
    char line[32];

    DisplayMode lastPage = TEMPERATURE;
    bool lastMotion = false;
    bool wasActive = true;

    float shownTemp = -999;
    float shownHum = -999;
    int shownLight = -1;

    while (1)
    {
        bool dirty = false;

        while (xQueueReceive(sensorQueue, &rx, 0))
        {
            if (strcmp(rx.sensor, "LDR") == 0)
            {
                latestLight = (int)rx.value1;

                if (currentPage == LIGHT &&
                    latestLight != shownLight)
                    dirty = true;
            }
            else
            {
                latestTemp = rx.value1;
                latestHum = rx.value2;

                if (currentPage == TEMPERATURE &&
                    latestTemp != shownTemp)
                    dirty = true;

                if (currentPage == HUMIDITY &&
                    latestHum != shownHum)
                    dirty = true;
            }
        }

        if (latestLight < 500)
            xEventGroupSetBits(eventGroup, EVENT_ALARM);
        else
            xEventGroupClearBits(eventGroup, EVENT_ALARM);

        EventBits_t bits = xEventGroupGetBits(eventGroup);

        if (!(bits & EVENT_ACTIVE))
        {
            if (wasActive)
            {
                ssd1306_clear_screen(&dev, false);
                wasActive = false;
            }

            vTaskDelay(pdMS_TO_TICKS(50));
            continue;
        }

        if (!wasActive)
        {
            dirty = true;
            wasActive = true;
        }

        if (lastPage != currentPage)
        {
            lastPage = currentPage;
            dirty = true;
        }

        if (lastMotion != motionDetected)
        {
            lastMotion = motionDetected;

            if (currentPage == MOTION)
                dirty = true;
        }

        if (dirty)
        {
            ssd1306_clear_screen(&dev, false);

            switch (currentPage)
            {
                case TEMPERATURE:
                    ssd1306_display_text(&dev,0,
                        "TEMPERATURE",11,false);
                    sprintf(line,"%.1f C",latestTemp);
                    shownTemp = latestTemp;
                    break;

                case HUMIDITY:
                    ssd1306_display_text(&dev,0,
                        "HUMIDITY",8,false);
                    sprintf(line,"%.1f %%",latestHum);
                    shownHum = latestHum;
                    break;

                case LIGHT:
                    ssd1306_display_text(&dev,0,
                        "LIGHT",5,false);
                    sprintf(line,"%d",latestLight);
                    shownLight = latestLight;
                    break;

                case MOTION:
                    ssd1306_display_text(&dev,0,
                        "MOTION",6,false);
                    sprintf(line,"%s",
                        motionDetected ?
                        "DETECTED" :
                        "NONE");
                    break;
            }

            ssd1306_display_text(&dev,2,
                line,
                strlen(line),
                false);
        }

        if (bits & EVENT_ALARM)
            ledc_set_duty(
                LEDC_LOW_SPEED_MODE,
                LEDC_CHANNEL_0,
                512);
        else
            ledc_set_duty(
                LEDC_LOW_SPEED_MODE,
                LEDC_CHANNEL_0,
                0);

        ledc_update_duty(
            LEDC_LOW_SPEED_MODE,
            LEDC_CHANNEL_0);

        vTaskDelay(pdMS_TO_TICKS(50));
    }
}

// =====================================================
// MAIN
// =====================================================
void app_main(void)
{
    sensorQueue = xQueueCreate(5, sizeof(SensorData));
    eventGroup = xEventGroupCreate();
    serialMutex = xSemaphoreCreateMutex();

    xEventGroupSetBits(eventGroup, EVENT_ACTIVE);

    adc_oneshot_unit_init_cfg_t init_cfg = {
        .unit_id = ADC_UNIT_1
    };
    adc_oneshot_new_unit(&init_cfg, &adc_handle);

    adc_oneshot_chan_cfg_t chan_cfg = {
        .bitwidth = ADC_BITWIDTH_DEFAULT,
        .atten = ADC_ATTEN_DB_12
    };

    adc_oneshot_config_channel(
        adc_handle,
        LDR_CHANNEL,
        &chan_cfg);

    i2c_master_init(&dev,
                    SDA_PIN,
                    SCL_PIN,
                    -1);

    ssd1306_init(&dev,128,64);

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

    printf("Part XI Started\n");

    xTaskCreate(ldrTask,
                "LDR",
                4096,
                NULL,
                1,
                NULL);

    xTaskCreate(dhtTask,
                "DHT",
                4096,
                NULL,
                1,
                NULL);

    xTaskCreate(inputTask,
                "INPUT",
                4096,
                NULL,
                1,
                NULL);

    xTaskCreate(motionTask,
                "PIR",
                4096,
                NULL,
                1,
                NULL);

    xTaskCreate(displayTask,
                "OLED",
                4096,
                NULL,
                1,
                NULL);
}