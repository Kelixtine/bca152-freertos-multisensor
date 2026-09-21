#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_adc/adc_oneshot.h"

adc_oneshot_unit_handle_t adc_handle;

void ldrTask(void *pv)
{
    int light = 0;

    while (1)
    {
        adc_oneshot_read(adc_handle, ADC_CHANNEL_6, &light); // GPIO34
        printf("[LDR] %d\n", light);
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

void dhtTask(void *pv)
{
    while (1)
    {
        printf("[DHT22] Waiting for sensor...\n");
        vTaskDelay(pdMS_TO_TICKS(2000));
    }
}

void app_main(void)
{
    printf("Multi-Sensor System Started\n");

    adc_oneshot_unit_init_cfg_t init_config = {
        .unit_id = ADC_UNIT_1,
    };
    adc_oneshot_new_unit(&init_config, &adc_handle);

    adc_oneshot_chan_cfg_t config = {
        .bitwidth = ADC_BITWIDTH_DEFAULT,
        .atten = ADC_ATTEN_DB_12,
    };
    adc_oneshot_config_channel(adc_handle, ADC_CHANNEL_6, &config);

    xTaskCreate(ldrTask, "LDR", 2048, NULL, 1, NULL);
    xTaskCreate(dhtTask, "DHT", 2048, NULL, 1, NULL);
}