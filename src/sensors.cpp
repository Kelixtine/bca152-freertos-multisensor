#include "sensors.h"
#include "rtos_objects.h"
#include "system_state.h"
#include "alarm.h" 
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h" 
#include "driver/gpio.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_timer.h"
#include "esp_rom_sys.h"

bool read_dht22(float *temp, float *humidity) {
    uint8_t data[5] = {0};
    gpio_set_direction(DHT_PIN, GPIO_MODE_OUTPUT);
    gpio_set_level(DHT_PIN, 0);
    esp_rom_delay_us(1100);
    gpio_set_level(DHT_PIN, 1);
    esp_rom_delay_us(30);
    gpio_set_direction(DHT_PIN, GPIO_MODE_INPUT);

    int timeout = 0;
    while (gpio_get_level(DHT_PIN) == 1) { if (++timeout > 200) return false; esp_rom_delay_us(1); }
    timeout = 0;
    while (gpio_get_level(DHT_PIN) == 0) { if (++timeout > 200) return false; esp_rom_delay_us(1); }
    timeout = 0;
    while (gpio_get_level(DHT_PIN) == 1) { if (++timeout > 200) return false; esp_rom_delay_us(1); }

    for (int i = 0; i < 40; i++) {
        timeout = 0;
        while (gpio_get_level(DHT_PIN) == 0) { if (++timeout > 200) return false; esp_rom_delay_us(1); }
        int64_t t = esp_timer_get_time();
        timeout = 0;
        while (gpio_get_level(DHT_PIN) == 1) { if (++timeout > 200) return false; esp_rom_delay_us(1); }
        if ((esp_timer_get_time() - t) > 40) {
            data[i / 8] |= (1 << (7 - (i % 8)));
        }
    }

    if (data[4] == ((data[0] + data[1] + data[2] + data[3]) & 0xFF)) {
        *humidity = ((data[0] << 8) | data[1]) * 0.1f;
        *temp = (((data[2] & 0x7F) << 8) | data[3]) * 0.1f;
        if (data[2] & 0x80) *temp *= -1.0f;
        return true;
    }
    return false;
}

void vSensorTask(void *pvParameters) {
    vTaskDelay(pdMS_TO_TICKS(100));

    TickType_t xLastWakeTime = xTaskGetTickCount();
    SensorData data = {24.0f, 55.0f, 50, false};

    adc_oneshot_unit_handle_t adc1_handle = NULL;
    adc_oneshot_unit_init_cfg_t init_config1 = {};
    init_config1.unit_id = ADC_UNIT_1;
    
    if (adc_oneshot_new_unit(&init_config1, &adc1_handle) == ESP_OK) {
        adc_oneshot_chan_cfg_t config = {};
        config.atten = ADC_ATTEN_DB_12;
        config.bitwidth = ADC_BITWIDTH_DEFAULT;
        adc_oneshot_config_channel(adc1_handle, ADC_CHANNEL_6, &config);
    }

    while (1) {
        float t, h;
        if (read_dht22(&t, &h)) {
            data.temperature = t;
            data.humidity = h;

            if (evaluateTemperature(t) != AlarmState::NORMAL) {
                xEventGroupSetBits(g_systemEvents, EVENT_ALARM);
            } else {
                xEventGroupClearBits(g_systemEvents, EVENT_ALARM);
            }
        }

        if (adc1_handle != NULL) {
            int raw_adc = 0;
            if (adc_oneshot_read(adc1_handle, ADC_CHANNEL_6, &raw_adc) == ESP_OK) {
                data.lightLevel = (int)((raw_adc / 4095.0f) * 100.0f);
            }
        }

        if (xSemaphoreTake(serialMutex, portMAX_DELAY) == pdTRUE) {
            printf("[SensorTask] Temp: %.1f C | Hum: %.1f %% | Light: %d %%\n", data.temperature, data.humidity, data.lightLevel);
            xSemaphoreGive(serialMutex);
        }

        xQueueSend(displayQueue, &data, 0);
        vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(2000));
    }
}