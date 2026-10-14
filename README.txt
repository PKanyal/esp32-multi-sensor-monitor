ESP-IDF Real-Time Multi-Sensor Monitor
======================================

Hardware assumed:
- Classic ESP32 (ESP32-WROOM DevKit)
- DHT11 on GPIO4 (use a 4.7k-10k pull-up to 3.3V if bare sensor)
- TCS34725 at I2C address 0x29
- SSD1306 128x64 I2C OLED at address 0x3C
- SDA GPIO21, SCL GPIO22; I2C devices share these lines

IMPORTANT:
- Check your modules' voltage compatibility. ESP32 GPIO is 3.3V.
- DHT11 timing can be sensitive to task scheduling. If readings fail often,
  use a tested ESP-IDF DHT component/driver or a dedicated RMT-based driver.
- This simple OLED font supports uppercase letters, digits, spaces and hyphens.
- TCS34725 initialization accepts common IDs 0x44, 0x4D, 0x10.
- Code uses the ESP-IDF 5.x I2C master API.

Build:
1. Install ESP-IDF 5.x and open its ESP-IDF terminal.
2. Extract this folder.
3. cd sensor_monitor
4. idf.py set-target esp32
5. idf.py build
6. Connect board and run: idf.py -p COM5 flash monitor
   Replace COM5 with your port (Linux example: /dev/ttyUSB0).

If build says an I2C API is missing, your ESP-IDF is likely older than 5.2.
Use ESP-IDF 5.2 or newer for this project.
