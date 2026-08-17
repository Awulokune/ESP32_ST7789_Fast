# ESP32_ST7789_Fast

ESP32 port of **Arduino_ST7789_Fast** for ST7789 SPI displays. AVR-specific SPI registers and inline assembly were replaced with the ESP32 Arduino SPI API while preserving the original API and RGB565 drawing behavior.

## Default configuration
- MOSI: GPIO 23
- SCLK: GPIO 18
- DC: GPIO 2
- RST: GPIO 4
- CS: -1
- Backlight: GPIO 15 (controlled by sketch)
- SPI mode: MODE3
- SPI clock: 27 MHz

## Example
```cpp
#include <ESP32_ST7789_Fast.h>
ESP32_ST7789_Fast tft(2, 4, -1, 18, 23);
void setup() {
  pinMode(15, OUTPUT);
  digitalWrite(15, HIGH);
  tft.init();
  tft.fillScreen(BLACK);
  tft.fillRect(20, 20, 100, 50, BLUE);
}
void loop() {}
```

Adafruit_GFX remains a dependency for the inherited text/graphics API.
