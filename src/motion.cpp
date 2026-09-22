#include "display.h"
#include "sensors.h"
#include "input.h"
#include "rtos_objects.h"
#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "rom/ets_sys.h"

#define I2C_SDA_PIN GPIO_NUM_21
#define I2C_SCL_PIN GPIO_NUM_22
#define SSD1306_ADDR 0x78 // 0x3C << 1

DisplayMode currentDisplayMode = MODE_TEMPERATURE;

// Standard C++ compliant 5x7 font lookup
static const uint8_t* get_glyph(char c) {
    static const uint8_t blank[5] = {0x00, 0x00, 0x00, 0x00, 0x00};
    static const uint8_t f_colon[5] = {0x00, 0x36, 0x36, 0x00, 0x00};
    static const uint8_t f_dot[5]   = {0x00, 0x60, 0x60, 0x00, 0x00};
    static const uint8_t f_pct[5]   = {0x23, 0x13, 0x08, 0x64, 0x62};
    static const uint8_t f_dash[5]  = {0x08, 0x08, 0x08, 0x08, 0x08};

    static const uint8_t digits[10][5] = {
        {0x3E, 0x51, 0x49, 0x45, 0x3E}, // 0
        {0x00, 0x42, 0x7F, 0x40, 0x00}, // 1
        {0x42, 0x61, 0x51, 0x49, 0x46}, // 2
        {0x21, 0x41, 0x45, 0x4B, 0x31}, // 3
        {0x18, 0x14, 0x12, 0x7F, 0x10}, // 4
        {0x27, 0x45, 0x45, 0x45, 0x39}, // 5
        {0x3C, 0x4A, 0x49, 0x49, 0x30}, // 6
        {0x01, 0x71, 0x09, 0x05, 0x03}, // 7
        {0x36, 0x49, 0x49, 0x49, 0x36}, // 8
        {0x06, 0x49, 0x49, 0x29, 0x1E}  // 9
    };

    static const uint8_t alpha[26][5] = {
        {0x7E, 0x11, 0x11, 0x11, 0x7E}, // A
        {0x7F, 0x49, 0x49, 0x49, 0x36}, // B
        {0x3E, 0x41, 0x41, 0x41, 0x22}, // C
        {0x7F, 0x41, 0x41, 0x22, 0x1C}, // D
        {0x7F, 0x49, 0x49, 0x49, 0x41}, // E
        {0x7F, 0x09, 0x09, 0x09, 0x01}, // F
        {0x3E, 0x41, 0x49, 0x49, 0x7A}, // G
        {0x7F, 0x08, 0x08, 0x08, 0x7F}, // H
        {0x00, 0x41, 0x7F, 0x41, 0x00}, // I
        {0x20, 0x40, 0x41, 0x3F, 0x01}, // J
        {0x7F, 0x08, 0x14, 0x22, 0x41}, // K
        {0x7F, 0x40, 0x40, 0x40, 0x40}, // L
        {0x7F, 0x02, 0x0C, 0x02, 0x7F}, // M
        {0x7F, 0x04, 0x08, 0x10, 0x7F}, // N
        {0x3E, 0x41, 0x41, 0x41, 0x3E}, // O
        {0x7F, 0x09, 0x09, 0x09, 0x06}, // P
        {0x3E, 0x41, 0x51, 0x21, 0x5E}, // Q
        {0x7F, 0x09, 0x19, 0x29, 0x46}, // R
        {0x46, 0x49, 0x49, 0x49, 0x31}, // S
        {0x01, 0x01, 0x7F, 0x01, 0x01}, // T
        {0x3F, 0x40, 0x40, 0x40, 0x3F}, // U
        {0x1F, 0x20, 0x40, 0x20, 0x1F}, // V
        {0x7F, 0x20, 0x18, 0x20, 0x7F}, // W
        {0x63, 0x14, 0x08, 0x14, 0x63}, // X
        {0x07, 0x08, 0x70, 0x08, 0x07}, // Y
        {0x61, 0x51, 0x49, 0x45, 0x43}  // Z
    };

    if (c >= '0' && c <= '9') return digits[c - '0'];
    if (c >= 'A' && c <= 'Z') return alpha[c - 'A'];
    if (c >= 'a' && c <= 'z') return alpha[c - 'a'];
    if (c == ':') return f_colon;
    if (c == '.') return f_dot;
    if (c == '%') return f_pct;
    if (c == '-') return f_dash;
    return blank;
}

static void i2c_bus_init(void) {
    gpio_reset_pin(I2C_SDA_PIN);
    gpio_reset_pin(I2C_SCL_PIN);
    gpio_set_direction(I2C_SDA_PIN, GPIO_MODE_INPUT_OUTPUT_OD);
    gpio_set_direction(I2C_SCL_PIN, GPIO_MODE_INPUT_OUTPUT_OD);
    gpio_set_pull_mode(I2C_SDA_PIN, GPIO_PULLUP_ONLY);
    gpio_set_pull_mode(I2C_SCL_PIN, GPIO_PULLUP_ONLY);
    gpio_set_level(I2C_SDA_PIN, 1);
    gpio_set_level(I2C_SCL_PIN, 1);
}

static void i2c_bus_start(void) {
    gpio_set_level(I2C_SDA_PIN, 1);
    gpio_set_level(I2C_SCL_PIN, 1);
    ets_delay_us(4);
    gpio_set_level(I2C_SDA_PIN, 0);
    ets_delay_us(4);
    gpio_set_level(I2C_SCL_PIN, 0);
}

static void i2c_bus_stop(void) {
    gpio_set_level(I2C_SDA_PIN, 0);
    gpio_set_level(I2C_SCL_PIN, 1);
    ets_delay_us(4);
    gpio_set_level(I2C_SDA_PIN, 1);
    ets_delay_us(4);
}

static void i2c_bus_write_byte(uint8_t byte) {
    for (int i = 0; i < 8; i++) {
        gpio_set_level(I2C_SDA_PIN, (byte & 0x80) ? 1 : 0);
        ets_delay_us(2);
        gpio_set_level(I2C_SCL_PIN, 1);
        ets_delay_us(4);
        gpio_set_level(I2C_SCL_PIN, 0);
        byte <<= 1;
    }
    gpio_set_level(I2C_SDA_PIN, 1);
    ets_delay_us(2);
    gpio_set_level(I2C_SCL_PIN, 1);
    ets_delay_us(4);
    gpio_set_level(I2C_SCL_PIN, 0);
}

static void oled_write_command(uint8_t cmd) {
    i2c_bus_start();
    i2c_bus_write_byte(SSD1306_ADDR);
    i2c_bus_write_byte(0x00);
    i2c_bus_write_byte(cmd);
    i2c_bus_stop();
}

static void oled_init(void) {
    i2c_bus_init();
    oled_write_command(0xAE);
    oled_write_command(0xD5);
    oled_write_command(0x80);
    oled_write_command(0xA8);
    oled_write_command(0x3F);
    oled_write_command(0xD3);
    oled_write_command(0x00);
    oled_write_command(0x40);
    oled_write_command(0x8D);
    oled_write_command(0x14);
    oled_write_command(0x20);
    oled_write_command(0x00);
    oled_write_command(0xA1);
    oled_write_command(0xC8);
    oled_write_command(0xDA);
    oled_write_command(0x12);
    oled_write_command(0x81);
    oled_write_command(0xCF);
    oled_write_command(0xD9);
    oled_write_command(0xF1);
    oled_write_command(0xDB);
    oled_write_command(0x40);
    oled_write_command(0xA4);
    oled_write_command(0xA6);
    oled_write_command(0xAF);
}

static void oled_render_text(const char *line1, const char *line2, const char *line3) {
    uint8_t buffer[1024];
    memset(buffer, 0, sizeof(buffer));

    const char *lines[3] = {line1, line2, line3};
    const int pages[3] = {1, 3, 5};

    for (int l = 0; l < 3; l++) {
        if (!lines[l]) continue;
        int len = strlen(lines[l]);
        int start_x = 8;
        for (int i = 0; i < len && (start_x + 6) < 128; i++) {
            const uint8_t *glyph = get_glyph(lines[l][i]);
            for (int col = 0; col < 5; col++) {
                buffer[pages[l] * 128 + start_x + col] = glyph[col];
            }
            start_x += 6;
        }
    }

    oled_write_command(0x21);
    oled_write_command(0);
    oled_write_command(127);
    oled_write_command(0x22);
    oled_write_command(0);
    oled_write_command(7);

    for (int i = 0; i < 1024; i += 16) {
        i2c_bus_start();
        i2c_bus_write_byte(SSD1306_ADDR);
        i2c_bus_write_byte(0x40);
        for (int j = 0; j < 16; j++) {
            i2c_bus_write_byte(buffer[i + j]);
        }
        i2c_bus_stop();
    }
}

static void oled_clear(void) {
    oled_write_command(0x21);
    oled_write_command(0);
    oled_write_command(127);
    oled_write_command(0x22);
    oled_write_command(0);
    oled_write_command(7);

    for (int i = 0; i < 1024; i += 16) {
        i2c_bus_start();
        i2c_bus_write_byte(SSD1306_ADDR);
        i2c_bus_write_byte(0x40);
        for (int j = 0; j < 16; j++) {
            i2c_bus_write_byte(0x00);
        }
        i2c_bus_stop();
    }
}

void display_task(void *pvParameters) {
    oled_init();
    oled_clear();

    SensorData data;
    memset(&data, 0, sizeof(SensorData));
    char line1[32], line2[32], line3[32];
    char log_buf[160]; 
    bool is_oled_active = true;

    for (;;) {
        if (sensorQueue != NULL) {
            xQueueReceive(sensorQueue, &data, 0);
        }

        int new_mode = 0;
        if (modeQueue != NULL && xQueueReceive(modeQueue, &new_mode, 0) == pdTRUE) {
            currentDisplayMode = (DisplayMode)new_mode;
        }

        bool active_state = true;
        if (systemEvents != NULL) {
            EventBits_t bits = xEventGroupGetBits(systemEvents);
            active_state = (bits & EVENT_ACTIVE) != 0;
        }

        if (active_state) {
            if (!is_oled_active) {
                oled_write_command(0xAF);
                is_oled_active = true;
            }

            snprintf(line1, sizeof(line1), "ROOM MONITOR");

            switch (currentDisplayMode) {
                case MODE_TEMPERATURE:
                    snprintf(line2, sizeof(line2), "TEMPERATURE");
                    snprintf(line3, sizeof(line3), "%.1f C", data.temperature);
                    break;
                case MODE_HUMIDITY:
                    snprintf(line2, sizeof(line2), "HUMIDITY");
                    snprintf(line3, sizeof(line3), "%.1f %%", data.humidity);
                    break;
                case MODE_LIGHT:
                    snprintf(line2, sizeof(line2), "AMBIENT LIGHT");
                    snprintf(line3, sizeof(line3), "%d %%", data.lightLevel);
                    break;
                case MODE_MOTION:
                    snprintf(line2, sizeof(line2), "MOTION SENSOR");
                    snprintf(line3, sizeof(line3), "%s", data.motionDetected ? "DETECTED" : "CLEAR");
                    break;
                default:
                    snprintf(line2, sizeof(line2), "TEMPERATURE");
                    snprintf(line3, sizeof(line3), "%.1f C", data.temperature);
                    break;
            }

            oled_render_text(line1, line2, line3);

            snprintf(log_buf, sizeof(log_buf), "[OLED DISPLAY] %s | %s: %s", line1, line2, line3);
            safe_log(log_buf);
        } else {
            if (is_oled_active) {
                oled_clear();
                oled_write_command(0xAE);
                is_oled_active = false;
                safe_log("[DisplayTask] OLED turned OFF (System INACTIVE)");
            }
        }

        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}