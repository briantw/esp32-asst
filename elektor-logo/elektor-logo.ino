#include <Adafruit_GFX.h>
#include <Adafruit_GC9A01A.h>
#include <SPI.h>

#include "elektor_logo.h"

// Define your display's pins
#define TFT_SCK  12 // Example, adjust to your ESP32's SCK pin
#define TFT_MOSI 11 // Example, adjust to your ESP32's MOSI pin
#define TFT_RST  8  // Example, adjust to your ESP32's RST pin
#define TFT_DC   9  // Example, adjust to your ESP32's DC pin
#define TFT_CS   10  // Example, adjust to your ESP32's CS pin

Adafruit_GC9A01A tft = Adafruit_GC9A01A(TFT_CS, TFT_DC, TFT_MOSI, TFT_SCK, TFT_RST);

void setup() {
  Serial.begin(115200);
  tft.begin();
  tft.setRotation(0); // Adjust rotation as needed (0, 1, 2, or 3)
  tft.fillScreen(GC9A01A_BLACK); // Clear the screen
}

void loop() {
  // Option 1: Drawing from a C-array (recommended for performance)
  tft.drawRGBBitmap(0, 0, elektor_logo, 240, 240); // (x, y, bitmap_data, width, height)

  // Option 2: Drawing from a raw BMP file (requires SD card or SPIFFS and more RAM)
  // This approach is more complex and requires additional libraries for file system access.
  // Example: Using an SD card with the Adafruit_ImageReader library.
  // tft.drawImage(filename, x, y);

  delay(5000); // Display for 5 seconds
}
