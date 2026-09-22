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

            gLastActivityTick = xTaskGetTickCount();
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