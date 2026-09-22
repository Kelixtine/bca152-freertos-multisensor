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
#define SSD1306_ADDR 0x78

DisplayMode currentDisplayMode = MODE_TEMPERATURE;

// ================= FONT =================

static const uint8_t* get_glyph(char c) {
    static const uint8_t blank[5]={0,0,0,0,0};
    static const uint8_t colon[5]={0x00,0x36,0x36,0x00,0x00};
    static const uint8_t dot[5]={0x00,0x60,0x60,0x00,0x00};
    static const uint8_t pct[5]={0x23,0x13,0x08,0x64,0x62};
    static const uint8_t dash[5]={0x08,0x08,0x08,0x08,0x08};

    static const uint8_t digits[10][5]={
        {0x3E,0x51,0x49,0x45,0x3E},
        {0x00,0x42,0x7F,0x40,0x00},
        {0x42,0x61,0x51,0x49,0x46},
        {0x21,0x41,0x45,0x4B,0x31},
        {0x18,0x14,0x12,0x7F,0x10},
        {0x27,0x45,0x45,0x45,0x39},
        {0x3C,0x4A,0x49,0x49,0x30},
        {0x01,0x71,0x09,0x05,0x03},
        {0x36,0x49,0x49,0x49,0x36},
        {0x06,0x49,0x49,0x29,0x1E}
    };

    static const uint8_t alpha[26][5]={
        {0x7E,0x11,0x11,0x11,0x7E},{0x7F,0x49,0x49,0x49,0x36},
        {0x3E,0x41,0x41,0x41,0x22},{0x7F,0x41,0x41,0x22,0x1C},
        {0x7F,0x49,0x49,0x49,0x41},{0x7F,0x09,0x09,0x09,0x01},
        {0x3E,0x41,0x49,0x49,0x7A},{0x7F,0x08,0x08,0x08,0x7F},
        {0x00,0x41,0x7F,0x41,0x00},{0x20,0x40,0x41,0x3F,0x01},
        {0x7F,0x08,0x14,0x22,0x41},{0x7F,0x40,0x40,0x40,0x40},
        {0x7F,0x02,0x0C,0x02,0x7F},{0x7F,0x04,0x08,0x10,0x7F},
        {0x3E,0x41,0x41,0x41,0x3E},{0x7F,0x09,0x09,0x09,0x06},
        {0x3E,0x41,0x51,0x21,0x5E},{0x7F,0x09,0x19,0x29,0x46},
        {0x46,0x49,0x49,0x49,0x31},{0x01,0x01,0x7F,0x01,0x01},
        {0x3F,0x40,0x40,0x40,0x3F},{0x1F,0x20,0x40,0x20,0x1F},
        {0x7F,0x20,0x18,0x20,0x7F},{0x63,0x14,0x08,0x14,0x63},
        {0x07,0x08,0x70,0x08,0x07},{0x61,0x51,0x49,0x45,0x43}
    };

    if(c>='0'&&c<='9') return digits[c-'0'];
    if(c>='A'&&c<='Z') return alpha[c-'A'];
    if(c>='a'&&c<='z') return alpha[c-'a'];
    if(c==':') return colon;
    if(c=='.') return dot;
    if(c=='%') return pct;
    if(c=='-') return dash;
    return blank;
}

// ================= I2C =================

static void i2c_bus_init(){
    gpio_reset_pin(I2C_SDA_PIN);
    gpio_reset_pin(I2C_SCL_PIN);
    gpio_set_direction(I2C_SDA_PIN,GPIO_MODE_INPUT_OUTPUT_OD);
    gpio_set_direction(I2C_SCL_PIN,GPIO_MODE_INPUT_OUTPUT_OD);
    gpio_set_pull_mode(I2C_SDA_PIN,GPIO_PULLUP_ONLY);
    gpio_set_pull_mode(I2C_SCL_PIN,GPIO_PULLUP_ONLY);
    gpio_set_level(I2C_SDA_PIN,1);
    gpio_set_level(I2C_SCL_PIN,1);
}

static void i2c_start(){
    gpio_set_level(I2C_SDA_PIN,1);
    gpio_set_level(I2C_SCL_PIN,1);
    ets_delay_us(4);
    gpio_set_level(I2C_SDA_PIN,0);
    ets_delay_us(4);
    gpio_set_level(I2C_SCL_PIN,0);
}

static void i2c_stop(){
    gpio_set_level(I2C_SDA_PIN,0);
    gpio_set_level(I2C_SCL_PIN,1);
    ets_delay_us(4);
    gpio_set_level(I2C_SDA_PIN,1);
    ets_delay_us(4);
}

static void i2c_write(uint8_t b){
    for(int i=0;i<8;i++){
        gpio_set_level(I2C_SDA_PIN,b&0x80);
        gpio_set_level(I2C_SCL_PIN,1);
        ets_delay_us(4);
        gpio_set_level(I2C_SCL_PIN,0);
        b<<=1;
    }
    gpio_set_level(I2C_SDA_PIN,1);
    gpio_set_level(I2C_SCL_PIN,1);
    ets_delay_us(4);
    gpio_set_level(I2C_SCL_PIN,0);
}

static void oled_cmd(uint8_t c){
    i2c_start();
    i2c_write(SSD1306_ADDR);
    i2c_write(0x00);
    i2c_write(c);
    i2c_stop();
}

static void oled_init(){
    i2c_bus_init();
    const uint8_t cmds[]={
        0xAE,0xD5,0x80,0xA8,0x3F,0xD3,0x00,0x40,
        0x8D,0x14,0x20,0x00,0xA1,0xC8,0xDA,0x12,
        0x81,0xCF,0xD9,0xF1,0xDB,0x40,0xA4,0xA6,0xAF
    };
    for(auto c:cmds) oled_cmd(c);
}

static void oled_clear(){
    oled_cmd(0x21); oled_cmd(0); oled_cmd(127);
    oled_cmd(0x22); oled_cmd(0); oled_cmd(7);

    for(int i=0;i<1024;i+=16){
        i2c_start();
        i2c_write(SSD1306_ADDR);
        i2c_write(0x40);
        for(int j=0;j<16;j++) i2c_write(0);
        i2c_stop();
    }
}

static void oled_render(const char*l1,const char*l2,const char*l3){
    uint8_t buf[1024]={0};
    const char*lines[3]={l1,l2,l3};
    const int pages[3]={1,3,5};

    for(int r=0;r<3;r++){
        int x=8;
        for(int i=0;i<strlen(lines[r]);i++){
            const uint8_t*g=get_glyph(lines[r][i]);
            for(int c=0;c<5;c++)
                buf[pages[r]*128+x+c]=g[c];
            x+=6;
        }
    }

    oled_cmd(0x21); oled_cmd(0); oled_cmd(127);
    oled_cmd(0x22); oled_cmd(0); oled_cmd(7);

    for(int i=0;i<1024;i+=16){
        i2c_start();
        i2c_write(SSD1306_ADDR);
        i2c_write(0x40);
        for(int j=0;j<16;j++) i2c_write(buf[i+j]);
        i2c_stop();
    }
}

// ================= DISPLAY TASK =================

void display_task(void *pvParameters)
{
    oled_init();
    oled_clear();

    SensorData data={};
    char line1[32],line2[32],line3[32];
    bool oledOn=true;

    for(;;)
    {
        if(sensorQueue)
            xQueueReceive(sensorQueue,&data,0);

        int mode=0;
        if(modeQueue &&
           xQueueReceive(modeQueue,&mode,0)==pdTRUE)
            currentDisplayMode=(DisplayMode)mode;

        bool active=true;
        if(systemEvents){
            EventBits_t bits=xEventGroupGetBits(systemEvents);
            active=(bits & EVENT_ACTIVE);
        }

        if(!active){
            if(oledOn){
                oled_clear();
                oled_cmd(0xAE);
                oledOn=false;
                safe_log("[DisplayTask] OLED OFF");
            }
            vTaskDelay(pdMS_TO_TICKS(100));
            continue;
        }

        if(!oledOn){
            oled_cmd(0xAF);
            oledOn=true;
        }

        strcpy(line1,"ROOM MONITOR");

        switch(currentDisplayMode){

            case MODE_TEMPERATURE:
                strcpy(line2,"Page: Temp");
                snprintf(line3,sizeof(line3),
                         "Val: %.1f C",data.temperature);
                break;

            case MODE_HUMIDITY:
                strcpy(line2,"Page: Humidity");
                snprintf(line3,sizeof(line3),
                         "Val: %.1f %%",data.humidity);
                break;

            case MODE_LIGHT:
                strcpy(line2,"Page: Light");
                snprintf(line3,sizeof(line3),
                         "Val: %d %%",data.lightLevel);
                break;

            case MODE_MOTION:
                strcpy(line2,"Page: Motion");
                snprintf(line3,sizeof(line3),
                         "Val: %s",
                         data.motionDetected?"DETECTED":"CLEAR");
                break;

            default:
                strcpy(line2,"Page: Temp");
                snprintf(line3,sizeof(line3),
                         "Val: %.1f C",data.temperature);
                break;
        }

        oled_render(line1,line2,line3);
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}