#include "dht11.h"
#include <stdint.h>
#include "driver/gpio.h"
#include "esp_rom_sys.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static int wait_level(int pin, int level, int timeout_us)
{
    while (gpio_get_level(pin) != level && timeout_us-- > 0)
        esp_rom_delay_us(1);
    return timeout_us > 0;
}

esp_err_t dht11_read(int pin, float *temp, float *hum)
{
    uint8_t b[5] = {0};

    gpio_set_direction(pin, GPIO_MODE_OUTPUT_OD);
    gpio_set_level(pin, 0);
    vTaskDelay(pdMS_TO_TICKS(20));
    gpio_set_level(pin, 1);
    esp_rom_delay_us(30);
    gpio_set_direction(pin, GPIO_MODE_INPUT);

    if (!wait_level(pin, 0, 100)) return ESP_ERR_TIMEOUT;
    if (!wait_level(pin, 1, 100)) return ESP_ERR_TIMEOUT;
    if (!wait_level(pin, 0, 100)) return ESP_ERR_TIMEOUT;

    for (int i = 0; i < 40; ++i) {
        if (!wait_level(pin, 1, 100)) return ESP_ERR_TIMEOUT;
        esp_rom_delay_us(35);
        int bit = gpio_get_level(pin);
        if (!wait_level(pin, 0, 100)) return ESP_ERR_TIMEOUT;
        b[i / 8] = (uint8_t)((b[i / 8] << 1) | (bit ? 1 : 0));
    }

    if ((uint8_t)(b[0] + b[1] + b[2] + b[3]) != b[4])
        return ESP_ERR_INVALID_CRC;

    *hum = (float)b[0] + (float)b[1] / 10.0f;
    *temp = (float)(b[2] & 0x7f) + (float)b[3] / 10.0f;
    if (b[2] & 0x80) *temp = -*temp;
    return ESP_OK;
}
