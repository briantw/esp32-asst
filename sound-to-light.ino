#include <Adafruit_NeoPixel.h>

#define MIC_PIN   4     // ADC input
#define LED_PIN   48    // WS2812 LED pin
#define NUM_LEDS  1

Adafruit_NeoPixel led(NUM_LEDS, LED_PIN, NEO_GRB + NEO_KHZ800);

void setup() {
  led.begin();
  led.show();
  analogReadResolution(12);             // 12-bit ADC
  analogSetPinAttenuation(MIC_PIN, ADC_11db);
}

void loop() {
  int level = analogRead(MIC_PIN);      // 0–4095
  int brightness = map(level, 0, 4095, 0, 255);
  led.setBrightness(brightness);
  led.setPixelColor(0, led.Color(255, 160, 50));  // warm orange
  led.show();
}
