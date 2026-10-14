#include <stdio.h>
#include <stdbool.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/i2c_master.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "dht11.h"
#include "tcs34725.h"
#include "ssd1306.h"

#define SDA_GPIO 21
#define SCL_GPIO 22
#define DHT_GPIO 4

static const char *TAG = "monitor";
static QueueHandle_t q;
static i2c_master_bus_handle_t bus;

typedef enum { MSG_DHT, MSG_COLOR } msg_type_t;
typedef struct {
    msg_type_t type;
    bool ok;
    int64_t time_ms;
    union {
        struct { float t, h; } dht;
        tcs34725_data_t color;
    } v;
} sensor_msg_t;

typedef struct {
    bool dht_ok, color_ok;
    float temp, hum;
    tcs34725_data_t color;
    int64_t dht_ms, color_ms;
} state_t;

static portMUX_TYPE mux = portMUX_INITIALIZER_UNLOCKED;
static state_t state;

static void dht_task(void *arg)
{
    (void)arg;
    TickType_t wake=xTaskGetTickCount();
    while(1) {
        sensor_msg_t m={.type=MSG_DHT,.time_ms=esp_timer_get_time()/1000};
        m.ok=(dht11_read(DHT_GPIO,&m.v.dht.t,&m.v.dht.h)==ESP_OK);
        if(xQueueSend(q,&m,0)!=pdPASS) ESP_LOGW(TAG,"Queue full: DHT sample dropped");
        vTaskDelayUntil(&wake,pdMS_TO_TICKS(2000));
    }
}
static void color_task(void *arg)
{
    (void)arg;
    TickType_t wake=xTaskGetTickCount();
    while(1) {
        sensor_msg_t m={.type=MSG_COLOR,.time_ms=esp_timer_get_time()/1000};
        m.ok=(tcs34725_read(&m.v.color)==ESP_OK);
        if(xQueueSend(q,&m,0)!=pdPASS) ESP_LOGW(TAG,"Queue full: color sample dropped");
        vTaskDelayUntil(&wake,pdMS_TO_TICKS(500));
    }
}
static void data_task(void *arg)
{
    (void)arg;
    sensor_msg_t m;
    while(1) if(xQueueReceive(q,&m,portMAX_DELAY)==pdPASS) {
        portENTER_CRITICAL(&mux);
        if(m.type==MSG_DHT) {
            state.dht_ok=m.ok; state.dht_ms=m.time_ms;
            if(m.ok){state.temp=m.v.dht.t;state.hum=m.v.dht.h;}
        } else {
            state.color_ok=m.ok; state.color_ms=m.time_ms;
            if(m.ok) state.color=m.v.color;
        }
        portEXIT_CRITICAL(&mux);
    }
}
static void display_task(void *arg)
{
    (void)arg;
    char l1[22],l2[22],l3[22],l4[22];
    while(1) {
        state_t s;
        portENTER_CRITICAL(&mux); s=state; portEXIT_CRITICAL(&mux);
        if(s.dht_ok && esp_timer_get_time()/1000-s.dht_ms<5000)
            snprintf(l1,sizeof(l1),"T %.1fC H %.1f%%",s.temp,s.hum);
        else snprintf(l1,sizeof(l1),"DHT ERROR/STALE");
        if(s.color_ok && esp_timer_get_time()/1000-s.color_ms<3000)
            snprintf(l2,sizeof(l2),"R%u G%u",s.color.r,s.color.g);
        else snprintf(l2,sizeof(l2),"COLOR ERROR/STALE");
        if(s.color_ok && esp_timer_get_time()/1000-s.color_ms<3000)
            snprintf(l3,sizeof(l3),"B%u C%u",s.color.b,s.color.c);
        else snprintf(l3,sizeof(l3),"RGB DATA N/A");
        snprintf(l4,sizeof(l4),"ESP32 MONITOR");
        ssd1306_show(l1,l2,l3,l4);
        vTaskDelay(pdMS_TO_TICKS(500));
    }
}
static esp_err_t init_i2c(void)
{
    i2c_master_bus_config_t cfg={
        .i2c_port=I2C_NUM_0,.sda_io_num=SDA_GPIO,.scl_io_num=SCL_GPIO,
        .clk_source=I2C_CLK_SRC_DEFAULT,.glitch_ignore_cnt=7,
        .flags.enable_internal_pullup=true
    };
    return i2c_new_master_bus(&cfg,&bus);
}
void app_main(void)
{
    ESP_LOGI(TAG,"Starting sensor monitor");
    q=xQueueCreate(8,sizeof(sensor_msg_t));
    if(!q){ESP_LOGE(TAG,"Queue create failed");return;}
    ESP_ERROR_CHECK(init_i2c());
    esp_err_t oled=ssd1306_init(bus);
    if(oled!=ESP_OK) ESP_LOGE(TAG,"OLED init failed: %s",esp_err_to_name(oled));
    esp_err_t color=tcs34725_init(bus);
    if(color!=ESP_OK) ESP_LOGE(TAG,"TCS34725 init failed: %s",esp_err_to_name(color));
    gpio_set_direction(DHT_GPIO,GPIO_MODE_INPUT_OUTPUT_OD);
    gpio_set_level(DHT_GPIO,1);
    xTaskCreate(data_task,"sensor_data",3072,NULL,4,NULL);
    xTaskCreate(dht_task,"dht11",3072,NULL,3,NULL);
    xTaskCreate(color_task,"color",3072,NULL,3,NULL);
    xTaskCreate(display_task,"display",4096,NULL,2,NULL);
}
