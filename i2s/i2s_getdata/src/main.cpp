/*
 * ESP32 I2S Noise Level Example for INMP441 Microphone.
 *
 * This reads soundwave samples via I2S, computes a mean value, and prints it to serial.
 * Modify the 'samples' array processing in getINMP() for custom soundwave analysis.
 */

#include <driver/i2s.h>
#include <Arduino.h>

#define I2S_WS 15  // aka LRCL
#define I2S_SD 32  // aka DOUT
#define I2S_SCK 14 // aka BCLK

const i2s_port_t I2S_PORT = I2S_NUM_0;
const int BLOCK_SIZE = 64;
const int SAMPLE_RATE = 10240;  // Adjust as needed (e.g., 16000 or 44100)
float mean = 0;
bool INMP_flag = 0;

void setup() {
  Serial.begin(115200);
  Serial.println("Configuring I2S...");
  esp_err_t i2s_err;

  // I2S configuration
  const i2s_config_t i2s_config = {
    .mode = i2s_mode_t(I2S_MODE_MASTER | I2S_MODE_RX),  // Receive mode
    .sample_rate = SAMPLE_RATE,
    .bits_per_sample = I2S_BITS_PER_SAMPLE_32BIT,  // 32-bit samples
    .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT,   // Left channel (L/R tied to GND)
    .communication_format = i2s_comm_format_t(I2S_COMM_FORMAT_I2S | I2S_COMM_FORMAT_I2S_MSB),
    .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,      // Interrupt level 1
    .dma_buf_count = 8,                            // Number of DMA buffers
    .dma_buf_len = BLOCK_SIZE                      // Samples per buffer
  };

  // I2S pin configuration
  const i2s_pin_config_t pin_config = {
    .bck_io_num = I2S_SCK,   // BCLK
    .ws_io_num = I2S_WS,     // LRCL
    .data_out_num = -1,      // Not used (output)
    .data_in_num = I2S_SD    // DOUT
  };

  // Install I2S driver
  i2s_err = i2s_driver_install(I2S_PORT, &i2s_config, 0, NULL);
  if (i2s_err != ESP_OK) {
    Serial.printf("Failed installing driver: %d\n", i2s_err);
    while (true);
  }

  // Set I2S pins
  i2s_err = i2s_set_pin(I2S_PORT, &pin_config);
  if (i2s_err != ESP_OK) {
    Serial.printf("Failed setting pin: %d\n", i2s_err);
    while (true);
  }

  Serial.println("I2S driver installed.");
  delay(1000);  // Time to read serial messages

  // Initial mic check
  getINMP();
  if (mean != 0.0) {
    Serial.println("INMP441 is present.");
    INMP_flag = 1;
  } else {
    Serial.println("No INMP441 detected. Check wiring.");
  }
}

void loop() {
  if (INMP_flag) {
    getINMP();
    Serial.print(abs(mean));  // Print absolute mean for sound level
    Serial.print(" ");
    Serial.println(1600);     // Reference line for serial plotter scaling
  } else {
    // Fallback: Add analog read or other logic if no mic
    delay(1000);
  }
}

void getINMP() {
  // Read soundwave samples into buffer
  int32_t samples[BLOCK_SIZE];
  size_t num_bytes_read = 0;
  esp_err_t result = i2s_read(I2S_PORT, (void*)samples, BLOCK_SIZE * sizeof(int32_t), &num_bytes_read, portMAX_DELAY);

  int samples_read = num_bytes_read / sizeof(int32_t);
  mean = 0;  // Reset mean

  if (samples_read > 0) {
    for (int i = 0; i < samples_read; ++i) {
      // Process raw 32-bit samples (shift right by 8 bits to get 24-bit effective data from INMP441)
      mean += (samples[i] >> 8);  // Adjust shift based on your needs
    }
    mean = mean / samples_read / 1024.0;  // Normalize (adjust divisor for scaling)
    
    // Optional: Print raw samples for full waveform data
    // for (int i = 0; i < samples_read; ++i) {
    //   Serial.println(samples[i] >> 8);
    // }
  }
}