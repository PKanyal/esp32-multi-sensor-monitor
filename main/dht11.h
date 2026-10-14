#pragma once
#include "esp_err.h"
esp_err_t dht11_read(int gpio_num, float *temperature_c, float *humidity_pct);
