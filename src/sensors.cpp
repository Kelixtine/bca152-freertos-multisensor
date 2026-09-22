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

bool read_dht22(float *temp, float *humidity)
{
    uint8_t data[5] = {0};

    gpio_set_direction(DHT_PIN, GPIO_MODE_OUTPUT);
    gpio_set_level(DHT_PIN, 0);
    esp_rom_delay_us(1100);

    gpio_set_level(DHT_PIN, 1);
    esp_rom_delay_us(30);

    gpio_set_direction(DHT_PIN, GPIO_MODE_INPUT);

    int timeout = 0;

    while (gpio_get_level(DHT_PIN))
    {
        if (++timeout > 200) return false;
        esp_rom_delay_us(1);
    }

    timeout = 0;
    while (!gpio_get_level(DHT_PIN))
    {
        if (++timeout > 200) return false;
        esp_rom_delay_us(1);
    }

    timeout = 0;
    while (gpio_get_level(DHT_PIN))
    {
        if (++timeout > 200) return false;
        esp_rom_delay_us(1);
    }

    for (int i = 0; i < 40; i++)
    {
        timeout = 0;
        while (!gpio_get_level(DHT_PIN))
        {
            if (++timeout > 200) return false;
            esp_rom_delay_us(1);
        }

        int64_t t = esp_timer_get_time();

        timeout = 0;
        while (gpio_get_level(DHT_PIN))
        {
            if (++timeout > 200) return false;
            esp_rom_delay_us(1);
        }

        if ((esp_timer_get_time() - t) > 40)
            data[i / 8] |= (1 << (7 - (i % 8)));
    }

    if (data[4] != ((data[0] + data[1] + data[2] + data[3]) & 0xFF))
        return false;

    *humidity = ((data[0] << 8) | data[1]) * 0.1f;
    *temp = (((data[2] & 0x7F) << 8) | data[3]) * 0.1f;

    if (data[2] & 0x80)
        *temp *= -1;

    return true;
}

void vSensorTask(void *pvParameters)
{
    vTaskDelay(pdMS_TO_TICKS(100));

    TickType_t lastWake = xTaskGetTickCount();

    SensorData data = {24.0f, 40.0f, 50, false};

    adc_oneshot_unit_handle_t adc = NULL;

    adc_oneshot_unit_init_cfg_t init = {};
    init.unit_id = ADC_UNIT_1;
    adc_oneshot_new_unit(&init, &adc);

    adc_oneshot_chan_cfg_t cfg = {};
    cfg.atten = ADC_ATTEN_DB_12;
    cfg.bitwidth = ADC_BITWIDTH_DEFAULT;

    adc_oneshot_config_channel(adc, ADC_CHANNEL_6, &cfg);

    while (1)
    {
        float t, h;

        if (read_dht22(&t, &h))
        {
            data.temperature = t;
            data.humidity = h;

            // Wake OLED whenever user changes DHT values
            g_systemState = SystemState::ACTIVE;
            xEventGroupSetBits(g_systemEvents, EVENT_ACTIVE);

            if (evaluateTemperature(t) == AlarmState::NORMAL)
                xEventGroupClearBits(g_systemEvents, EVENT_ALARM);
            else
                xEventGroupSetBits(g_systemEvents, EVENT_ALARM);
        }

        int raw = 0;
        adc_oneshot_read(adc, ADC_CHANNEL_6, &raw);

        data.lightLevel = (raw * 100) / 4095;

        xQueueSend(displayQueue, &data, 0);

        if (xSemaphoreTake(serialMutex, portMAX_DELAY))
        {
            printf("[SensorTask] Temp: %.1f C | Hum: %.1f %% | Light: %d %%\n",
                   data.temperature,
                   data.humidity,
                   data.lightLevel);

            xSemaphoreGive(serialMutex);
        }

        vTaskDelayUntil(&lastWake, pdMS_TO_TICKS(2000));
    }
}