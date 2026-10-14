#pragma once
#include "esp_err.h"
#include "driver/i2c_master.h"
esp_err_t ssd1306_init(i2c_master_bus_handle_t bus);
void ssd1306_show(const char *line1, const char *line2, const char *line3, const char *line4);
