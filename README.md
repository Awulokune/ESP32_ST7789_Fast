# ESP32_ST7789_Fast

Fast ST7789 SPI display library for ESP32.

## Installation

Arduino IDE → Library Manager → search for:

ESP32_ST7789_Fast

Or download this repository as ZIP.

## Basic Example

```cpp
#include <ESP32_ST7789_Fast.h>

#define TFT_DC  2
#define TFT_RST 4
#define TFT_CS  -1
#define TFT_SCLK 18
#define TFT_MOSI 23
#define TFT_BL  15

ESP32_ST7789_Fast tft(TFT_DC, TFT_RST, TFT_CS, TFT_SCLK, TFT_MOSI);

void setup() {
    pinMode(TFT_BL, OUTPUT);
    digitalWrite(TFT_BL, HIGH);

    tft.init();
    tft.setRotation(0);
    tft.fillScreen(BLACK);

    tft.setTextColor(WHITE);
    tft.setTextSize(2);
    tft.setCursor(20, 20);
    tft.println("AWULOKUNE");
}

void loop() {
}
