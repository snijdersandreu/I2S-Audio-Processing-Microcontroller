#include <Arduino.h>
#include "arduinoFFT.h"
#include "AudioTools.h"
#include <WiFi.h>
#include <ESPmDNS.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <ArduinoJson.h> // Include ArduinoJson library for JSON parsing


using namespace audio_tools;

#define SAMPLE_RATE 44100
#define SAMPLES 2048
#define NUM_BANDS 8
#define EMA_ALPHA 0.1
#ifndef PI
#define PI 3.14159265358979323846
#endif

const char *ssid = "TP-Link_E343";
const char *password = "73571476";

const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML><html>
<head>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <title>ESP32 Spectrum Analyzer</title>
  <link href="https://fonts.googleapis.com/css2?family=Roboto:wght@400;700&display=swap" rel="stylesheet">
  <script src="https://cdn.jsdelivr.net/npm/chart.js"></script>
  <style>
    body {
      font-family: 'Roboto', sans-serif;
      display: flex;
      justify-content: center;
      align-items: center;
      flex-direction: column;
      height: 100vh;
      margin: 0;
      background-color: #f5f5f5;
      color: #333;
    }
    h2 {
      margin: 20px 0;
      font-size: 24px;
      text-align: center;
      color: #333;
    }
    .chart-container {
      width: 90%;
      max-width: 600px;
      max-height: 400px; /* Preserved max-height to avoid stretching */
      background-color: #fff;
      box-shadow: 0 2px 4px rgba(0,0,0,0.1);
      border-radius: 8px;
      padding: 20px;
      display: flex;
      flex-direction: column;
      align-items: center;
      gap: 30px; /* Increased space between chart and button */
    }
    canvas {
      width: 100% !important;
      max-width: 100%;
      height: auto !important;
    }
    #toggleFFT {
      padding: 15px 50px; /* Larger button */
      font-size: 18px; /* Larger text */
      color: #fff;
      background-color: #007bff;
      border: none;
      border-radius: 5px;
      cursor: pointer;
      transition: background-color 0.3s;
    }
    #toggleFFT:hover {
      background-color: #0056b3;
    }
  </style>
</head>
<body>
  <h2>Bandas de frecuencia</h2>
  <h4>Se recomienda desactivar la fuente con el boton de abajo cuando no haya musica</h4>
  <div class="chart-container">
    <canvas id="spectrumChart"></canvas>
    <button id="toggleFFT">Desactivar fuente</button>
  </div>
  <script>
    var ctx = document.getElementById('spectrumChart').getContext('2d');
    var fftActive = true; 
    var spectrumChart = new Chart(ctx, {
      type: 'bar',
      data: {
        labels: ['62.5Hz', '125Hz', '250Hz', '500Hz', '1kHz', '2kHz', '4kHz', '8kHz'],
        datasets: [{
          label: 'Spectrum',
          data: [0, 0, 0, 0, 0, 0, 0, 0],
          backgroundColor: 'rgba(54, 162, 235, 0.5)',
          borderColor: 'rgba(54, 162, 235, 1)',
          borderWidth: 1
        }]
      },
      options: {
        scales: {
          y: {
            beginAtZero: true,
            min: 0,
            max: 1,
            ticks: {
              stepSize: 0.1
            }
          }
        },
        responsive: true,
        maintainAspectRatio: false
      }
    });

    document.getElementById('toggleFFT').addEventListener('click', function() {
      fftActive = !fftActive; // Toggle the state
      this.textContent = fftActive ? 'Desactivar fuente' : 'Activar fuente';

      if(ws.readyState === WebSocket.OPEN) {
        ws.send(JSON.stringify({type: 'FFT_DISPLAY', active: fftActive}));
      }

      if (!fftActive) {
        spectrumChart.data.datasets.forEach((dataset) => {
          dataset.data.fill(0);
        });
        spectrumChart.update();
      }
    });

    var wsUri = 'ws://' + window.location.hostname + '/ws';
    var ws;

    function initWebSocket() {
      console.log('Trying to open a WebSocket connection...');
      ws = new WebSocket(wsUri);
      ws.onopen = function(evt) { console.log("WebSocket open"); };
      ws.onclose = function(evt) {
        console.log("WebSocket closed, retrying...");
        setTimeout(function() { initWebSocket(); }, 2000);
      };
      ws.onmessage = function(event) {
        if (fftActive) {
          var data = JSON.parse(event.data);
          spectrumChart.data.datasets[0].data = data;
          spectrumChart.update();
        }
      };
      ws.onerror = function(evt) { console.log("WebSocket error"); };
    }

    window.addEventListener('load', initWebSocket);
  </script>
</body>
</html>
)rawliteral";




ArduinoFFT<float> FFT = ArduinoFFT<float>();

AsyncWebServer server(80); // HTTP server will run on port 80
AsyncWebSocket ws("/ws");  // Create a WebSocket object

I2SStream in;

unsigned long lastUpdateTime = 0;
const long updateInterval = 1000 / 50; // Interval for updating the spectrum data, in milliseconds (12.5fps)

float vReal[SAMPLES];
float vImag[SAMPLES];
float binnedFFT[NUM_BANDS];
float lowerFrequencies[NUM_BANDS] = {63, 125, 250, 500, 1000, 2000, 4000, 8000};
float FFT_noise_threshold = 0.5;
float emaBinnedFFT[NUM_BANDS] = {0};

int bandLowerBin[NUM_BANDS];

bool fftActive = true; 

float normalize(uint16_t sample)
{
  int16_t signedSample = sample;
  return signedSample / 32768.0;
}

void expand(float binnedFFT[], size_t size)
{
  for (size_t i = 0; i < size; i++)
  {
    if (binnedFFT[i] < 0.5)
    {
      // Set values below 0.5 to 0
      binnedFFT[i] = 0;
    }
    else
    {
      // Rescale values from 0.5 to 1 to be from 0 to 1
      // The formula for rescaling from [A,B] to [C,D] is: (val-A)/(B-A) * (D-C) + C
      // In this case, from [0.5, 1] to [0, 1], which simplifies to: (val-0.5) / (1-0.5)
      binnedFFT[i] = (binnedFFT[i] - 0.5) * 2;
    }
  }
}

void fetchAndWindowSamples()
{
  uint8_t buffer[4]; // Buffer for two 16-bit samples (stereo)

  for (int i = 0; i < SAMPLES; i++)
  {
    if (in.readBytes(buffer, 4) == 4)
    {
      uint16_t sample = buffer[0] | (buffer[1] << 8); // Using only the left channel
      // Apply windowing and normalizing the sample
      vReal[i] = normalize(sample) * (0.5 * (1.0 - cos(2 * PI * i / (SAMPLES - 1))));
      vImag[i] = 0;
    }
    else
    {
      vReal[i] = 0;
      vImag[i] = 0;
    }
  }
}

void performFFT()
{
  if(!fftActive){
    for(int i = 0; i < SAMPLES; i++){
      vReal[i] = 0;
    }
  }
  FFT.compute(vReal, vImag, SAMPLES, FFT_FORWARD); // FFT
  FFT.complexToMagnitude(vReal, vImag, SAMPLES);   // Convert to magnitudes
}

void binToOctaveBands(float vReal[], uint16_t sampleRate, size_t nFFTPoints, float (&binnedFFT)[NUM_BANDS])
{
  // Temporary storage for the current FFT magnitudes to be normalized
  float currentBinnedFFT[NUM_BANDS] = {0};

  // Accumulate FFT magnitudes into octave bands
  for (int i = 0; i < NUM_BANDS; i++)
  {
    int startBin = bandLowerBin[i];
    int endBin = (i < NUM_BANDS - 1) ? bandLowerBin[i + 1] : nFFTPoints / 2;

    for (int k = startBin; k < endBin; k++)
    {
      currentBinnedFFT[i] += vReal[k];
    }
    currentBinnedFFT[i] /= (endBin - startBin); // Average magnitude for the band
  }

  // Update the EMA for each band and normalize the current magnitude by the EMA
  for (int i = 0; i < NUM_BANDS; i++)
  {
    // Update EMA
    emaBinnedFFT[i] = EMA_ALPHA * currentBinnedFFT[i] + (1 - EMA_ALPHA) * emaBinnedFFT[i];

    // Normalize the binnedFFT by EMA, ensuring we avoid division by zero
    if (emaBinnedFFT[i] > FFT_noise_threshold)
    {
      binnedFFT[i] = currentBinnedFFT[i] / emaBinnedFFT[i];
    }
    else
    {
      binnedFFT[i] = 0;
    }

    // Truncate values to the 0 to 1 range
    binnedFFT[i] = binnedFFT[i] > 1 ? 1 : binnedFFT[i];
  }
}

bool connectToWiFi(const char *ssid, const char *password, int maxRetries = 20)
{
  Serial.println("Connecting to WiFi...");
  WiFi.begin(ssid, password);

  int retries = 0;
  while (WiFi.status() != WL_CONNECTED && retries < maxRetries)
  {
    delay(500);
    Serial.print(".");
    retries++;
  }

  if (WiFi.status() != WL_CONNECTED)
  {
    Serial.println("Failed to connect to WiFi. Please check your credentials");
    return false;
  }

  Serial.println("Connected to WiFi");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());
  return true;
}

String createJsonMessage()
{
  String message = "[";
  for (int i = 0; i < NUM_BANDS; i++)
  {
    message += String(binnedFFT[i]);
    if (i < NUM_BANDS - 1)
      message += ",";
  }
  message += "]";
  return message;
}

void onWsEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len) {
  if (type == WS_EVT_CONNECT) {
    Serial.println("WebSocket client connected");
    client->text("Welcome!");
  } else if (type == WS_EVT_DISCONNECT) {
    Serial.println("WebSocket client disconnected");
  } else if (type == WS_EVT_ERROR) {
    Serial.println("WebSocket error occurred");
  } else if (type == WS_EVT_DATA) {
    // Create a temporary buffer for the incoming data
    String message = String((char*)data).substring(0, len);
    // Use ArduinoJson to parse the incoming JSON message
    DynamicJsonDocument doc(1024); // Adjust size according to your expected messages
    DeserializationError error = deserializeJson(doc, message);
    
    if (!error) { // Check if JSON parsing succeeded
      if (doc.containsKey("type") && doc["type"] == "FFT_DISPLAY") {
        // If the message is to toggle the FFT display, update the fftActive variable
        fftActive = doc["active"].as<bool>(); // Update your global fftActive variable
        Serial.print("FFT Display is now ");
        Serial.println(fftActive ? "active" : "inactive");
      }
    } else {
      // If there was an error parsing the JSON, you can log it or handle it as needed
      Serial.print("JSON parsing failed: ");
      Serial.println(error.c_str());
    }
  }
  // Handle other events...
}


void sendSpectrumData()
{
  if (ws.count() > 0)
  { // Check if there are any connected clients
    String message = createJsonMessage();
    ws.textAll(message);
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

  // Connect to Wi-Fi
  connectToWiFi(ssid, password);

  if (!MDNS.begin("fontana"))
  { // Start the mDNS responder for http://fontana.local
    Serial.println("Error starting mDNS responder!");
  }
  Serial.println("mDNS responder started");

  // Setup web server routes
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request)
            { request->send_P(200, "text/html", index_html); });

  // Start the server
  server.begin();

  ws.onEvent(onWsEvent);
  server.addHandler(&ws); // Attach WebSocket server to the web server
}

void loop()
{
  unsigned long currentMillis = millis();

  if (currentMillis - lastUpdateTime >= updateInterval)
  {
    lastUpdateTime = currentMillis;

    fetchAndWindowSamples(); // Fetch and window the samples
    performFFT();            // Perform the FFT

    binToOctaveBands(vReal, SAMPLE_RATE, SAMPLES, binnedFFT);
    expand(binnedFFT, NUM_BANDS);

    for (int i = 0; i < NUM_BANDS; i++)
    {
      Serial.print(binnedFFT[i]);
      Serial.print(" ");
    }

    Serial.println();

    sendSpectrumData();
  }
}
