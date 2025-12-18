# Phase 4: WAV File Writing

## Learning Objectives

By the end of this phase, you will:

- Understand the WAV file format in tedious but necessary detail
- Write properly formatted WAV headers
- Implement continuous audio recording to SD card
- Handle the WAV size fields that need updating after recording
- Use double buffering to prevent audio dropouts
- Create a complete recording system with start/stop control

---

## Step 1: The WAV File Format

WAV (Waveform Audio File Format) is Microsoft's answer to the question "how do we store audio in the most straightforward way possible while still making it slightly confusing?" It's a RIFF-based format, which means it's structured as nested chunks.

### 1.1 File Structure

```
┌─────────────────────────────────────────────────────┐
│ RIFF Chunk (contains everything)                     │
│ ┌─────────────────────────────────────────────────┐ │
│ │ "RIFF"         │ 4 bytes                        │ │
│ │ File size - 8  │ 4 bytes (little-endian)        │ │
│ │ "WAVE"         │ 4 bytes                        │ │
│ └─────────────────────────────────────────────────┘ │
│ ┌─────────────────────────────────────────────────┐ │
│ │ Format Sub-chunk                                │ │
│ │ "fmt "         │ 4 bytes (note the space)       │ │
│ │ Subchunk size  │ 4 bytes (16 for PCM)           │ │
│ │ Audio format   │ 2 bytes (1 = PCM)              │ │
│ │ Num channels   │ 2 bytes                        │ │
│ │ Sample rate    │ 4 bytes                        │ │
│ │ Byte rate      │ 4 bytes                        │ │
│ │ Block align    │ 2 bytes                        │ │
│ │ Bits/sample    │ 2 bytes                        │ │
│ └─────────────────────────────────────────────────┘ │
│ ┌─────────────────────────────────────────────────┐ │
│ │ Data Sub-chunk                                  │ │
│ │ "data"         │ 4 bytes                        │ │
│ │ Data size      │ 4 bytes                        │ │
│ │ Audio samples  │ variable                       │ │
│ └─────────────────────────────────────────────────┘ │
└─────────────────────────────────────────────────────┘
```

### 1.2 Header in C

```c
#include <stdint.h>

// WAV file header - exactly 44 bytes
typedef struct __attribute__((packed)) {
    // RIFF chunk descriptor
    char     riff_tag[4];       // "RIFF"
    uint32_t riff_size;         // File size - 8
    char     wave_tag[4];       // "WAVE"

    // Format sub-chunk
    char     fmt_tag[4];        // "fmt "
    uint32_t fmt_size;          // 16 for PCM
    uint16_t audio_format;      // 1 = PCM
    uint16_t num_channels;      // 1 = mono, 2 = stereo
    uint32_t sample_rate;       // e.g., 16000
    uint32_t byte_rate;         // sample_rate * num_channels * bits_per_sample/8
    uint16_t block_align;       // num_channels * bits_per_sample/8
    uint16_t bits_per_sample;   // 16

    // Data sub-chunk
    char     data_tag[4];       // "data"
    uint32_t data_size;         // Number of bytes of audio data
} wav_header_t;

_Static_assert(sizeof(wav_header_t) == 44, "WAV header must be 44 bytes");
```

The `__attribute__((packed))` ensures the compiler doesn't add padding bytes. The `_Static_assert` catches mistakes at compile time rather than during the "why is my audio file corrupt" debugging session.

### 1.3 Creating a Header

```c
void init_wav_header(wav_header_t *header, uint32_t sample_rate,
                     uint16_t bits_per_sample, uint16_t num_channels)
{
    // RIFF chunk
    memcpy(header->riff_tag, "RIFF", 4);
    header->riff_size = 0;  // Will be filled in later
    memcpy(header->wave_tag, "WAVE", 4);

    // Format sub-chunk
    memcpy(header->fmt_tag, "fmt ", 4);  // Note the space!
    header->fmt_size = 16;               // PCM format
    header->audio_format = 1;            // 1 = PCM (uncompressed)
    header->num_channels = num_channels;
    header->sample_rate = sample_rate;
    header->bits_per_sample = bits_per_sample;

    // Derived values
    header->block_align = num_channels * (bits_per_sample / 8);
    header->byte_rate = sample_rate * header->block_align;

    // Data sub-chunk
    memcpy(header->data_tag, "data", 4);
    header->data_size = 0;  // Will be filled in later
}
```

### 1.4 Updating Size Fields

After recording, you need to update two size fields:

```c
void finalise_wav_header(wav_header_t *header, uint32_t data_bytes)
{
    header->data_size = data_bytes;
    header->riff_size = data_bytes + sizeof(wav_header_t) - 8;
    // The -8 is because riff_size doesn't include "RIFF" or itself
}
```

---

## Step 2: Simple WAV Writer

Let's start with a basic "record N seconds to a file" implementation.

### 2.1 Project Setup

This phase combines everything from Phases 2 and 3. Your `platformio.ini` should include both SD and I2S support.

### 2.2 Complete WAV Recording Implementation

```c
#include <stdio.h>
#include <string.h>
#include <math.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/i2s_pdm.h"
#include "esp_log.h"
#include "esp_vfs_fat.h"
#include "sdmmc_cmd.h"
#include "driver/sdspi_host.h"

static const char *TAG = "wav-recorder";

// Pin definitions - M5 Capsule
#define PDM_CLK_GPIO    43
#define PDM_DATA_GPIO   44
#define SD_MOSI         14
#define SD_MISO         39
#define SD_CLK          12
#define SD_CS           11

// Audio settings
#define SAMPLE_RATE     16000
#define BITS_PER_SAMPLE 16
#define NUM_CHANNELS    1

// Buffer sizes
#define DMA_BUF_COUNT   4
#define DMA_BUF_LEN     1024
#define AUDIO_BUF_SIZE  2048

#define MOUNT_POINT "/sdcard"

// WAV header structure
typedef struct __attribute__((packed)) {
    char     riff_tag[4];
    uint32_t riff_size;
    char     wave_tag[4];
    char     fmt_tag[4];
    uint32_t fmt_size;
    uint16_t audio_format;
    uint16_t num_channels;
    uint32_t sample_rate;
    uint32_t byte_rate;
    uint16_t block_align;
    uint16_t bits_per_sample;
    char     data_tag[4];
    uint32_t data_size;
} wav_header_t;

static i2s_chan_handle_t rx_chan = NULL;
static sdmmc_card_t *sd_card = NULL;
static int16_t audio_buffer[AUDIO_BUF_SIZE];

// Initialise WAV header with placeholder sizes
void init_wav_header(wav_header_t *h)
{
    memcpy(h->riff_tag, "RIFF", 4);
    h->riff_size = 0;
    memcpy(h->wave_tag, "WAVE", 4);
    memcpy(h->fmt_tag, "fmt ", 4);
    h->fmt_size = 16;
    h->audio_format = 1;
    h->num_channels = NUM_CHANNELS;
    h->sample_rate = SAMPLE_RATE;
    h->bits_per_sample = BITS_PER_SAMPLE;
    h->block_align = NUM_CHANNELS * (BITS_PER_SAMPLE / 8);
    h->byte_rate = SAMPLE_RATE * h->block_align;
    memcpy(h->data_tag, "data", 4);
    h->data_size = 0;
}

// Update header with final sizes
void finalise_wav_header(wav_header_t *h, uint32_t data_bytes)
{
    h->data_size = data_bytes;
    h->riff_size = data_bytes + sizeof(wav_header_t) - 8;
}

esp_err_t init_sd_card(void)
{
    ESP_LOGI(TAG, "Initialising SD card");

    esp_vfs_fat_sdmmc_mount_config_t mount_config = {
        .format_if_mount_failed = false,
        .max_files = 5,
        .allocation_unit_size = 16 * 1024
    };

    spi_bus_config_t bus_cfg = {
        .mosi_io_num = SD_MOSI,
        .miso_io_num = SD_MISO,
        .sclk_io_num = SD_CLK,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = 4000,
    };

    ESP_ERROR_CHECK(spi_bus_initialize(SPI2_HOST, &bus_cfg, SDSPI_DEFAULT_DMA));

    sdspi_device_config_t slot_config = SDSPI_DEVICE_CONFIG_DEFAULT();
    slot_config.gpio_cs = SD_CS;
    slot_config.host_id = SPI2_HOST;

    return esp_vfs_fat_sdspi_mount(MOUNT_POINT, &bus_cfg, &slot_config,
                                    &mount_config, &sd_card);
}

esp_err_t init_microphone(void)
{
    ESP_LOGI(TAG, "Initialising microphone");

    i2s_chan_config_t chan_cfg = I2S_CHANNEL_DEFAULT_CONFIG(I2S_NUM_0, I2S_ROLE_MASTER);
    chan_cfg.dma_desc_num = DMA_BUF_COUNT;
    chan_cfg.dma_frame_num = DMA_BUF_LEN;

    ESP_ERROR_CHECK(i2s_new_channel(&chan_cfg, NULL, &rx_chan));

    i2s_pdm_rx_config_t pdm_cfg = {
        .clk_cfg = I2S_PDM_RX_CLK_DEFAULT_CONFIG(SAMPLE_RATE),
        .slot_cfg = I2S_PDM_RX_SLOT_DEFAULT_CONFIG(I2S_DATA_BIT_WIDTH_16BIT, I2S_SLOT_MODE_MONO),
        .gpio_cfg = {
            .clk = PDM_CLK_GPIO,
            .din = PDM_DATA_GPIO,
            .invert_flags = { .clk_inv = false },
        },
    };

    ESP_ERROR_CHECK(i2s_channel_init_pdm_rx_mode(rx_chan, &pdm_cfg));
    ESP_ERROR_CHECK(i2s_channel_enable(rx_chan));

    return ESP_OK;
}

esp_err_t record_wav(const char *filename, int duration_seconds)
{
    ESP_LOGI(TAG, "Recording %d seconds to %s", duration_seconds, filename);

    FILE *f = fopen(filename, "wb");
    if (f == NULL) {
        ESP_LOGE(TAG, "Failed to open file for writing");
        return ESP_FAIL;
    }

    // Write placeholder header
    wav_header_t header;
    init_wav_header(&header);
    fwrite(&header, sizeof(header), 1, f);

    // Calculate how many samples we need
    uint32_t total_samples = SAMPLE_RATE * duration_seconds;
    uint32_t samples_recorded = 0;
    uint32_t bytes_written = 0;

    ESP_LOGI(TAG, "Recording started...");

    while (samples_recorded < total_samples) {
        size_t bytes_read = 0;

        esp_err_t ret = i2s_channel_read(rx_chan, audio_buffer,
                                          sizeof(audio_buffer), &bytes_read,
                                          portMAX_DELAY);

        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "I2S read error: %s", esp_err_to_name(ret));
            break;
        }

        if (bytes_read > 0) {
            // Don't write more than we need
            size_t samples_in_buffer = bytes_read / sizeof(int16_t);
            size_t samples_remaining = total_samples - samples_recorded;
            size_t samples_to_write = (samples_in_buffer < samples_remaining)
                                       ? samples_in_buffer : samples_remaining;
            size_t bytes_to_write = samples_to_write * sizeof(int16_t);

            size_t written = fwrite(audio_buffer, 1, bytes_to_write, f);
            bytes_written += written;
            samples_recorded += samples_to_write;

            // Progress (every second)
            if (samples_recorded % SAMPLE_RATE < DMA_BUF_LEN) {
                ESP_LOGI(TAG, "  %d / %d seconds",
                         samples_recorded / SAMPLE_RATE, duration_seconds);
            }
        }
    }

    ESP_LOGI(TAG, "Recording complete: %d bytes", bytes_written);

    // Update header with actual size
    finalise_wav_header(&header, bytes_written);

    // Seek back to beginning and rewrite header
    fseek(f, 0, SEEK_SET);
    fwrite(&header, sizeof(header), 1, f);

    fclose(f);

    ESP_LOGI(TAG, "File saved: %s", filename);
    return ESP_OK;
}

void app_main(void)
{
    ESP_LOGI(TAG, "WAV Recorder Starting");

    if (init_sd_card() != ESP_OK) {
        ESP_LOGE(TAG, "SD card init failed");
        return;
    }

    if (init_microphone() != ESP_OK) {
        ESP_LOGE(TAG, "Microphone init failed");
        return;
    }

    // Wait a moment for things to stabilise
    vTaskDelay(pdMS_TO_TICKS(1000));

    // Record 10 seconds
    record_wav(MOUNT_POINT "/test.wav", 10);

    // Clean up
    i2s_channel_disable(rx_chan);
    i2s_del_channel(rx_chan);
    esp_vfs_fat_sdcard_unmount(MOUNT_POINT, sd_card);
    spi_bus_free(SPI2_HOST);

    ESP_LOGI(TAG, "Done. Remove SD card and play test.wav on a computer.");
}
```

---

## Step 3: Double Buffering

The simple approach above works for short recordings, but for continuous recording we need better buffer management. The problem: while we're writing one buffer to SD, we might miss incoming audio.

### 3.1 The Double Buffer Strategy

```
Time →

DMA fills:    [A][B][A][B][A][B][A][B]...
              ├──┤
                  │
                  └── When A is full, DMA switches to B
                      We process/write A

              ├─────┤
                    │
                    └── When B is full, DMA switches back to A
                        We process/write B
```

### 3.2 Implementation with FreeRTOS Queue

```c
#include "freertos/queue.h"

#define BUFFER_SIZE     4096  // Samples per buffer
#define NUM_BUFFERS     3     // Triple buffering for safety

typedef struct {
    int16_t samples[BUFFER_SIZE];
    size_t count;
} audio_buffer_t;

static QueueHandle_t audio_queue = NULL;
static audio_buffer_t buffers[NUM_BUFFERS];
static volatile int current_buffer = 0;

void audio_capture_task(void *pvParameters)
{
    while (1) {
        audio_buffer_t *buf = &buffers[current_buffer];
        size_t bytes_read = 0;

        esp_err_t ret = i2s_channel_read(rx_chan, buf->samples,
                                          sizeof(buf->samples), &bytes_read,
                                          portMAX_DELAY);

        if (ret == ESP_OK && bytes_read > 0) {
            buf->count = bytes_read / sizeof(int16_t);

            // Send buffer pointer to writer task
            // Don't block if queue is full (we'd lose data anyway)
            if (xQueueSend(audio_queue, &buf, 0) != pdTRUE) {
                ESP_LOGW(TAG, "Queue full - buffer dropped");
            }

            // Move to next buffer
            current_buffer = (current_buffer + 1) % NUM_BUFFERS;
        }
    }
}

void sd_writer_task(void *pvParameters)
{
    FILE *f = (FILE *)pvParameters;
    audio_buffer_t *buf;
    uint32_t total_written = 0;

    while (1) {
        // Wait for buffer from capture task
        if (xQueueReceive(audio_queue, &buf, pdMS_TO_TICKS(1000)) == pdTRUE) {
            size_t bytes = buf->count * sizeof(int16_t);
            size_t written = fwrite(buf->samples, 1, bytes, f);

            if (written != bytes) {
                ESP_LOGE(TAG, "Write incomplete: %d of %d", written, bytes);
            }

            total_written += written;

            // Flush periodically (not every write - too slow)
            if (total_written % (SAMPLE_RATE * 2) < BUFFER_SIZE) {
                fflush(f);
            }
        } else {
            ESP_LOGW(TAG, "No audio data received for 1 second");
        }
    }
}
```

### 3.3 Complete Double-Buffered Recorder

```c
#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"
#include "driver/i2s_pdm.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "esp_vfs_fat.h"
#include "sdmmc_cmd.h"
#include "driver/sdspi_host.h"
#include "esp_timer.h"

static const char *TAG = "wav-recorder-v2";

// Hardware pins
#define PDM_CLK_GPIO    43
#define PDM_DATA_GPIO   44
#define SD_MOSI         14
#define SD_MISO         39
#define SD_CLK          12
#define SD_CS           11
#define BUTTON_GPIO     42  // M5 Capsule button

// Audio config
#define SAMPLE_RATE     16000
#define BITS_PER_SAMPLE 16
#define NUM_CHANNELS    1

// Buffer config
#define SAMPLES_PER_BUFFER  2048
#define NUM_BUFFERS         4

#define MOUNT_POINT "/sdcard"

// WAV header
typedef struct __attribute__((packed)) {
    char     riff_tag[4];
    uint32_t riff_size;
    char     wave_tag[4];
    char     fmt_tag[4];
    uint32_t fmt_size;
    uint16_t audio_format;
    uint16_t num_channels;
    uint32_t sample_rate;
    uint32_t byte_rate;
    uint16_t block_align;
    uint16_t bits_per_sample;
    char     data_tag[4];
    uint32_t data_size;
} wav_header_t;

// Audio buffer structure
typedef struct {
    int16_t samples[SAMPLES_PER_BUFFER];
    size_t count;
} audio_buffer_t;

// Global state
static i2s_chan_handle_t rx_chan = NULL;
static sdmmc_card_t *sd_card = NULL;
static QueueHandle_t audio_queue = NULL;
static SemaphoreHandle_t recording_mutex = NULL;
static volatile bool is_recording = false;
static audio_buffer_t buffers[NUM_BUFFERS];
static int buffer_index = 0;
static TaskHandle_t capture_task_handle = NULL;
static TaskHandle_t writer_task_handle = NULL;

void init_wav_header(wav_header_t *h)
{
    memcpy(h->riff_tag, "RIFF", 4);
    h->riff_size = 0;
    memcpy(h->wave_tag, "WAVE", 4);
    memcpy(h->fmt_tag, "fmt ", 4);
    h->fmt_size = 16;
    h->audio_format = 1;
    h->num_channels = NUM_CHANNELS;
    h->sample_rate = SAMPLE_RATE;
    h->bits_per_sample = BITS_PER_SAMPLE;
    h->block_align = NUM_CHANNELS * (BITS_PER_SAMPLE / 8);
    h->byte_rate = SAMPLE_RATE * h->block_align;
    memcpy(h->data_tag, "data", 4);
    h->data_size = 0;
}

void finalise_wav_header(wav_header_t *h, uint32_t data_bytes)
{
    h->data_size = data_bytes;
    h->riff_size = data_bytes + sizeof(wav_header_t) - 8;
}

esp_err_t init_sd_card(void)
{
    esp_vfs_fat_sdmmc_mount_config_t mount_config = {
        .format_if_mount_failed = false,
        .max_files = 5,
        .allocation_unit_size = 16 * 1024
    };

    spi_bus_config_t bus_cfg = {
        .mosi_io_num = SD_MOSI,
        .miso_io_num = SD_MISO,
        .sclk_io_num = SD_CLK,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = 4000,
    };

    ESP_ERROR_CHECK(spi_bus_initialize(SPI2_HOST, &bus_cfg, SDSPI_DEFAULT_DMA));

    sdspi_device_config_t slot_config = SDSPI_DEVICE_CONFIG_DEFAULT();
    slot_config.gpio_cs = SD_CS;
    slot_config.host_id = SPI2_HOST;

    return esp_vfs_fat_sdspi_mount(MOUNT_POINT, &bus_cfg, &slot_config,
                                    &mount_config, &sd_card);
}

esp_err_t init_microphone(void)
{
    i2s_chan_config_t chan_cfg = I2S_CHANNEL_DEFAULT_CONFIG(I2S_NUM_0, I2S_ROLE_MASTER);
    chan_cfg.dma_desc_num = 4;
    chan_cfg.dma_frame_num = 1024;

    ESP_ERROR_CHECK(i2s_new_channel(&chan_cfg, NULL, &rx_chan));

    i2s_pdm_rx_config_t pdm_cfg = {
        .clk_cfg = I2S_PDM_RX_CLK_DEFAULT_CONFIG(SAMPLE_RATE),
        .slot_cfg = I2S_PDM_RX_SLOT_DEFAULT_CONFIG(I2S_DATA_BIT_WIDTH_16BIT, I2S_SLOT_MODE_MONO),
        .gpio_cfg = {
            .clk = PDM_CLK_GPIO,
            .din = PDM_DATA_GPIO,
            .invert_flags = { .clk_inv = false },
        },
    };

    ESP_ERROR_CHECK(i2s_channel_init_pdm_rx_mode(rx_chan, &pdm_cfg));
    return ESP_OK;
}

void audio_capture_task(void *pvParameters)
{
    ESP_LOGI(TAG, "Capture task started");

    while (1) {
        if (!is_recording) {
            vTaskDelay(pdMS_TO_TICKS(100));
            continue;
        }

        audio_buffer_t *buf = &buffers[buffer_index];
        size_t bytes_read = 0;

        esp_err_t ret = i2s_channel_read(rx_chan, buf->samples,
                                          sizeof(buf->samples), &bytes_read,
                                          pdMS_TO_TICKS(200));

        if (ret == ESP_OK && bytes_read > 0) {
            buf->count = bytes_read / sizeof(int16_t);

            if (xQueueSend(audio_queue, &buf, 0) != pdTRUE) {
                ESP_LOGW(TAG, "Queue full!");
            }

            buffer_index = (buffer_index + 1) % NUM_BUFFERS;
        }
    }
}

void sd_writer_task(void *pvParameters)
{
    ESP_LOGI(TAG, "Writer task started");

    char filename[64];
    FILE *f = NULL;
    wav_header_t header;
    uint32_t bytes_written = 0;
    int file_number = 0;

    while (1) {
        audio_buffer_t *buf;

        if (xQueueReceive(audio_queue, &buf, pdMS_TO_TICKS(500)) == pdTRUE) {
            if (f == NULL && is_recording) {
                // Start new file
                snprintf(filename, sizeof(filename),
                         MOUNT_POINT "/rec_%04d.wav", file_number++);
                f = fopen(filename, "wb");

                if (f) {
                    init_wav_header(&header);
                    fwrite(&header, sizeof(header), 1, f);
                    bytes_written = 0;
                    ESP_LOGI(TAG, "Started: %s", filename);
                } else {
                    ESP_LOGE(TAG, "Failed to create file");
                }
            }

            if (f != NULL) {
                size_t bytes = buf->count * sizeof(int16_t);
                size_t written = fwrite(buf->samples, 1, bytes, f);
                bytes_written += written;

                // Periodic flush (every ~1 second)
                if (bytes_written % (SAMPLE_RATE * 2) < (SAMPLES_PER_BUFFER * 2)) {
                    fflush(f);
                }
            }
        }

        // Check if recording stopped
        if (!is_recording && f != NULL) {
            // Finalise and close file
            finalise_wav_header(&header, bytes_written);
            fseek(f, 0, SEEK_SET);
            fwrite(&header, sizeof(header), 1, f);
            fclose(f);
            f = NULL;

            ESP_LOGI(TAG, "Saved: %s (%d bytes)", filename, bytes_written);
        }
    }
}

void start_recording(void)
{
    if (is_recording) return;

    ESP_LOGI(TAG, "Starting recording");
    i2s_channel_enable(rx_chan);
    is_recording = true;
}

void stop_recording(void)
{
    if (!is_recording) return;

    ESP_LOGI(TAG, "Stopping recording");
    is_recording = false;
    i2s_channel_disable(rx_chan);

    // Wait for writer to finish
    vTaskDelay(pdMS_TO_TICKS(500));
}

void button_task(void *pvParameters)
{
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << BUTTON_GPIO),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
    };
    gpio_config(&io_conf);

    bool last_state = true;
    bool current_state;

    while (1) {
        current_state = gpio_get_level(BUTTON_GPIO);

        // Button pressed (active low, with debounce)
        if (!current_state && last_state) {
            vTaskDelay(pdMS_TO_TICKS(50));  // Debounce
            current_state = gpio_get_level(BUTTON_GPIO);

            if (!current_state) {
                if (is_recording) {
                    stop_recording();
                } else {
                    start_recording();
                }
            }
        }

        last_state = current_state;
        vTaskDelay(pdMS_TO_TICKS(20));
    }
}

void app_main(void)
{
    ESP_LOGI(TAG, "Double-Buffered WAV Recorder");

    // Initialise hardware
    ESP_ERROR_CHECK(init_sd_card());
    ESP_LOGI(TAG, "SD card ready");

    ESP_ERROR_CHECK(init_microphone());
    ESP_LOGI(TAG, "Microphone ready");

    // Create queue for audio buffers
    audio_queue = xQueueCreate(NUM_BUFFERS, sizeof(audio_buffer_t *));
    recording_mutex = xSemaphoreCreateMutex();

    // Create tasks
    xTaskCreatePinnedToCore(audio_capture_task, "capture", 4096, NULL, 10,
                            &capture_task_handle, 0);
    xTaskCreatePinnedToCore(sd_writer_task, "writer", 8192, NULL, 5,
                            &writer_task_handle, 1);
    xTaskCreate(button_task, "button", 2048, NULL, 5, NULL);

    ESP_LOGI(TAG, "Press button to start/stop recording");
}
```

---

## Step 4: File Chunking for Long Recordings

Recording for hours into a single file is problematic:

1. Corruption affects all data
2. FAT32 has a 4GB file size limit
3. Seeking becomes slow

Solution: Split recordings into chunks (e.g., 5-minute files).

### 4.1 Automatic File Splitting

```c
#define CHUNK_DURATION_SEC  (5 * 60)  // 5 minutes
#define CHUNK_MAX_BYTES     (SAMPLE_RATE * sizeof(int16_t) * CHUNK_DURATION_SEC)

void sd_writer_task_chunked(void *pvParameters)
{
    char filename[64];
    FILE *f = NULL;
    wav_header_t header;
    uint32_t bytes_in_chunk = 0;
    uint32_t file_number = 0;
    uint32_t chunk_number = 0;

    while (1) {
        audio_buffer_t *buf;

        if (xQueueReceive(audio_queue, &buf, pdMS_TO_TICKS(500)) == pdTRUE) {
            // Check if we need a new file
            if (f == NULL || bytes_in_chunk >= CHUNK_MAX_BYTES) {
                // Close current file if open
                if (f != NULL) {
                    finalise_wav_header(&header, bytes_in_chunk);
                    fseek(f, 0, SEEK_SET);
                    fwrite(&header, sizeof(header), 1, f);
                    fclose(f);
                    ESP_LOGI(TAG, "Chunk %d complete", chunk_number);
                }

                // Create new file
                chunk_number++;
                snprintf(filename, sizeof(filename),
                         MOUNT_POINT "/rec_%04d_%03d.wav",
                         file_number, chunk_number);

                f = fopen(filename, "wb");
                if (f) {
                    init_wav_header(&header);
                    fwrite(&header, sizeof(header), 1, f);
                    bytes_in_chunk = 0;
                    ESP_LOGI(TAG, "Started chunk: %s", filename);
                }
            }

            // Write data
            if (f != NULL) {
                size_t bytes = buf->count * sizeof(int16_t);
                fwrite(buf->samples, 1, bytes, f);
                bytes_in_chunk += bytes;
            }
        }

        // Handle recording stop
        if (!is_recording && f != NULL) {
            finalise_wav_header(&header, bytes_in_chunk);
            fseek(f, 0, SEEK_SET);
            fwrite(&header, sizeof(header), 1, f);
            fclose(f);
            f = NULL;
            file_number++;
            chunk_number = 0;
            ESP_LOGI(TAG, "Recording session complete");
        }
    }
}
```

---

## Step 5: Timestamped Filenames

Using sequential numbers is fine for testing, but in practice you want timestamps.

### 5.1 Getting the Time

Without WiFi/NTP, the ESP32 has no idea what time it is. Options:

1. **Count from boot** — Simple, but meaningless timestamps
2. **Battery-backed RTC** — Requires additional hardware
3. **Set time at connection** — If you'll be syncing via Bluetooth anyway

For now, let's use milliseconds since boot:

```c
#include "esp_timer.h"

void get_timestamp_string(char *buf, size_t len)
{
    int64_t uptime_ms = esp_timer_get_time() / 1000;
    int hours = (uptime_ms / 3600000) % 100;
    int minutes = (uptime_ms / 60000) % 60;
    int seconds = (uptime_ms / 1000) % 60;

    snprintf(buf, len, "%02d-%02d-%02d", hours, minutes, seconds);
}

// Usage:
char timestamp[16];
get_timestamp_string(timestamp, sizeof(timestamp));
snprintf(filename, sizeof(filename), MOUNT_POINT "/rec_%s.wav", timestamp);
```

---

## Checklist Before Proceeding

- [ ] Can record a WAV file that plays correctly on a computer
- [ ] WAV header has correct values (sample rate, bit depth, size)
- [ ] Double buffering prevents audio dropouts
- [ ] Button starts and stops recording cleanly
- [ ] File is properly finalised (header updated) when recording stops
- [ ] File chunking works for long recordings
- [ ] No data corruption after abrupt power loss (within reason)

---

## Common Problems and Solutions

| Problem                    | Solution                                                       |
| -------------------------- | -------------------------------------------------------------- |
| WAV file won't play        | Check header—especially "fmt " with space, byte order          |
| Audio is choppy            | Buffer overrun—increase buffer count or reduce other task load |
| File size is 0 or 44 bytes | Header written but no data—check capture task is running       |
| Playback speed is wrong    | Sample rate mismatch between header and actual recording       |
| Clicks/pops in audio       | Buffer boundary issues—ensure buffers are contiguous           |
| SD write too slow          | Use larger writes, Class 10 card, reduce flush frequency       |

---

## Testing Your Recording

After creating a WAV file, test it properly:

1. **Check file size:** A 10-second 16kHz 16-bit mono recording should be ~320KB + 44 bytes header
2. **Play on computer:** Should play at correct speed, intelligible audio
3. **Check in audio editor:** Load in Audacity, verify sample rate shows 16000 Hz
4. **Inspect header:** Use `xxd test.wav | head` to verify header bytes

Expected header hex dump:

```
00000000: 5249 4646 xxxx xxxx 5741 5645 666d 7420  RIFF....WAVEfmt
00000010: 1000 0000 0100 0100 803e 0000 0074 0000  .........>...t..
```

Where `803e` = 0x3E80 = 16000 (little-endian sample rate).

---

## Next Phase

With recordings safely stored on SD, proceed to **Phase 5: M5 Capsule Specifics**, where we adapt everything to the actual hardware and add polish.
