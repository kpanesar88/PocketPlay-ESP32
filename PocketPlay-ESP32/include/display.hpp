#ifndef DISPLAY_HPP
#define DISPLAY_HPP

#include <Arduino.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ST7735.h>
#include <SPI.h>

// TFT pins
#define TFT_CS    22
#define TFT_DC    21
#define TFT_RST   4
#define TFT_MOSI  23
#define TFT_SCLK  18

extern Adafruit_ST7735 tft;

void setupDisplay();

#endif