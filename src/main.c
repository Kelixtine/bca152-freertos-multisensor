#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_adc/adc_oneshot.h"

adc_oneshot_unit_handle_t adc_handle;

void ldrTask(void *pv)
{
    int light;

    while (1)
    {
        adc_oneshot_read(adc_handle, ADC_CHANNEL_6, &light);
        printf("[LDR] %d\n", light);
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

void dhtTask(void *pv)
{
    while (1)
    {
        // Placeholder until the queue stage
        printf("[DHT22] Temp: 24.0 C  Humidity: 40.0 %%\n");
        vTaskDelay(pdMS_TO_TICKS(2000));
    }
}

void app_main(void)
{
    printf("Multi-Sensor System Started\n");

    adc_oneshot_unit_init_cfg_t init_cfg = {
        .unit_id = ADC_UNIT_1
    };
    adc_oneshot_new_unit(&init_cfg, &adc_handle);

    adc_oneshot_chan_cfg_t chan_cfg = {
        .bitwidth = ADC_BITWIDTH_DEFAULT,
        .atten = ADC_ATTEN_DB_12
    };
    adc_oneshot_config_channel(adc_handle, ADC_CHANNEL_6, &chan_cfg);

    xTaskCreate(ldrTask, "LDR", 4096, NULL, 1, NULL);
    xTaskCreate(dhtTask, "DHT", 4096, NULL, 1, NULL);
}