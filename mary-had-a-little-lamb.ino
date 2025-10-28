// ESP32-S3 I2S melody test (sine tones -> I2S amp)
// Pins: BCLK=GPIO5, LRCLK/WSEL=GPIO6, DIN=GPIO7
// Change pins below if your wiring differs.

#include <Arduino.h>
#include "driver/i2s.h"
#include <math.h>

static const int PIN_BCLK = 5;
static const int PIN_LRCK = 6;   // also called WS
static const int PIN_DOUT = 7;   // DIN on the amplifier

// Audio format
static const int SAMPLE_RATE   = 22050;
static const int BITS_PER_SAMP = I2S_BITS_PER_SAMPLE_16BIT;
static const i2s_port_t I2S_PORT = I2S_NUM_0;

// Simple tune (Mary Had a Little Lamb – first phrases)
struct Note { float freq; int ms; };
#define _ 0.0f
Note melody[] = {
  {329.63f, 300}, {293.66f, 300}, {261.63f, 300}, {293.66f, 300},
  {329.63f, 300}, {329.63f, 300}, {329.63f, 600},
  {293.66f, 300}, {293.66f, 300}, {293.66f, 600},
  {329.63f, 300}, {392.00f, 300}, {392.00f, 600},
  {329.63f, 300}, {293.66f, 300}, {261.63f, 300}, {293.66f, 300},
  {329.63f, 300}, {329.63f, 300}, {329.63f, 300}, {329.63f, 300},
  {293.66f, 300}, {293.66f, 300}, {329.63f, 300}, {293.66f, 300},
  {261.63f, 800}
};

void i2sInit() {
  i2s_config_t cfg = {
    .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_TX),
    .sample_rate = SAMPLE_RATE,
    .bits_per_sample = (i2s_bits_per_sample_t)BITS_PER_SAMP,
    .channel_format = I2S_CHANNEL_FMT_RIGHT_LEFT, // stereo frames
    .communication_format = I2S_COMM_FORMAT_STAND_I2S,
    .intr_alloc_flags = 0,
    .dma_buf_count = 8,
    .dma_buf_len = 256,
    .use_apll = false,
    .tx_desc_auto_clear = true,
    .fixed_mclk = 0
  };

  i2s_pin_config_t pins = {
    .bck_io_num = PIN_BCLK,
    .ws_io_num = PIN_LRCK,
    .data_out_num = PIN_DOUT,
    .data_in_num = I2S_PIN_NO_CHANGE
  };

  i2s_driver_install(I2S_PORT, &cfg, 0, nullptr);
  i2s_set_pin(I2S_PORT, &pins);
  // Mono content duplicated to both channels:
  i2s_set_clk(I2S_PORT, SAMPLE_RATE, (i2s_bits_per_sample_t)BITS_PER_SAMP, I2S_CHANNEL_STEREO);
}

void playTone(float freq, int duration_ms, float volume = 0.2f) {
  // volume: 0.0..1.0 (keep modest to avoid clipping/distortion)
  if (freq <= 0.0f) {
    // Rest: send zeros
    const int samples = (SAMPLE_RATE * duration_ms) / 1000;
    const int frames = samples;
    const int bytes = frames * 2 /*channels*/ * sizeof(int16_t);
    static int16_t zeroBuf[512 * 2]; // stereo zeros
    size_t written = 0;
    int remaining = bytes;
    while (remaining > 0) {
      int chunk = min(remaining, (int)sizeof(zeroBuf));
      i2s_write(I2S_PORT, zeroBuf, chunk, &written, portMAX_DELAY);
      remaining -= written;
    }
    return;
  }

  const float twoPiOverFs = 2.0f * (float)M_PI / (float)SAMPLE_RATE;
  const int totalSamples  = (SAMPLE_RATE * duration_ms) / 1000;
  const int chunkSamples  = 256; // per channel
  int16_t buf[chunkSamples * 2]; // interleaved stereo

  float phase = 0.0f;
  const float phaseInc = twoPiOverFs * freq;
  const int16_t peak = (int16_t)(volume * 32767.0f);

  int generated = 0;
  while (generated < totalSamples) {
    int n = min(chunkSamples, totalSamples - generated);
    for (int i = 0; i < n; ++i) {
      int16_t s = (int16_t)(sinf(phase) * peak);
      phase += phaseInc;
      if (phase > 2.0f * (float)M_PI) phase -= 2.0f * (float)M_PI;
      // stereo duplicate: L then R
      buf[2 * i + 0] = s;
      buf[2 * i + 1] = s;
    }
    size_t written = 0;
    i2s_write(I2S_PORT, buf, n * 2 * sizeof(int16_t), &written, portMAX_DELAY);
    generated += n;
  }
}

void setup() {
  i2sInit();
  // brief power-on rest
  playTone(0.0f, 100);
}

void loop() {
  for (auto &n : melody) {
    playTone(n.freq, n.ms, 0.25f);
    // tiny gap between notes to make articulation clearer
    playTone(0.0f, 20);
  }
  // pause before repeating
  playTone(0.0f, 800);
}
