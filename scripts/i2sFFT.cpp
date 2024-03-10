#include <Arduino.h>
#include "arduinoFFT.h"
#include "AudioTools.h"

using namespace audio_tools;

#define SAMPLE_RATE 44100
#define SAMPLES 1024
#define NUM_BANDS 8
#define EMA_ALPHA 0.1
#ifndef PI
#define PI 3.14159265358979323846
#endif

ArduinoFFT<float> FFT = ArduinoFFT<float>();

float vReal[SAMPLES];
float vImag[SAMPLES];
float binnedFFT[NUM_BANDS];
float lowerFrequencies[NUM_BANDS] = {62.5, 125, 250, 500, 1000, 2000, 4000, 8000};
float FFT_noise_threshold = 0.5;
float emaBinnedFFT[NUM_BANDS] = {0};


int bandLowerBin[NUM_BANDS];

I2SStream in;

float normalize(uint16_t sample)
{
  int16_t signedSample = sample;
  return signedSample / 32768.0;
}

void fetchAndWindowSamples() {
    uint8_t buffer[4]; // Buffer for two 16-bit samples (stereo)

    for (int i = 0; i < SAMPLES; i++) {
        if (in.readBytes(buffer, 4) == 4) {
            uint16_t sample = buffer[0] | (buffer[1] << 8); // Using only the left channel
            // Normalize the sample
            float normalizedSample = normalize(sample);
            // Apply a threshold filter before windowing
            if (abs(normalizedSample) < 0.003) {
                normalizedSample = 0;
            }
            // Apply windowing to the (possibly filtered) normalized sample
            vReal[i] = normalizedSample * (0.5 * (1.0 - cos(2 * PI * i / (SAMPLES - 1))));
            vImag[i] = 0;
        } else {
            vReal[i] = 0;
            vImag[i] = 0;
        }
    }
}


void performFFT()
{
  FFT.compute(vReal, vImag, SAMPLES, FFT_FORWARD); // FFT
  FFT.complexToMagnitude(vReal, vImag, SAMPLES);   // Convert to magnitudes
}

void binToOctaveBands(float vReal[], uint16_t sampleRate, size_t nFFTPoints, float (&binnedFFT)[NUM_BANDS])
{
  // Temporary storage for the current FFT magnitudes to be normalized
  float currentBinnedFFT[NUM_BANDS] = {0};

  // Accumulate FFT magnitudes into octave bands
  for (int i = 0; i < NUM_BANDS; i++) {
    int startBin = bandLowerBin[i];
    int endBin = (i < NUM_BANDS - 1) ? bandLowerBin[i + 1] : nFFTPoints / 2;

    for (int k = startBin; k < endBin; k++) {
      currentBinnedFFT[i] += vReal[k];
    }
    currentBinnedFFT[i] /= (endBin - startBin); // Average magnitude for the band
  }

  // Update the EMA for each band and normalize the current magnitude by the EMA
  for (int i = 0; i < NUM_BANDS; i++) {
    // Update EMA
    emaBinnedFFT[i] = EMA_ALPHA * currentBinnedFFT[i] + (1 - EMA_ALPHA) * emaBinnedFFT[i];

    // Normalize the binnedFFT by EMA, ensuring we avoid division by zero
    if (emaBinnedFFT[i] > FFT_noise_threshold) {
      binnedFFT[i] = currentBinnedFFT[i] / emaBinnedFFT[i];
    } else {
      binnedFFT[i] = 0;
    }

    // Truncate values to the 0 to 1 range
    binnedFFT[i] = binnedFFT[i] > 1 ? 1 : binnedFFT[i];
  }
}


void setup()
{
  Serial.begin(115200);
  AudioLogger::instance().begin(Serial, AudioLogger::Error);

  auto config = in.defaultConfig(RXTX_MODE);
  config.sample_rate = SAMPLE_RATE;
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

  for (int i = 0; i < NUM_BANDS; i++)
  {
    bandLowerBin[i] = round(lowerFrequencies[i] * SAMPLES / SAMPLE_RATE);
  }
}

void loop()
{
  fetchAndWindowSamples(); // Fetch and window the samples
  performFFT();            // Perform the FFT

  binToOctaveBands(vReal, SAMPLE_RATE, SAMPLES, binnedFFT);
  for (int i = 0; i < NUM_BANDS; i++)
  {
    Serial.print(binnedFFT[i]);
    Serial.print(" ");
  }

  Serial.println();
  delay(35); // Delay for a bit before the next batch
}
