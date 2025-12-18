# 3.2 ESP-IDF I2S Driver (via Arduino)

## Overview

The Arduino framework for ESP32 is essentially ESP-IDF wearing a cardigan and trying to look approachable. Beneath its friendly `Serial.println()` exterior lurks the full power of Espressif's IoT Development Framework, including the I2S driver.

We'll configure I2S to capture audio from the microphone. This involves configuration structures, DMA buffers, and terminology that sounds more intimidating than it is.

## What You'll Learn

- Configuring I2S in receive (RX) mode for microphone input
- Setting up PDM capture (if your mic is PDM)
- Understanding DMA buffer configuration
- Reading audio samples into your code

## Prerequisites

- Completed [Step 3.1: I2S Protocol Understanding](./07-i2s-protocol-understanding.md)
- The M5 Capsule microphone pin assignments (from documentation)
- Acceptance that this may not work on the first try

---

## Step 1: Include the Headers

The I2S driver lives in ESP-IDF but is accessible from Arduino:

```cpp
#include <Arduino.h>
#include <driver/i2s.h>
```

The `driver/i2s.h` header provides the low-level I2S functions. It's documented in the [ESP-IDF I2S documentation](https://docs.espressif.com/projects/esp-idf/en/latest/esp32s3/api-reference/peripherals/i2s.html), which you should bookmark. You'll be visiting it frequently.

**Note on API versions:** ESP-IDF v5.x introduced a new I2S API (`driver/i2s_std.h`, `driver/i2s_pdm.h`). Depending on your Arduino-ESP32 core version, you may have the legacy API (`driver/i2s.h`) or the new API. This guide covers the legacy API as it's more commonly available in Arduino. The concepts transfer to the new API with different function names.

## Step 2: Configuration Structure

I2S is configured using a structure that specifies everything: mode, sample rate, bits, pins, and buffer sizes.

```cpp
#include <Arduino.h>
#include <driver/i2s.h>

// Pin definitions - UPDATE FOR YOUR HARDWARE
#define I2S_MIC_SERIAL_CLOCK     GPIO_NUM_X   // SCK/BCLK
#define I2S_MIC_LEFT_RIGHT_CLOCK GPIO_NUM_Y   // WS/LRCLK
#define I2S_MIC_SERIAL_DATA      GPIO_NUM_Z   // SD/DATA

// Audio parameters
#define SAMPLE_RATE     16000
#define BITS_PER_SAMPLE I2S_BITS_PER_SAMPLE_16BIT
#define CHANNEL_FORMAT  I2S_CHANNEL_FMT_ONLY_LEFT  // Mono mic

// DMA buffer configuration
#define DMA_BUF_COUNT   4       // Number of DMA buffers
#define DMA_BUF_LEN     1024    // Samples per buffer

void configureI2S() {
    // I2S configuration structure
    i2s_config_t i2s_config = {
        .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX),
        .sample_rate = SAMPLE_RATE,
        .bits_per_sample = BITS_PER_SAMPLE,
        .channel_format = CHANNEL_FORMAT,
        .communication_format = I2S_COMM_FORMAT_STAND_I2S,
        .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
        .dma_buf_count = DMA_BUF_COUNT,
        .dma_buf_len = DMA_BUF_LEN,
        .use_apll = false,
        .tx_desc_auto_clear = false,
        .fixed_mclk = 0
    };

    // Pin configuration
    i2s_pin_config_t pin_config = {
        .bck_io_num = I2S_MIC_SERIAL_CLOCK,
        .ws_io_num = I2S_MIC_LEFT_RIGHT_CLOCK,
        .data_out_num = I2S_PIN_NO_CHANGE,  // Not transmitting
        .data_in_num = I2S_MIC_SERIAL_DATA
    };

    // Install and configure
    esp_err_t err = i2s_driver_install(I2S_NUM_0, &i2s_config, 0, NULL);
    if (err != ESP_OK) {
        Serial.printf("ERROR: i2s_driver_install failed: %s\n", esp_err_to_name(err));
        return;
    }

    err = i2s_set_pin(I2S_NUM_0, &pin_config);
    if (err != ESP_OK) {
        Serial.printf("ERROR: i2s_set_pin failed: %s\n", esp_err_to_name(err));
        return;
    }

    Serial.println("I2S configured successfully");
}
```

### Understanding the Configuration

| Field                  | Purpose                      | Your Value                                                            |
| ---------------------- | ---------------------------- | --------------------------------------------------------------------- |
| `mode`                 | Master/slave, TX/RX          | `I2S_MODE_MASTER \| I2S_MODE_RX` (we generate clock, we receive data) |
| `sample_rate`          | Samples per second           | 16000                                                                 |
| `bits_per_sample`      | Resolution                   | `I2S_BITS_PER_SAMPLE_16BIT`                                           |
| `channel_format`       | Mono/stereo                  | `I2S_CHANNEL_FMT_ONLY_LEFT` (mono)                                    |
| `communication_format` | I2S variant                  | `I2S_COMM_FORMAT_STAND_I2S` (standard Philips)                        |
| `intr_alloc_flags`     | Interrupt priority           | `ESP_INTR_FLAG_LEVEL1` (low, doesn't block system)                    |
| `dma_buf_count`        | Number of DMA buffers        | 4 (more = more latency, more safety)                                  |
| `dma_buf_len`          | Samples per buffer           | 1024 (adjust based on your processing)                                |
| `use_apll`             | Audio PLL for precise timing | `false` (true for exact sample rates)                                 |

## Step 3: PDM Configuration (If Applicable)

If the M5 Capsule uses a PDM microphone (which is likely), the configuration differs slightly:

```cpp
void configurePDM() {
    // PDM configuration
    i2s_config_t i2s_config = {
        .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX | I2S_MODE_PDM),
        .sample_rate = SAMPLE_RATE,
        .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
        .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT,
        .communication_format = I2S_COMM_FORMAT_STAND_I2S,
        .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
        .dma_buf_count = DMA_BUF_COUNT,
        .dma_buf_len = DMA_BUF_LEN,
        .use_apll = false,
        .tx_desc_auto_clear = false,
        .fixed_mclk = 0
    };

    // For PDM, we only need CLK and DATA pins
    // WS is not used in PDM mode
    i2s_pin_config_t pin_config = {
        .bck_io_num = I2S_PIN_NO_CHANGE,    // Not used for PDM
        .ws_io_num = I2S_MIC_SERIAL_CLOCK,  // PDM CLK
        .data_out_num = I2S_PIN_NO_CHANGE,  // Not transmitting
        .data_in_num = I2S_MIC_SERIAL_DATA  // PDM DATA
    };

    esp_err_t err = i2s_driver_install(I2S_NUM_0, &i2s_config, 0, NULL);
    if (err != ESP_OK) {
        Serial.printf("ERROR: PDM driver install failed: %s\n", esp_err_to_name(err));
        return;
    }

    err = i2s_set_pin(I2S_NUM_0, &pin_config);
    if (err != ESP_OK) {
        Serial.printf("ERROR: PDM set pin failed: %s\n", esp_err_to_name(err));
        return;
    }

    Serial.println("PDM I2S configured successfully");
}
```

The key difference is `I2S_MODE_PDM` in the mode flags. The ESP32 handles the decimation (converting 1-bit PDM to PCM) internally.

## Step 4: Reading Audio Data

With I2S configured, we read samples using `i2s_read()`:

```cpp
#define BUFFER_SIZE 1024

int16_t audioBuffer[BUFFER_SIZE];

size_t readAudioSamples() {
    size_t bytesRead = 0;

    esp_err_t err = i2s_read(
        I2S_NUM_0,
        audioBuffer,
        sizeof(audioBuffer),
        &bytesRead,
        portMAX_DELAY  // Wait forever for data
    );

    if (err != ESP_OK) {
        Serial.printf("ERROR: i2s_read failed: %s\n", esp_err_to_name(err));
        return 0;
    }

    // Convert bytes to samples
    size_t samplesRead = bytesRead / sizeof(int16_t);
    return samplesRead;
}
```

### Understanding i2s_read()

```cpp
esp_err_t i2s_read(
    i2s_port_t i2s_num,      // Which I2S port (I2S_NUM_0 or I2S_NUM_1)
    void *dest,               // Destination buffer
    size_t size,              // Buffer size in BYTES
    size_t *bytes_read,       // Output: actual bytes read
    TickType_t ticks_to_wait  // Timeout (portMAX_DELAY = wait forever)
);
```

The function fills your buffer with audio samples and tells you how many bytes it actually read. This may be less than you requested if the timeout is reached.

**Important:** The size parameter is in bytes, not samples. For 16-bit audio, multiply your sample count by 2.

## Step 5: Complete Working Example

Let's put it together into a sketch that captures audio and prints sample values:

```cpp
#include <Arduino.h>
#include <M5Capsule.h>
#include <driver/i2s.h>

// UPDATE THESE PINS FOR YOUR HARDWARE
#define I2S_MIC_CLK  GPIO_NUM_43   // PDM Clock
#define I2S_MIC_DATA GPIO_NUM_44   // PDM Data

#define SAMPLE_RATE     16000
#define DMA_BUF_COUNT   4
#define DMA_BUF_LEN     1024
#define BUFFER_SIZE     1024

int16_t audioBuffer[BUFFER_SIZE];
bool i2sInitialised = false;

void setupI2S() {
    Serial.println("Configuring I2S (PDM mode)...");

    i2s_config_t i2s_config = {
        .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX | I2S_MODE_PDM),
        .sample_rate = SAMPLE_RATE,
        .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
        .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT,
        .communication_format = I2S_COMM_FORMAT_STAND_I2S,
        .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
        .dma_buf_count = DMA_BUF_COUNT,
        .dma_buf_len = DMA_BUF_LEN,
        .use_apll = false,
        .tx_desc_auto_clear = false,
        .fixed_mclk = 0
    };

    i2s_pin_config_t pin_config = {
        .bck_io_num = I2S_PIN_NO_CHANGE,
        .ws_io_num = I2S_MIC_CLK,
        .data_out_num = I2S_PIN_NO_CHANGE,
        .data_in_num = I2S_MIC_DATA
    };

    esp_err_t err = i2s_driver_install(I2S_NUM_0, &i2s_config, 0, NULL);
    if (err != ESP_OK) {
        Serial.printf("ERROR: i2s_driver_install failed: %s\n", esp_err_to_name(err));
        return;
    }

    err = i2s_set_pin(I2S_NUM_0, &pin_config);
    if (err != ESP_OK) {
        Serial.printf("ERROR: i2s_set_pin failed: %s\n", esp_err_to_name(err));
        return;
    }

    i2sInitialised = true;
    Serial.println("I2S initialised successfully!");
    Serial.printf("  Sample rate: %d Hz\n", SAMPLE_RATE);
    Serial.printf("  Buffer: %d samples x %d buffers\n", DMA_BUF_LEN, DMA_BUF_COUNT);
}

void setup() {
    M5.begin();
    Serial.begin(115200);
    while (!Serial) delay(10);

    Serial.println("\n=== I2S Audio Capture Test ===\n");

    setupI2S();

    if (!i2sInitialised) {
        Serial.println("I2S setup failed. Halting.");
        while (true) delay(1000);
    }

    Serial.println("\nReading audio samples...");
    Serial.println("Speak near the microphone or clap to see values change.\n");
}

void loop() {
    if (!i2sInitialised) {
        delay(1000);
        return;
    }

    // Read samples
    size_t bytesRead = 0;
    esp_err_t err = i2s_read(
        I2S_NUM_0,
        audioBuffer,
        sizeof(audioBuffer),
        &bytesRead,
        pdMS_TO_TICKS(100)
    );

    if (err != ESP_OK) {
        Serial.printf("Read error: %s\n", esp_err_to_name(err));
        return;
    }

    size_t samplesRead = bytesRead / sizeof(int16_t);

    if (samplesRead == 0) {
        Serial.println("No samples received");
        return;
    }

    // Calculate statistics
    int32_t sum = 0;
    int16_t minVal = 32767;
    int16_t maxVal = -32768;

    for (size_t i = 0; i < samplesRead; i++) {
        int16_t sample = audioBuffer[i];
        sum += abs(sample);
        if (sample < minVal) minVal = sample;
        if (sample > maxVal) maxVal = sample;
    }

    int32_t avgAmplitude = sum / samplesRead;

    // Print statistics
    Serial.printf("Samples: %d | Min: %6d | Max: %6d | Avg: %5ld | ",
                  samplesRead, minVal, maxVal, avgAmplitude);

    // Visual level meter
    int bars = map(avgAmplitude, 0, 5000, 0, 30);
    bars = constrain(bars, 0, 30);
    for (int i = 0; i < bars; i++) Serial.print("█");
    for (int i = bars; i < 30; i++) Serial.print("░");
    Serial.println();

    delay(100);  // Update ~10 times per second
}
```

### What to Expect

When running this sketch:

1. **Silent room:** Low average amplitude (maybe 50-200)
2. **Talking:** Medium amplitude (500-2000)
3. **Clapping nearby:** High amplitude (5000+, possibly clipping at 32767)

If you see:

- **All zeros:** Mic isn't connected or wrong pins
- **32767/-32768 constantly:** Incorrect format or broken hardware
- **Constant non-zero value:** DC offset issue (we'll fix this later)
- **Reasonable variation:** Success!

## Step 6: Troubleshooting

### "i2s_driver_install failed"

```
ERROR: i2s_driver_install failed: ESP_ERR_INVALID_ARG
```

Common causes:

- Invalid pin numbers
- DMA buffer count too small (minimum 2)
- Invalid sample rate (try 16000 or 44100)

### "No samples received"

- Check pin connections
- For PDM: Is `I2S_MODE_PDM` in the mode flags?
- Is the microphone powered?

### Constant values or noise

- Try different `communication_format` values
- Check if left/right channel is correct
- Some mics need a delay after power-on before outputting valid data

### Samples but wrong scale

If values seem too small or large, the microphone may have different sensitivity or the bit depth may be misconfigured.

---

## Verification

Before moving on, confirm:

- [ ] I2S initialises without errors
- [ ] You see varying sample values in the serial monitor
- [ ] Values respond to sound (higher when you speak or clap)
- [ ] No constant error messages during operation

If the output looks like audio (varying values that respond to sound), you're ready for the next step.

---

## What's Next

You have audio samples appearing in RAM. Excellent. But those samples vanish into the void each loop iteration. In [Step 3.3: Basic Audio Capture Sketch](./09-basic-audio-capture-sketch.md), we'll capture a few seconds of audio and examine it more closely, preparing for the ultimate goal: writing it to the SD card.

---

## Quick Reference

```cpp
// Include
#include <driver/i2s.h>

// Install driver
i2s_driver_install(I2S_NUM_0, &config, 0, NULL);

// Set pins
i2s_set_pin(I2S_NUM_0, &pin_config);

// Read samples
i2s_read(I2S_NUM_0, buffer, size_in_bytes, &bytes_read, timeout);

// Uninstall driver (for cleanup)
i2s_driver_uninstall(I2S_NUM_0);

// Mode flags
I2S_MODE_MASTER  // We generate clock
I2S_MODE_RX      // We receive data
I2S_MODE_PDM     // PDM microphone mode

// Bit depths
I2S_BITS_PER_SAMPLE_8BIT
I2S_BITS_PER_SAMPLE_16BIT
I2S_BITS_PER_SAMPLE_24BIT
I2S_BITS_PER_SAMPLE_32BIT

// Channel formats
I2S_CHANNEL_FMT_ONLY_LEFT
I2S_CHANNEL_FMT_ONLY_RIGHT
I2S_CHANNEL_FMT_RIGHT_LEFT  // Stereo
```
