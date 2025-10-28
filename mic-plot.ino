// Audio waveform plotter via Serial Plotter
// Connect electret mic module output to GPIO 4 (ADC input)
// Ensure the mic output never exceeds 3.3 V!

const int MIC_PIN = 4;
const int SAMPLE_DELAY_US = 125;  // ~8 kHz sampling (1 s / 8000 Hz = 125 µs)

void setup() {
  Serial.begin(115200);
  analogReadResolution(12);  // ESP32 ADC: 12-bit (0–4095)
  analogSetPinAttenuation(MIC_PIN, ADC_11db);  // for 0–3.3 V range
}

void loop() {
  int value = analogRead(MIC_PIN);
  Serial.println(value);
  delayMicroseconds(SAMPLE_DELAY_US);
}
