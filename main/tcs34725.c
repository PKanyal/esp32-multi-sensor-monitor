#include "tcs34725.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#define TCS_ADDR 0x29
#define CMD      0x80
#define AUTO_INC 0x20
#define ENABLE   0x00
#define ATIME    0x01
#define CONTROL  0x0F
#define ID       0x12
#define CDATAL   0x14

static i2c_master_dev_handle_t dev;

static esp_err_t write_reg(uint8_t reg, uint8_t val)
{
    uint8_t d[2] = { (uint8_t)(CMD | reg), val };
    return i2c_master_transmit(dev, d, sizeof(d), 100);
}
static esp_err_t read_regs(uint8_t reg, uint8_t *data, size_t n)
{
    uint8_t r = CMD | AUTO_INC | reg;
    return i2c_master_transmit_receive(dev, &r, 1, data, n, 100);
}
esp_err_t tcs34725_init(i2c_master_bus_handle_t bus)
{
    i2c_device_config_t cfg = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = TCS_ADDR,
        .scl_speed_hz = 100000
    };
    esp_err_t e = i2c_master_bus_add_device(bus, &cfg, &dev);
    if (e != ESP_OK) return e;
    uint8_t id = 0;
    e = read_regs(ID, &id, 1);
    if (e != ESP_OK) return e;
    if (id != 0x44 && id != 0x4D && id != 0x10) return ESP_ERR_NOT_FOUND;
    if ((e = write_reg(ATIME, 0xD5)) != ESP_OK) return e;
    if ((e = write_reg(CONTROL, 0x01)) != ESP_OK) return e; // 4x gain
    if ((e = write_reg(ENABLE, 0x03)) != ESP_OK) return e; // power + ADC
    vTaskDelay(pdMS_TO_TICKS(10));
    return ESP_OK;
}
esp_err_t tcs34725_read(tcs34725_data_t *out)
{
    uint8_t b[8];
    esp_err_t e = read_regs(CDATAL, b, sizeof(b));
    if (e != ESP_OK) return e;
    out->c = (uint16_t)b[1] << 8 | b[0];
    out->r = (uint16_t)b[3] << 8 | b[2];
    out->g = (uint16_t)b[5] << 8 | b[4];
    out->b = (uint16_t)b[7] << 8 | b[6];
    return ESP_OK;
}
