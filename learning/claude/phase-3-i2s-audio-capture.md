# Phase 3: I2S Audio Capture

## Learning Objectives

By the end of this phase, you will:

- Understand the I2S protocol and its variants (standard, PDM, TDM)
- Configure the ESP32-S3's I2S peripheral for microphone input
- Capture audio data into memory buffers
- Handle DMA transfers and buffer management
- Verify audio capture with visual feedback (RMS levels)

---

## Step 1: Understanding I2S

I2S (Inter-IC Sound) is a serial bus protocol for transmitting digital audio between integrated circuits. Despite the name suggesting a two-device protocol, it's evolved into a somewhat overloaded term covering several related standards.

### 1.1 Standard I2S

The classic I2S format uses three signals:

| Signal     | Name        | Function                     |
| ---------- | ----------- | ---------------------------- |
| SCK (BCLK) | Bit Clock   | Clocks each bit              |
| WS (LRCLK) | Word Select | Left/Right channel indicator |
| SD (DATA)  | Serial Data | The actual audio samples     |

Data format:

- Two's complement signed integers
- MSB first
- WS = 0 for left channel, WS = 1 for right channel
- Data transitions one clock after WS changes

### 1.2 PDM (Pulse Density Modulation)

Many MEMS microphones, including the one in the M5 Capsule, output **PDM** rather than standard I2S. PDM is fundamentally different:

- Uses only two wires: **CLK** and **DATA**
- Data is a stream of 1s and 0s at a very high rate (typically 1-3 MHz)
- The density of 1s represents the amplitude
- Requires decimation filtering to convert to PCM

The ESP32-S3's I2S peripheral has built-in PDM-to-PCM conversion, which saves you from implementing the decimation filter yourself. This is a considerable mercy.

### 1.3 M5 Capsule Microphone

The M5 Capsule uses an **SPM1423** PDM microphone. Key specifications:

| Parameter   | Value                    |
| ----------- | ------------------------ |
| Type        | MEMS PDM                 |
| SNR         | 61 dB                    |
| Sensitivity | -26 dBFS                 |
| Clock       | 1.0 - 3.25 MHz           |
| Data format | PDM on rising clock edge |

**Pin assignments on M5 Capsule:**

| Function | GPIO    |
| -------- | ------- |
| PDM CLK  | GPIO 43 |
| PDM DATA | GPIO 44 |

---

## Step 2: ESP-IDF I2S Driver Overview

The ESP-IDF v5.x I2S driver has been completely rewritten from v4.x. Make sure you're reading the correct documentation.

### 2.1 Driver Architecture

```
┌─────────────────────────────────────┐
│         Application Code            │
└──────────────────┬──────────────────┘
                   │
┌──────────────────▼──────────────────┐
│       I2S Driver (i2s_channel)      │
│  ┌─────────────────────────────┐    │
│  │   i2s_std  │  i2s_pdm  │  i2s_tdm │
│  └─────────────────────────────┘    │
└──────────────────┬──────────────────┘
                   │
┌──────────────────▼──────────────────┐
│          DMA Controller             │
└──────────────────┬──────────────────┘
                   │
┌──────────────────▼──────────────────┐
│           I2S Peripheral            │
└─────────────────────────────────────┘
```

### 2.2 Key Concepts

**Channel:** An I2S channel represents one direction of communication (TX or RX). The ESP32-S3 has two I2S controllers, each capable of TX and RX.

**Slot:** Within a channel, slots are the time divisions for different audio channels (left, right, or more in TDM).

**DMA Descriptors:** Circular buffers managed by DMA. Audio flows into/out of these automatically while the CPU does other things.

---

## Step 3: Project Setup

Create a new project `pendant-audio`:

```ini
; platformio.ini
[env:m5capsule]
platform = espressif32
board = esp32-s3-devkitc-1
framework = espidf

monitor_speed = 115200
build_flags =
    -DCONFIG_ESP_CONSOLE_USB_SERIAL_JTAG=y

board_build.partitions = default.csv
board_build.flash_mode = dio
```

---

## Step 4: Basic PDM Microphone Capture

### 4.1 Required Headers

```c
#include <stdio.h>
#include <string.h>
#include <math.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/i2s_pdm.h"
#include "esp_log.h"
```

### 4.2 Configuration Constants

```c
static const char *TAG = "audio";

// M5 Capsule PDM microphone pins
#define PDM_CLK_GPIO    43
#define PDM_DATA_GPIO   44

// Audio parameters
#define SAMPLE_RATE     16000   // 16 kHz - good for speech
#define SAMPLE_BITS     16      // 16-bit audio
#define DMA_BUF_COUNT   4       // Number of DMA buffers
#define DMA_BUF_LEN     1024    // Samples per DMA buffer
```

### 4.3 I2S Channel Handle

```c
static i2s_chan_handle_t rx_chan = NULL;
```

### 4.4 Initialisation Function

```c
esp_err_t init_microphone(void)
{
    esp_err_t ret;

    ESP_LOGI(TAG, "Initialising PDM microphone");

    // Step 1: Create the I2S channel
    i2s_chan_config_t chan_cfg = I2S_CHANNEL_DEFAULT_CONFIG(I2S_NUM_0, I2S_ROLE_MASTER);
    chan_cfg.dma_desc_num = DMA_BUF_COUNT;
    chan_cfg.dma_frame_num = DMA_BUF_LEN;

    ret = i2s_new_channel(&chan_cfg, NULL, &rx_chan);  // NULL for TX, we only need RX
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to create I2S channel: %s", esp_err_to_name(ret));
        return ret;
    }

    // Step 2: Configure as PDM RX
    i2s_pdm_rx_config_t pdm_rx_cfg = {
        .clk_cfg = I2S_PDM_RX_CLK_DEFAULT_CONFIG(SAMPLE_RATE),
        .slot_cfg = I2S_PDM_RX_SLOT_DEFAULT_CONFIG(I2S_DATA_BIT_WIDTH_16BIT, I2S_SLOT_MODE_MONO),
        .gpio_cfg = {
            .clk = PDM_CLK_GPIO,
            .din = PDM_DATA_GPIO,
            .invert_flags = {
                .clk_inv = false,
            },
        },
    };

    ret = i2s_channel_init_pdm_rx_mode(rx_chan, &pdm_rx_cfg);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to init PDM RX mode: %s", esp_err_to_name(ret));
        i2s_del_channel(rx_chan);
        return ret;
    }

    // Step 3: Enable the channel
    ret = i2s_channel_enable(rx_chan);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to enable I2S channel: %s", esp_err_to_name(ret));
        i2s_del_channel(rx_chan);
        return ret;
    }

    ESP_LOGI(TAG, "PDM microphone initialised: %d Hz, %d-bit", SAMPLE_RATE, SAMPLE_BITS);
    return ESP_OK;
}
```

### 4.5 Deinitialisation

```c
void deinit_microphone(void)
{
    if (rx_chan != NULL) {
        i2s_channel_disable(rx_chan);
        i2s_del_channel(rx_chan);
        rx_chan = NULL;
        ESP_LOGI(TAG, "Microphone deinitialised");
    }
}
```

---

## Step 5: Reading Audio Data

### 5.1 Basic Read Operation

```c
esp_err_t read_audio(int16_t *buffer, size_t samples, size_t *samples_read)
{
    size_t bytes_to_read = samples * sizeof(int16_t);
    size_t bytes_read = 0;

    esp_err_t ret = i2s_channel_read(rx_chan, buffer, bytes_to_read, &bytes_read, portMAX_DELAY);

    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "I2S read failed: %s", esp_err_to_name(ret));
        *samples_read = 0;
        return ret;
    }

    *samples_read = bytes_read / sizeof(int16_t);
    return ESP_OK;
}
```

### 5.2 Understanding the Data Format

The PDM-to-PCM conversion produces signed 16-bit integers:

- Range: -32768 to +32767
- Zero = silence (roughly)
- Larger absolute values = louder

The actual values depend on microphone sensitivity and gain settings. Don't expect pristine 0 during silence—there's always noise.

---

## Step 6: Calculating Audio Levels

To verify the microphone is working, calculate and display the RMS (Root Mean Square) level.

### 6.1 RMS Calculation

```c
float calculate_rms(const int16_t *samples, size_t count)
{
    if (count == 0) return 0.0f;

    int64_t sum_squares = 0;
    for (size_t i = 0; i < count; i++) {
        int32_t sample = samples[i];
        sum_squares += sample * sample;
    }

    double mean_square = (double)sum_squares / count;
    return sqrtf(mean_square);
}
```

### 6.2 Converting to Decibels

```c
float rms_to_db(float rms)
{
    if (rms < 1.0f) {
        return -96.0f;  // Floor value
    }
    // Reference: 32768 (full scale)
    return 20.0f * log10f(rms / 32768.0f);
}
```

### 6.3 Visual Level Meter

```c
void print_level_meter(float db)
{
    // Map -60 dB to 0 dB onto 0 to 40 characters
    int bars = (int)((db + 60.0f) * (40.0f / 60.0f));
    if (bars < 0) bars = 0;
    if (bars > 40) bars = 40;

    char meter[42];
    memset(meter, ' ', 41);
    memset(meter, '#', bars);
    meter[41] = '\0';

    printf("\r[%s] %6.1f dB", meter, db);
    fflush(stdout);
}
```

---

## Step 7: Complete Audio Level Monitor

```c
#include <stdio.h>
#include <string.h>
#include <math.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/i2s_pdm.h"
#include "esp_log.h"

static const char *TAG = "audio-monitor";

#define PDM_CLK_GPIO    43
#define PDM_DATA_GPIO   44
#define SAMPLE_RATE     16000
#define SAMPLE_BITS     16
#define DMA_BUF_COUNT   4
#define DMA_BUF_LEN     1024

// Buffer for audio samples
#define AUDIO_BUF_SIZE  1024
static int16_t audio_buffer[AUDIO_BUF_SIZE];

static i2s_chan_handle_t rx_chan = NULL;

esp_err_t init_microphone(void)
{
    ESP_LOGI(TAG, "Initialising PDM microphone");

    i2s_chan_config_t chan_cfg = I2S_CHANNEL_DEFAULT_CONFIG(I2S_NUM_0, I2S_ROLE_MASTER);
    chan_cfg.dma_desc_num = DMA_BUF_COUNT;
    chan_cfg.dma_frame_num = DMA_BUF_LEN;

    ESP_ERROR_CHECK(i2s_new_channel(&chan_cfg, NULL, &rx_chan));

    i2s_pdm_rx_config_t pdm_rx_cfg = {
        .clk_cfg = I2S_PDM_RX_CLK_DEFAULT_CONFIG(SAMPLE_RATE),
        .slot_cfg = I2S_PDM_RX_SLOT_DEFAULT_CONFIG(I2S_DATA_BIT_WIDTH_16BIT, I2S_SLOT_MODE_MONO),
        .gpio_cfg = {
            .clk = PDM_CLK_GPIO,
            .din = PDM_DATA_GPIO,
            .invert_flags = {
                .clk_inv = false,
            },
        },
    };

    ESP_ERROR_CHECK(i2s_channel_init_pdm_rx_mode(rx_chan, &pdm_rx_cfg));
    ESP_ERROR_CHECK(i2s_channel_enable(rx_chan));

    ESP_LOGI(TAG, "Microphone ready");
    return ESP_OK;
}

void deinit_microphone(void)
{
    if (rx_chan) {
        i2s_channel_disable(rx_chan);
        i2s_del_channel(rx_chan);
        rx_chan = NULL;
    }
}

float calculate_rms(const int16_t *samples, size_t count)
{
    if (count == 0) return 0.0f;

    int64_t sum_squares = 0;
    for (size_t i = 0; i < count; i++) {
        int32_t sample = samples[i];
        sum_squares += sample * sample;
    }

    return sqrtf((double)sum_squares / count);
}

float rms_to_db(float rms)
{
    if (rms < 1.0f) return -96.0f;
    return 20.0f * log10f(rms / 32768.0f);
}

void print_level_meter(float db)
{
    int bars = (int)((db + 60.0f) * (40.0f / 60.0f));
    if (bars < 0) bars = 0;
    if (bars > 40) bars = 40;

    char meter[42];
    memset(meter, '-', 41);
    memset(meter, '#', bars);
    meter[41] = '\0';

    printf("\r[%s] %6.1f dB ", meter, db);
    fflush(stdout);
}

void audio_monitor_task(void *pvParameters)
{
    ESP_LOGI(TAG, "Audio monitor task started");

    while (1) {
        size_t bytes_read = 0;
        esp_err_t ret = i2s_channel_read(rx_chan, audio_buffer,
                                          sizeof(audio_buffer), &bytes_read,
                                          portMAX_DELAY);

        if (ret == ESP_OK && bytes_read > 0) {
            size_t samples = bytes_read / sizeof(int16_t);
            float rms = calculate_rms(audio_buffer, samples);
            float db = rms_to_db(rms);
            print_level_meter(db);
        } else {
            ESP_LOGE(TAG, "Read error: %s", esp_err_to_name(ret));
            vTaskDelay(pdMS_TO_TICKS(100));
        }
    }
}

void app_main(void)
{
    ESP_LOGI(TAG, "Audio Level Monitor Starting");

    if (init_microphone() != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialise microphone");
        return;
    }

    // Create the monitoring task
    xTaskCreate(audio_monitor_task, "audio_monitor", 4096, NULL, 5, NULL);

    ESP_LOGI(TAG, "Speak into the microphone to see levels");
}
```

When you run this, you should see a level meter updating in the terminal. Speak or make noise near the microphone—the meter should respond. If it doesn't, check your pin assignments and that the M5 Capsule's microphone isn't covered.

---

## Step 8: Understanding DMA Buffers

The DMA (Direct Memory Access) system is crucial for reliable audio capture. Without it, you'd need to read every sample manually, which the CPU cannot do fast enough.

### 8.1 How DMA Works

```
                   ┌─────────────────────────────────────┐
                   │           I2S Peripheral            │
                   └──────────────┬──────────────────────┘
                                  │ Audio stream
                                  ▼
    ┌─────────┐  ┌─────────┐  ┌─────────┐  ┌─────────┐
    │ Buffer 0│→ │ Buffer 1│→ │ Buffer 2│→ │ Buffer 3│→ (loops)
    └─────────┘  └─────────┘  └─────────┘  └─────────┘
         ▲                          │
         │                          │ CPU reads when buffer full
         │                          ▼
         │                    Application
         │                          │
         └──────────────────────────┘
              (Release buffer back to DMA)
```

DMA fills buffers automatically. When one is full, it moves to the next. Your code reads completed buffers. If you don't read fast enough, old data is overwritten—this is a buffer overrun.

### 8.2 Tuning DMA Parameters

```c
i2s_chan_config_t chan_cfg = I2S_CHANNEL_DEFAULT_CONFIG(I2S_NUM_0, I2S_ROLE_MASTER);

// dma_desc_num: Number of buffers in the circular chain
// More buffers = more tolerance for processing delays
// Fewer buffers = lower latency
chan_cfg.dma_desc_num = 4;  // 4 is a good starting point

// dma_frame_num: Samples per buffer
// Larger = fewer interrupts, more efficient
// Smaller = lower latency
chan_cfg.dma_frame_num = 1024;  // ~64ms at 16kHz
```

**Buffer duration calculation:**

```
Duration = dma_frame_num / sample_rate
1024 samples / 16000 Hz = 64 ms per buffer
4 buffers × 64 ms = 256 ms total buffer capacity
```

For audio recording, 256ms is plenty—you can afford occasional processing delays without losing data.

### 8.3 Detecting Buffer Overruns

ESP-IDF doesn't directly report overruns, but you can detect them:

```c
void audio_capture_task(void *pvParameters)
{
    TickType_t last_read = xTaskGetTickCount();
    const TickType_t expected_interval = pdMS_TO_TICKS(64);  // For 1024 samples at 16kHz

    while (1) {
        size_t bytes_read = 0;
        i2s_channel_read(rx_chan, audio_buffer, sizeof(audio_buffer), &bytes_read, portMAX_DELAY);

        TickType_t now = xTaskGetTickCount();
        TickType_t elapsed = now - last_read;

        // If we took much longer than expected, we might have lost data
        if (elapsed > expected_interval * 2) {
            ESP_LOGW(TAG, "Possible overrun: read took %d ms",
                     pdTICKS_TO_MS(elapsed));
        }

        last_read = now;

        // Process audio...
    }
}
```

---

## Step 9: Sample Rate and Quality Trade-offs

### 9.1 Common Sample Rates

| Sample Rate | Use Case                 | Bandwidth |
| ----------- | ------------------------ | --------- |
| 8000 Hz     | Telephone                | ~3.5 kHz  |
| 16000 Hz    | Speech recognition, VoIP | ~7 kHz    |
| 22050 Hz    | AM radio quality         | ~10 kHz   |
| 44100 Hz    | CD audio                 | ~20 kHz   |

For speech capture, 16 kHz is the sweet spot—captures all intelligible speech while keeping data rates manageable.

### 9.2 Data Rate Calculations

At 16 kHz, 16-bit mono:

```
16000 samples/sec × 2 bytes/sample = 32,000 bytes/sec = 32 KB/s
Per minute: 1.92 MB
Per hour: 115.2 MB
```

A 32 GB SD card holds approximately 280 hours of audio. You'll run out of battery long before you run out of storage.

### 9.3 PDM Clock Frequency

The PDM clock must be much higher than the sample rate. The ESP-IDF calculates this automatically, but understanding it helps with debugging:

```
PDM Clock = Sample Rate × Decimation Ratio × 2

For 16 kHz with decimation ratio of 64:
PDM Clock = 16000 × 64 × 2 = 2.048 MHz
```

The SPM1423 microphone supports 1.0 - 3.25 MHz, so we're within range.

---

## Step 10: Practical Exercise - Continuous Capture to Memory

Before writing to SD, let's capture to a memory buffer and verify the data looks sensible:

```c
#include <stdio.h>
#include <string.h>
#include <math.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/i2s_pdm.h"
#include "esp_log.h"
#include "esp_timer.h"

static const char *TAG = "capture-test";

#define PDM_CLK_GPIO    43
#define PDM_DATA_GPIO   44
#define SAMPLE_RATE     16000

static i2s_chan_handle_t rx_chan = NULL;

// Large buffer for 5 seconds of audio
#define CAPTURE_SECONDS 5
#define CAPTURE_SAMPLES (SAMPLE_RATE * CAPTURE_SECONDS)
static int16_t *capture_buffer = NULL;

esp_err_t init_microphone(void)
{
    i2s_chan_config_t chan_cfg = I2S_CHANNEL_DEFAULT_CONFIG(I2S_NUM_0, I2S_ROLE_MASTER);
    chan_cfg.dma_desc_num = 4;
    chan_cfg.dma_frame_num = 1024;

    ESP_ERROR_CHECK(i2s_new_channel(&chan_cfg, NULL, &rx_chan));

    i2s_pdm_rx_config_t pdm_rx_cfg = {
        .clk_cfg = I2S_PDM_RX_CLK_DEFAULT_CONFIG(SAMPLE_RATE),
        .slot_cfg = I2S_PDM_RX_SLOT_DEFAULT_CONFIG(I2S_DATA_BIT_WIDTH_16BIT, I2S_SLOT_MODE_MONO),
        .gpio_cfg = {
            .clk = PDM_CLK_GPIO,
            .din = PDM_DATA_GPIO,
            .invert_flags = { .clk_inv = false },
        },
    };

    ESP_ERROR_CHECK(i2s_channel_init_pdm_rx_mode(rx_chan, &pdm_rx_cfg));
    ESP_ERROR_CHECK(i2s_channel_enable(rx_chan));

    return ESP_OK;
}

void analyse_capture(const int16_t *samples, size_t count)
{
    // Find min, max, and calculate RMS
    int16_t min_val = INT16_MAX;
    int16_t max_val = INT16_MIN;
    int64_t sum_squares = 0;

    for (size_t i = 0; i < count; i++) {
        if (samples[i] < min_val) min_val = samples[i];
        if (samples[i] > max_val) max_val = samples[i];
        sum_squares += (int32_t)samples[i] * samples[i];
    }

    float rms = sqrtf((double)sum_squares / count);
    float db = 20.0f * log10f(rms / 32768.0f);

    ESP_LOGI(TAG, "Capture analysis:");
    ESP_LOGI(TAG, "  Samples: %d", count);
    ESP_LOGI(TAG, "  Min: %d", min_val);
    ESP_LOGI(TAG, "  Max: %d", max_val);
    ESP_LOGI(TAG, "  RMS: %.1f (%.1f dB)", rms, db);

    // Check for issues
    if (max_val - min_val < 100) {
        ESP_LOGW(TAG, "Very low dynamic range - microphone may not be working");
    }

    if (min_val == max_val) {
        ESP_LOGE(TAG, "All samples identical - no audio signal detected");
    }
}

void dump_samples(const int16_t *samples, size_t start, size_t count)
{
    ESP_LOGI(TAG, "Sample dump (offset %d):", start);
    for (size_t i = 0; i < count && (start + i) < CAPTURE_SAMPLES; i++) {
        printf("%6d ", samples[start + i]);
        if ((i + 1) % 10 == 0) printf("\n");
    }
    printf("\n");
}

void app_main(void)
{
    ESP_LOGI(TAG, "Audio Capture Test");

    // Allocate capture buffer (in PSRAM if available)
    capture_buffer = heap_caps_malloc(CAPTURE_SAMPLES * sizeof(int16_t), MALLOC_CAP_SPIRAM);
    if (capture_buffer == NULL) {
        // Fall back to internal RAM
        capture_buffer = malloc(CAPTURE_SAMPLES * sizeof(int16_t));
    }

    if (capture_buffer == NULL) {
        ESP_LOGE(TAG, "Failed to allocate capture buffer");
        return;
    }

    ESP_LOGI(TAG, "Allocated buffer for %d seconds of audio", CAPTURE_SECONDS);

    if (init_microphone() != ESP_OK) {
        ESP_LOGE(TAG, "Failed to init microphone");
        return;
    }

    ESP_LOGI(TAG, "Starting capture in 2 seconds... Make some noise!");
    vTaskDelay(pdMS_TO_TICKS(2000));

    ESP_LOGI(TAG, "Capturing...");

    int64_t start_time = esp_timer_get_time();

    size_t total_samples = 0;
    while (total_samples < CAPTURE_SAMPLES) {
        size_t bytes_read = 0;
        size_t samples_needed = CAPTURE_SAMPLES - total_samples;
        size_t bytes_needed = samples_needed * sizeof(int16_t);
        if (bytes_needed > 2048) bytes_needed = 2048;  // Read in chunks

        esp_err_t ret = i2s_channel_read(rx_chan, &capture_buffer[total_samples],
                                          bytes_needed, &bytes_read, portMAX_DELAY);

        if (ret == ESP_OK) {
            total_samples += bytes_read / sizeof(int16_t);
        }

        // Progress indicator
        if (total_samples % SAMPLE_RATE == 0) {
            ESP_LOGI(TAG, "  %d seconds captured", total_samples / SAMPLE_RATE);
        }
    }

    int64_t end_time = esp_timer_get_time();
    float elapsed = (end_time - start_time) / 1000000.0f;

    ESP_LOGI(TAG, "Capture complete: %d samples in %.2f seconds", total_samples, elapsed);

    // Disable microphone
    i2s_channel_disable(rx_chan);

    // Analyse the capture
    analyse_capture(capture_buffer, total_samples);

    // Dump first and last few samples
    dump_samples(capture_buffer, 0, 50);
    dump_samples(capture_buffer, total_samples - 50, 50);

    // Clean up
    free(capture_buffer);
    i2s_del_channel(rx_chan);

    ESP_LOGI(TAG, "Test complete");
}
```

---

## Checklist Before Proceeding

- [ ] I2S PDM driver initialises without errors
- [ ] Audio level meter responds to sound
- [ ] RMS values are sensible (not stuck at 0 or maximum)
- [ ] No buffer overruns during continuous capture
- [ ] Understand DMA buffer configuration
- [ ] 5-second capture completes successfully

---

## Common Problems and Solutions

| Problem                                 | Solution                                                  |
| --------------------------------------- | --------------------------------------------------------- |
| All samples are zero                    | Wrong GPIO pins, or mic not powered                       |
| All samples are the same non-zero value | Clock not running, check CLK pin                          |
| Very noisy output                       | Check for power supply noise, add capacitor on mic power  |
| Scratchy/distorted                      | Sample rate mismatch, or PDM clock too fast/slow          |
| Buffer overruns                         | Increase DMA buffer count or reduce other task priorities |
| "I2S HW not supported"                  | Wrong I2S number for ESP32-S3, use I2S_NUM_0              |

---

## Resources

- [ESP-IDF I2S Documentation](https://docs.espressif.com/projects/esp-idf/en/latest/esp32s3/api-reference/peripherals/i2s.html)
- [ESP-IDF PDM Example](https://github.com/espressif/esp-idf/tree/master/examples/peripherals/i2s/i2s_basic/i2s_pdm)
- [SPM1423 Datasheet](https://m5stack.oss-cn-shenzhen.aliyuncs.com/resource/docs/datasheet/core/SPM1423HM4H-B_datasheet_en.pdf)

---

## Next Phase

With audio streaming into memory, proceed to **Phase 4: WAV File Writing**, where we combine everything into playable audio files.
