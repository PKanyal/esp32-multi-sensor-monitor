
idf_component_register(
    SRCS "main.c" "dht11.c" "tcs34725.c" "ssd1306.c"
    INCLUDE_DIRS "."
    REQUIRES
        esp_driver_gpio
        esp_driver_i2c
        esp_timer
)
