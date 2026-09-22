#include "input.h"
#include "rtos_objects.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

void vInputTask(void *pvParameters) {
    vTaskDelay(pdMS_TO_TICKS(100));

    gpio_config_t io_conf = {};
    io_conf.pin_bit_mask = (1ULL << ENCODER_CLK) | (1ULL << ENCODER_DT) | (1ULL << ENCODER_SW);
    io_conf.mode = GPIO_MODE_INPUT;
    io_conf.pull_up_en = GPIO_PULLUP_ENABLE;
    io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
    io_conf.intr_type = GPIO_INTR_DISABLE;
    gpio_config(&io_conf);

    int lastClk = gpio_get_level(ENCODER_CLK);

    while (1) {
        int currentClk = gpio_get_level(ENCODER_CLK);
        if (lastClk == 1 && currentClk == 0) {
            NavDirection dir = (gpio_get_level(ENCODER_DT) != currentClk) 
                               ? NavDirection::NEXT 
                               : NavDirection::PREVIOUS;
            xQueueSend(navQueue, &dir, 0);
        }
        lastClk = currentClk;
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}