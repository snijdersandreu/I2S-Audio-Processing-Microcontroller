#include <Arduino.h>
#include "AudioTools.h"

using namespace audio_tools;

#define SAMPLES 2048
#define SAMPLE_RATE 44100

float vReal[SAMPLES];

uint16_t sample_rate = SAMPLE_RATE;
I2SStream in;

float normalize(uint16_t sample) {
    int16_t signedSample = sample;
    return signedSample / 32768.0;
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
  uint8_t buffer[4]; // Buffer for two 16-bit samples (stereo)

  for (int i = 0; i < SAMPLES; i++) {
    if (in.readBytes(buffer, 4) == 4) {
      uint16_t sample = buffer[0] | (buffer[1] << 8); // Using only the left channel little endian
      vReal[i] = normalize(sample); // Direct assignment without windowing
    } else {
      vReal[i] = 0;
    }
  }

  for (int i = 0; i < 30; i++) { 
    Serial.print(vReal[i], 6);
    Serial.print(" ");
  }
  Serial.println();
  delay(100); // Delay for a bit before the next batch
}

