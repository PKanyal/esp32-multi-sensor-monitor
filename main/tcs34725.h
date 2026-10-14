#pragma once
#include <stdint.h>
#include "esp_err.h"
#include "driver/i2c_master.h"
typedef struct { uint16_t r, g, b, c; } tcs34725_data_t;
esp_err_t tcs34725_init(i2c_master_bus_handle_t bus);
esp_err_t tcs34725_read(tcs34725_data_t *out);
