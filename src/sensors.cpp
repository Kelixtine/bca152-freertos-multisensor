#include "sensors.h"
#include "rtos_objects.h"
#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_adc/adc_oneshot.h"
#include "rom/ets_sys.h"

static portMUX_TYPE dht_mux = portMUX_INITIALIZER_UNLOCKED;
static adc_oneshot_unit_handle_t adc1_handle = NULL;

void sensors_init(void) {
    gpio_reset_pin(DHT_PIN);
    gpio_set_direction(DHT_PIN, GPIO_MODE_INPUT_OUTPUT_OD);
    gpio_set_pull_mode(DHT_PIN, GPIO_PULLUP_ONLY);
    gpio_set_level(DHT_PIN, 1);

    gpio_reset_pin(PIR_PIN);
    gpio_set_direction(PIR_PIN, GPIO_MODE_INPUT);

    // Modernong ESP-IDF Oneshot ADC config nga fully zero-initialized
    adc_oneshot_unit_init_cfg_t init_config1;
    memset(&init_config1, 0, sizeof(init_config1));
    init_config1.unit_id = ADC_UNIT_1;
    init_config1.ulp_mode = ADC_ULP_MODE_DISABLE;
    adc_oneshot_new_unit(&init_config1, &adc1_handle);

    adc_oneshot_chan_cfg_t chan_config;
    memset(&chan_config, 0, sizeof(chan_config));
    chan_config.atten = ADC_ATTEN_DB_12;
    chan_config.bitwidth = ADC_BITWIDTH_12;
    adc_oneshot_config_channel(adc1_handle, ADC_CHANNEL_6, &chan_config);
}

static bool dht22_read(float *temp, float *hum) {
    uint8_t data[5] = {0, 0, 0, 0, 0};

    gpio_set_direction(DHT_PIN, GPIO_MODE_OUTPUT_OD);
    gpio_set_level(DHT_PIN, 0);
    ets_delay_us(1200);
    gpio_set_level(DHT_PIN, 1);
    ets_delay_us(30);
    gpio_set_direction(DHT_PIN, GPIO_MODE_INPUT);

    taskENTER_CRITICAL(&dht_mux);

    int timeout = 100;
    while (gpio_get_level(DHT_PIN) == 1) {
        if (--timeout == 0) {
            taskEXIT_CRITICAL(&dht_mux);
            return false;
        }
        ets_delay_us(1);
    }

    timeout = 100;
    while (gpio_get_level(DHT_PIN) == 0) {
        if (--timeout == 0) {
            taskEXIT_CRITICAL(&dht_mux);
            return false;
        }
        ets_delay_us(1);
    }

    timeout = 100;
    while (gpio_get_level(DHT_PIN) == 1) {
        if (--timeout == 0) {
            taskEXIT_CRITICAL(&dht_mux);
            return false;
        }
        ets_delay_us(1);
    }

    for (int i = 0; i < 40; i++) {
        timeout = 100;

        while (gpio_get_level(DHT_PIN) == 0) {
            if (--timeout == 0) {
                taskEXIT_CRITICAL(&dht_mux);
                return false;
            }
            ets_delay_us(1);
        }

        int t = 0;

        while (gpio_get_level(DHT_PIN) == 1) {
            t++;

            if (t > 150) {
                break;
            }

            ets_delay_us(1);
        }

        data[i / 8] <<= 1;

        if (t > 35) {
            data[i / 8] |= 1;
        }
    }

    taskEXIT_CRITICAL(&dht_mux);

    if (data[4] == ((data[0] + data[1] + data[2] + data[3]) & 0xFF)) {
        int raw_hum = (data[0] << 8) | data[1];
        int raw_temp = ((data[2] & 0x7F) << 8) | data[3];

        if (data[2] & 0x80) {
            raw_temp = -raw_temp;
        }

        *hum = raw_hum / 10.0f;
        *temp = raw_temp / 10.0f;

        return true;
    }

    return false;
}

void sensor_task(void *pvParameters) {
    SensorData current_data;

    memset(&current_data, 0, sizeof(SensorData));

    current_data.temperature = 24.0f;
    current_data.humidity = 55.0f;

    TickType_t lastWakeTime = xTaskGetTickCount();
    const TickType_t period = pdMS_TO_TICKS(2000);

    for (;;) {
        // -----------------------------
        // DHT22 Temperature & Humidity
        // -----------------------------
        float t = 0.0f;
        float h = 0.0f;

        if (dht22_read(&t, &h)) {
            current_data.temperature = t;
            current_data.humidity = h;
        }

        // -----------------------------
        // LDR / Photoresistor
        // -----------------------------
        int raw_adc = 0;

        if (adc1_handle != NULL) {
            adc_oneshot_read(
                adc1_handle,
                ADC_CHANNEL_6,
                &raw_adc
            );
        }

        // Wokwi photoresistor produces a lower ADC value
        // at higher illumination, so invert the ADC reading.
        current_data.lightLevel =
            ((4095 - raw_adc) * 100) / 4095;

        // Keep the value between 0% and 100%.
        if (current_data.lightLevel < 0) {
            current_data.lightLevel = 0;
        }

        if (current_data.lightLevel > 100) {
            current_data.lightLevel = 100;
        }

        // -----------------------------
        // PIR Motion Sensor
        // -----------------------------
        current_data.motionDetected =
            (gpio_get_level(PIR_PIN) == 1);

        // -----------------------------
        // Send sensor data to queue
        // -----------------------------
        if (sensorQueue != NULL) {
            xQueueOverwrite(sensorQueue, &current_data);
        }

        // -----------------------------
        // Serial Logging
        // -----------------------------
        char log_buf[128];

        snprintf(
            log_buf,
            sizeof(log_buf),
            "[SensorTask] Temp: %.1f C | Hum: %.1f %% | Light: %d %% | Motion: %s",
            current_data.temperature,
            current_data.humidity,
            current_data.lightLevel,
            current_data.motionDetected ? "YES" : "NO"
        );

        safe_log(log_buf);

        // Run the sensor task every 2 seconds.
        vTaskDelayUntil(&lastWakeTime, period);
    }
}