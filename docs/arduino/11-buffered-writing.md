# 4.2 Buffered Writing

## Overview

The naive approach to recording audio is simple: read samples, write to SD card, repeat. Unfortunately, SD cards were not designed with the real-time demands of audio in mind. They have their own thoughts about when they'd like to accept data, and those thoughts occasionally involve taking a 100-millisecond holiday to reorganise their internal block structure.

During which time, audio samples keep arriving at 16,000 per second, piling up like frustrated commuters at a signal failure.

This is where buffering comes in — the art of holding audio in RAM while the SD card decides whether or not it's ready to participate.

## What You'll Learn

- Why SD cards have latency spikes
- Double buffering (ping-pong) technique
- Ring buffer implementation
- Using FreeRTOS tasks for parallel operation

## Prerequisites

- Completed [Step 4.1: WAV File Format](./10-wav-file-format.md)
- Successfully recorded a WAV file
- An appreciation for concurrency problems

---

## Step 1: The Problem With Direct Writing

Consider this loop:

```cpp
while (recording) {
    i2s_read(I2S_NUM_0, buffer, sizeof(buffer), &bytesRead, timeout);
    file.write(buffer, bytesRead);
}
```

This works until it doesn't. SD cards periodically pause for:

- **Wear levelling:** Redistributing writes across the flash
- **Block erasing:** Flash must erase before writing (and erasing is slow)
- **FAT updates:** Filesystem bookkeeping
- **Random cosmic events:** Who knows, really

These pauses can exceed 100ms. At 16kHz, 100ms is 1600 samples. If your buffer only holds 1024 samples, you've lost audio.

```
Timeline:
═══════════════════════════════════════════════════════════════►

I2S:     [1024 samples][1024 samples][1024 samples][1024 samples]...
               ↓             ↓             ↓             ↓

SD card:    [write]       [write]    [........PAUSE........][write]
                                            ↑
                                    SAMPLES LOST HERE
```

## Step 2: Double Buffering

The classic solution: use two buffers. While one is being written to SD, the other fills from I2S.

```
Timeline:
═══════════════════════════════════════════════════════════════►

I2S:     [Fill A]   [Fill B]   [Fill A]   [Fill B]...
              ↓          ↓          ↓          ↓
SD:       [---]    [Write A]  [Write B]  [Write A]...
```

Even if the SD write takes longer than expected, you have the other buffer as runway:

```cpp
#define BUFFER_SAMPLES 4096
#define NUM_BUFFERS 2

int16_t buffers[NUM_BUFFERS][BUFFER_SAMPLES];
volatile int activeBuffer = 0;
volatile int samplesInBuffer[NUM_BUFFERS] = {0, 0};
volatile bool bufferReady[NUM_BUFFERS] = {false, false};

void captureTask(void* param) {
    while (recording) {
        int buf = activeBuffer;
        size_t bytesRead = 0;

        // Read into active buffer
        esp_err_t err = i2s_read(
            I2S_NUM_0,
            &buffers[buf][samplesInBuffer[buf]],
            (BUFFER_SAMPLES - samplesInBuffer[buf]) * sizeof(int16_t),
            &bytesRead,
            pdMS_TO_TICKS(50)
        );

        if (err == ESP_OK) {
            samplesInBuffer[buf] += bytesRead / sizeof(int16_t);

            // Buffer full? Switch to other buffer
            if (samplesInBuffer[buf] >= BUFFER_SAMPLES) {
                bufferReady[buf] = true;
                activeBuffer = (buf + 1) % NUM_BUFFERS;
                samplesInBuffer[activeBuffer] = 0;
            }
        }
    }
}

void writeTask(void* param) {
    while (recording) {
        for (int buf = 0; buf < NUM_BUFFERS; buf++) {
            if (bufferReady[buf]) {
                // Write this buffer to SD
                file.write((uint8_t*)buffers[buf], samplesInBuffer[buf] * sizeof(int16_t));
                bufferReady[buf] = false;
            }
        }
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}
```

### Buffer Size Calculation

How big should buffers be? Big enough to survive SD latency spikes.

For 100ms latency tolerance at 16kHz:

```
100ms × 16000 samples/sec = 1600 samples × 2 bytes = 3200 bytes per buffer
```

With two buffers, you can tolerate up to 200ms. With four buffers, 400ms. More buffers = more tolerance, but more RAM usage.

## Step 3: Ring Buffer (More Sophisticated)

A ring buffer (circular buffer) is more flexible than fixed ping-pong buffers:

```cpp
class RingBuffer {
private:
    int16_t* buffer;
    size_t capacity;
    volatile size_t head;  // Write position
    volatile size_t tail;  // Read position
    volatile size_t count; // Samples in buffer

public:
    RingBuffer(size_t samples) : capacity(samples), head(0), tail(0), count(0) {
        buffer = new int16_t[samples];
    }

    ~RingBuffer() {
        delete[] buffer;
    }

    // Add samples to buffer (producer side)
    size_t write(const int16_t* data, size_t samples) {
        size_t written = 0;

        while (written < samples && count < capacity) {
            buffer[head] = data[written];
            head = (head + 1) % capacity;
            count++;
            written++;
        }

        return written;
    }

    // Remove samples from buffer (consumer side)
    size_t read(int16_t* data, size_t samples) {
        size_t readCount = 0;

        while (readCount < samples && count > 0) {
            data[readCount] = buffer[tail];
            tail = (tail + 1) % capacity;
            count--;
            readCount++;
        }

        return readCount;
    }

    // How many samples can we write?
    size_t available() const { return capacity - count; }

    // How many samples can we read?
    size_t stored() const { return count; }

    // Buffer fullness as percentage
    int percentFull() const { return (count * 100) / capacity; }
};
```

### Using the Ring Buffer

```cpp
RingBuffer audioBuffer(32768);  // 2 seconds at 16kHz

void captureLoop() {
    int16_t tempBuffer[1024];
    size_t bytesRead = 0;

    i2s_read(I2S_NUM_0, tempBuffer, sizeof(tempBuffer), &bytesRead, pdMS_TO_TICKS(50));
    size_t samplesRead = bytesRead / sizeof(int16_t);

    size_t written = audioBuffer.write(tempBuffer, samplesRead);

    if (written < samplesRead) {
        // Buffer overflow! Data lost.
        static int overflows = 0;
        overflows++;
        Serial.printf("OVERFLOW! Lost %d samples (total: %d)\n",
                      samplesRead - written, overflows);
    }
}

void writeLoop() {
    if (audioBuffer.stored() >= 8192) {  // Write in 8K chunks
        int16_t tempBuffer[8192];
        size_t read = audioBuffer.read(tempBuffer, 8192);
        file.write((uint8_t*)tempBuffer, read * sizeof(int16_t));
    }
}
```

## Step 4: FreeRTOS Tasks

The ESP32 has two cores. Why not use both?

- **Core 0:** Audio capture (time-critical)
- **Core 1:** SD card writing (can tolerate delays)

```cpp
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

TaskHandle_t captureTaskHandle;
TaskHandle_t writeTaskHandle;

volatile bool isRecording = false;
RingBuffer* audioBuffer;
File audioFile;

void audioCaptureTask(void* param) {
    int16_t tempBuf[1024];

    while (true) {
        if (!isRecording) {
            vTaskDelay(pdMS_TO_TICKS(100));
            continue;
        }

        size_t bytesRead = 0;
        esp_err_t err = i2s_read(I2S_NUM_0, tempBuf, sizeof(tempBuf),
                                  &bytesRead, pdMS_TO_TICKS(50));

        if (err == ESP_OK && bytesRead > 0) {
            size_t samples = bytesRead / sizeof(int16_t);
            size_t written = audioBuffer->write(tempBuf, samples);

            if (written < samples) {
                Serial.printf("OVERFLOW: lost %d samples\n", samples - written);
            }
        }

        // Yield to other tasks
        vTaskDelay(1);
    }
}

void sdWriteTask(void* param) {
    int16_t writeBuf[4096];

    while (true) {
        if (!isRecording) {
            vTaskDelay(pdMS_TO_TICKS(100));
            continue;
        }

        // Write when we have enough data
        if (audioBuffer->stored() >= 4096) {
            size_t samples = audioBuffer->read(writeBuf, 4096);
            audioFile.write((uint8_t*)writeBuf, samples * sizeof(int16_t));
        }

        // Also periodically flush
        static unsigned long lastFlush = 0;
        if (millis() - lastFlush > 5000) {
            audioFile.flush();
            lastFlush = millis();
        }

        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

void setupTasks() {
    // Create audio capture task on Core 0
    xTaskCreatePinnedToCore(
        audioCaptureTask,   // Function
        "AudioCapture",     // Name
        4096,               // Stack size
        NULL,               // Parameters
        2,                  // Priority (higher = more important)
        &captureTaskHandle, // Handle
        0                   // Core 0
    );

    // Create SD write task on Core 1
    xTaskCreatePinnedToCore(
        sdWriteTask,
        "SDWrite",
        8192,               // Larger stack for SD operations
        NULL,
        1,                  // Lower priority than capture
        &writeTaskHandle,
        1                   // Core 1
    );
}
```

### Task Priorities

| Task          | Priority    | Why                         |
| ------------- | ----------- | --------------------------- |
| Audio capture | High (2)    | Must not miss samples       |
| SD writing    | Lower (1)   | Can fall behind temporarily |
| Main loop     | Default (1) | UI, button handling         |

## Step 5: Monitoring Buffer Health

In production, you want to know if you're close to overflowing:

```cpp
void monitorTask(void* param) {
    while (true) {
        if (isRecording) {
            int fill = audioBuffer->percentFull();

            if (fill > 80) {
                Serial.printf("WARNING: Buffer %d%% full!\n", fill);
            }

            // LED indicator
            if (fill > 90) {
                // Red - critical
                setLED(CRGB::Red);
            } else if (fill > 70) {
                // Yellow - warning
                setLED(CRGB::Yellow);
            } else {
                // Green - healthy
                setLED(CRGB::Green);
            }
        }

        vTaskDelay(pdMS_TO_TICKS(500));
    }
}
```

## Step 6: Complete Buffered Recording System

Here's a full implementation:

```cpp
#include <Arduino.h>
#include <M5Capsule.h>
#include <SD.h>
#include <SPI.h>
#include <driver/i2s.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

// Pin configuration - UPDATE THESE
#define SD_CS    4
#define SD_MOSI  15
#define SD_MISO  2
#define SD_SCK   14
#define I2S_CLK  43
#define I2S_DATA 44

// Audio parameters
#define SAMPLE_RATE     16000
#define BUFFER_SAMPLES  32768  // 2 seconds buffer

// Ring buffer class
class RingBuffer {
private:
    int16_t* buffer;
    size_t capacity;
    volatile size_t head, tail, count;
    portMUX_TYPE mux = portMUX_INITIALIZER_UNLOCKED;

public:
    RingBuffer(size_t samples) : capacity(samples), head(0), tail(0), count(0) {
        buffer = (int16_t*)heap_caps_malloc(samples * sizeof(int16_t), MALLOC_CAP_INTERNAL);
    }

    size_t write(const int16_t* data, size_t samples) {
        portENTER_CRITICAL(&mux);
        size_t written = 0;
        while (written < samples && count < capacity) {
            buffer[head] = data[written++];
            head = (head + 1) % capacity;
            count++;
        }
        portEXIT_CRITICAL(&mux);
        return written;
    }

    size_t read(int16_t* data, size_t samples) {
        portENTER_CRITICAL(&mux);
        size_t readCount = 0;
        while (readCount < samples && count > 0) {
            data[readCount++] = buffer[tail];
            tail = (tail + 1) % capacity;
            count--;
        }
        portEXIT_CRITICAL(&mux);
        return readCount;
    }

    size_t stored() { return count; }
    int percentFull() { return (count * 100) / capacity; }
};

// WAV header
struct WavHeader {
    char riff[4] = {'R','I','F','F'};
    uint32_t chunkSize;
    char wave[4] = {'W','A','V','E'};
    char fmt[4] = {'f','m','t',' '};
    uint32_t subchunk1Size = 16;
    uint16_t audioFormat = 1;
    uint16_t numChannels = 1;
    uint32_t sampleRate = SAMPLE_RATE;
    uint32_t byteRate = SAMPLE_RATE * 2;
    uint16_t blockAlign = 2;
    uint16_t bitsPerSample = 16;
    char data[4] = {'d','a','t','a'};
    uint32_t subchunk2Size;
} __attribute__((packed));

// Global state
RingBuffer* audioBuffer;
File audioFile;
volatile bool isRecording = false;
volatile uint32_t totalBytesWritten = 0;
volatile uint32_t droppedSamples = 0;
TaskHandle_t captureTask, writeTask;
SPIClass spi = SPIClass(HSPI);

// Audio capture task (Core 0, high priority)
void audioCaptureTaskFunc(void* param) {
    int16_t tempBuf[512];

    while (true) {
        if (!isRecording) {
            vTaskDelay(pdMS_TO_TICKS(50));
            continue;
        }

        size_t bytesRead = 0;
        esp_err_t err = i2s_read(I2S_NUM_0, tempBuf, sizeof(tempBuf),
                                  &bytesRead, pdMS_TO_TICKS(25));

        if (err == ESP_OK && bytesRead > 0) {
            size_t samples = bytesRead / sizeof(int16_t);
            size_t written = audioBuffer->write(tempBuf, samples);

            if (written < samples) {
                droppedSamples += (samples - written);
            }
        }

        taskYIELD();
    }
}

// SD write task (Core 1, lower priority)
void sdWriteTaskFunc(void* param) {
    int16_t writeBuf[2048];
    unsigned long lastFlush = 0;

    while (true) {
        if (!isRecording) {
            vTaskDelay(pdMS_TO_TICKS(50));
            continue;
        }

        // Write when buffer has enough data
        while (audioBuffer->stored() >= 2048) {
            size_t samples = audioBuffer->read(writeBuf, 2048);
            size_t bytes = samples * sizeof(int16_t);
            audioFile.write((uint8_t*)writeBuf, bytes);
            totalBytesWritten += bytes;
        }

        // Periodic flush
        if (millis() - lastFlush > 5000) {
            audioFile.flush();
            lastFlush = millis();
        }

        vTaskDelay(pdMS_TO_TICKS(5));
    }
}

void startRecording() {
    // Generate filename
    char filename[32];
    snprintf(filename, sizeof(filename), "/rec_%lu.wav", millis() / 1000);

    audioFile = SD.open(filename, FILE_WRITE);
    if (!audioFile) {
        Serial.println("Failed to create file!");
        return;
    }

    // Write placeholder header
    WavHeader header = {};
    audioFile.write((uint8_t*)&header, sizeof(header));

    totalBytesWritten = 0;
    droppedSamples = 0;
    isRecording = true;

    Serial.printf("Recording started: %s\n", filename);
}

void stopRecording() {
    isRecording = false;
    vTaskDelay(pdMS_TO_TICKS(100));  // Let tasks finish

    // Drain remaining buffer
    int16_t writeBuf[2048];
    while (audioBuffer->stored() > 0) {
        size_t samples = audioBuffer->read(writeBuf, 2048);
        audioFile.write((uint8_t*)writeBuf, samples * sizeof(int16_t));
        totalBytesWritten += samples * sizeof(int16_t);
    }

    // Update WAV header
    WavHeader header = {};
    header.subchunk2Size = totalBytesWritten;
    header.chunkSize = 36 + totalBytesWritten;
    audioFile.seek(0);
    audioFile.write((uint8_t*)&header, sizeof(header));
    audioFile.close();

    Serial.printf("Recording stopped. %u bytes, %u samples dropped\n",
                  totalBytesWritten + 44, droppedSamples);
}

void setup() {
    M5.begin();
    Serial.begin(115200);
    while (!Serial) delay(10);

    Serial.println("\n=== Buffered Recording System ===\n");

    // Initialise SD
    spi.begin(SD_SCK, SD_MISO, SD_MOSI, SD_CS);
    SD.begin(SD_CS, spi, 4000000);

    // Initialise I2S
    i2s_config_t cfg = {
        .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX | I2S_MODE_PDM),
        .sample_rate = SAMPLE_RATE,
        .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
        .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT,
        .communication_format = I2S_COMM_FORMAT_STAND_I2S,
        .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
        .dma_buf_count = 8,
        .dma_buf_len = 512,
        .use_apll = false,
    };
    i2s_pin_config_t pins = {
        .bck_io_num = I2S_PIN_NO_CHANGE,
        .ws_io_num = I2S_CLK,
        .data_out_num = I2S_PIN_NO_CHANGE,
        .data_in_num = I2S_DATA
    };
    i2s_driver_install(I2S_NUM_0, &cfg, 0, NULL);
    i2s_set_pin(I2S_NUM_0, &pins);

    // Create ring buffer
    audioBuffer = new RingBuffer(BUFFER_SAMPLES);

    // Create tasks
    xTaskCreatePinnedToCore(audioCaptureTaskFunc, "Capture", 4096, NULL, 2, &captureTask, 0);
    xTaskCreatePinnedToCore(sdWriteTaskFunc, "Write", 8192, NULL, 1, &writeTask, 1);

    Serial.println("Press button to start/stop recording\n");
}

void loop() {
    M5.update();

    if (M5.BtnA.wasPressed()) {
        if (isRecording) {
            stopRecording();
        } else {
            startRecording();
        }
    }

    // Status display
    static unsigned long lastStatus = 0;
    if (isRecording && millis() - lastStatus > 1000) {
        Serial.printf("Buffer: %d%% | Written: %u KB | Dropped: %u\n",
                      audioBuffer->percentFull(),
                      totalBytesWritten / 1024,
                      droppedSamples);
        lastStatus = millis();
    }

    delay(10);
}
```

---

## Verification

Before moving on, confirm:

- [ ] Recording runs for extended periods (5+ minutes) without buffer overflow
- [ ] Dropped sample count stays at zero during normal operation
- [ ] Buffer never reaches 100% during recording
- [ ] Files play back without audible glitches

If you see occasional overflows, increase buffer size or reduce write chunk size.

---

## What's Next

You have a robust recording system. In [Step 4.3: Putting It All Together](./12-putting-it-all-together.md), we'll add the finishing touches: start/stop logic, status indication, error recovery, and a clean state machine for managing the recording lifecycle.

---

## Quick Reference

```cpp
// FreeRTOS task creation
xTaskCreatePinnedToCore(
    function,       // Task function
    "Name",         // Task name
    stackSize,      // Stack in bytes
    params,         // Parameters (void*)
    priority,       // Higher = more urgent
    &handle,        // Task handle
    coreID          // 0 or 1
);

// Critical sections (thread-safe)
portMUX_TYPE mux = portMUX_INITIALIZER_UNLOCKED;
portENTER_CRITICAL(&mux);
// ... critical code ...
portEXIT_CRITICAL(&mux);

// Task control
vTaskDelay(pdMS_TO_TICKS(ms));  // Sleep
taskYIELD();                     // Give other tasks a chance
vTaskDelete(handle);             // Terminate task
```
