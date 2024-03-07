#include <Arduino.h>
#include "arduinoFFT.h"
#include "AudioTools.h"

using namespace audio_tools;

#define SAMPLES 1024
#define SAMPLE_RATE 44100

arduinoFFT FFT = arduinoFFT();
double vReal[SAMPLES];
double vImag[SAMPLES];

uint16_t sample_rate = SAMPLE_RATE;
I2SStream in;

void fetchAndWindowSamples() {
  uint8_t buffer[4]; // Buffer for two 16-bit samples (stereo)

  for (int i = 0; i < SAMPLES; i++) {
    if (in.readBytes(buffer, 4) == 4) {
      // Using only the left channel samples and discarding the right channel
      int16_t sample = buffer[0] | (buffer[1] << 8);
      // Apply windowing to the sample
      vReal[i] = sample * (0.5 * (1.0 - cos(2 * PI * i / (SAMPLES - 1))));
      vImag[i] = 0;
    } else {
      vReal[i] = 0;
      vImag[i] = 0;
    }
  }
}

void performFFT() {
  FFT.Compute(vReal, vImag, SAMPLES, FFT_FORWARD); // FFT
  FFT.ComplexToMagnitude(vReal, vImag, SAMPLES); // Convert to magnitudes
}

void setup(){
  Serial.begin(115200);
  AudioLogger::instance().begin(Serial, AudioLogger::Error);

  auto config = in.defaultConfig(RXTX_MODE);
  config.sample_rate = sample_rate;
  config.bits_per_sample = 16;
  config.i2s_format = I2S_STD_FORMAT;
  config.is_master = true;
  config.port_no = 0;
  config.pin_ws = 18;
  config.pin_bck = 5;
  config.pin_data = 19;
  config.pin_data_rx = 17;
  config.pin_mck = 0;
  config.use_apll = true;

  in.begin(config);

  Serial.println("I2S started...");
}

void loop() {
  fetchAndWindowSamples(); // Fetch and window the samples
  performFFT(); // Perform the FFT

  for (int i = 0; i < (SAMPLES / 2); i++) { // Only half is needed due to symmetry
    // Here you can process the FFT result, e.g., print or send via Serial
    Serial.println(vReal[i]);
  }

  delay(40); // Delay for a bit before the next batch
}




