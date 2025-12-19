/* Hey Elektor Wake Word - Minimalist Proof of Concept
 * Focus: ADC Mic (GPIO 4) and RGB LED (GPIO 48)
 */

#include <Hey_Elektor_inferencing.h>
#include <Adafruit_NeoPixel.h>

// Hardware
#define MIC_PIN          4      // Analog Out from Electret
#define RGB_PIN          48     // ESP32-S3 Onboard LED
#define DC_OFFSET        53   // Mid-point for 12-bit ADC (3.3V / 2)

// Logic
#define CONFIDENCE_THRESHOLD 0.50 // changed down from 0.70 to accommodate noisy ADC and microphone

Adafruit_NeoPixel pixel(1, RGB_PIN, NEO_GRB + NEO_KHZ800);

// Audio Buffer State
typedef struct {
    signed short *buffers[2];
    unsigned char buf_select;
    volatile uint8_t buf_ready;
    unsigned int buf_count;
    unsigned int n_samples;
} inference_t;

static inference_t inference;
hw_timer_t *adcTimer = NULL;
portMUX_TYPE timerMux = portMUX_INITIALIZER_UNLOCKED;

// LED Timing (Non-blocking)
unsigned long led_off_time = 0;

/**
 * Timer Interrupt - Runs at 16kHz
 */
void IRAM_ATTR onTimer() {
    portENTER_CRITICAL_ISR(&timerMux);
    
    int16_t sample = analogRead(MIC_PIN) - DC_OFFSET;
    inference.buffers[inference.buf_select][inference.buf_count++] = sample;
    
    if(inference.buf_count >= inference.n_samples) {
        inference.buf_select ^= 1;
        inference.buf_count = 0;
        inference.buf_ready = 1;
    }
    
    portEXIT_CRITICAL_ISR(&timerMux);
}

void setup() {
    Serial.begin(115200);
    
    // Initialize LED
    pixel.begin();
    pixel.setPixelColor(0, pixel.Color(0, 5, 0)); // Dim green = System Alive
    pixel.show();

    // Initialize ADC
    analogReadResolution(12);
    analogSetAttenuation(ADC_11db); // Allow 0-3.3V range

    // Allocate Buffers
    if (!microphone_inference_start(EI_CLASSIFIER_SLICE_SIZE)) {
        Serial.println("ERR: Could not allocate audio buffers");
        pixel.setPixelColor(0, pixel.Color(10, 0, 0)); // Red = Error
        pixel.show();
        while(1);
    }

    run_classifier_init();
    Serial.println("Hey Elektor Model Initialized. Listening...");
}

void loop() {
    // 1. Non-blocking LED reset
    if (led_off_time > 0 && millis() > led_off_time) {
        pixel.setPixelColor(0, pixel.Color(0, 5, 0)); // Back to dim green
        pixel.show();
        led_off_time = 0;
    }

    // Serial.println(analogRead(MIC_PIN));

    // 2. Check for new audio data
    if (inference.buf_ready == 1) {

        Serial.println("Buffer full! Starting Inference..."); // Debug point B
        
        signal_t signal;
        signal.total_length = EI_CLASSIFIER_SLICE_SIZE;
        signal.get_data = &microphone_audio_signal_get_data;

        ei_impulse_result_t result = {0};
        EI_IMPULSE_ERROR r = run_classifier_continuous(&signal, &result, false);
        
        // Mark buffer as handled
        inference.buf_ready = 0;

        if (r != EI_IMPULSE_OK) return;

        // 3. Search for the label "hey elektor"
        for (size_t ix = 0; ix < EI_CLASSIFIER_LABEL_COUNT; ix++) {
            if (strcmp(result.classification[ix].label, "hey elektor") == 0) {
                
                // If confidence is high enough, flash the LED
                if (result.classification[ix].value > CONFIDENCE_THRESHOLD) {
                    Serial.printf("Wake Word Detected! Confidence: %.2f\n", result.classification[ix].value);
                    
                    pixel.setPixelColor(0, pixel.Color(0, 255, 255)); // Bright Cyan
                    pixel.show();
                    led_off_time = millis() + 1000; // Set to turn off in 1 second
                }
            }
        }
    }
}

/**
 * Inference Helper Functions
 */
static bool microphone_inference_start(uint32_t n_samples) {
    inference.buffers[0] = (signed short *)malloc(n_samples * sizeof(signed short));
    inference.buffers[1] = (signed short *)malloc(n_samples * sizeof(signed short));
    if (inference.buffers[0] == NULL || inference.buffers[1] == NULL) return false;

    inference.buf_select = 0;
    inference.buf_count = 0;
    inference.n_samples = n_samples;
    inference.buf_ready = 0;

    // Standard ESP32-S3 Timer Setup for 16kHz
    // Using a 1MHz clock (1 tick = 1 microsecond)
    // 1,000,000 / 16,000 = 62.5 ticks
    adcTimer = timerBegin(1000000); 
    timerAttachInterrupt(adcTimer, &onTimer);
    timerAlarm(adcTimer, 62, true, 0); // 62 ticks = ~16.1kHz
    
    // Explicitly start the timer if on Core 3.0
    #if ESP_ARDUINO_VERSION >= ESP_ARDUINO_VERSION_VAL(3, 0, 0)
    timerStart(adcTimer);
    #endif

    return true;
}

static int microphone_audio_signal_get_data(size_t offset, size_t length, float *out_ptr) {
    // Pull from the completed buffer (the one NOT being written to)
    numpy::int16_to_float(&inference.buffers[inference.buf_select ^ 1][offset], out_ptr, length);
    return 0;
}