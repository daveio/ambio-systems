# Step 12: Putting It All Together

## Overview

You've arrived. After eleven steps of building individual components—blinking LEDs, mounting SD cards, wrestling with I2S, crafting WAV headers, and implementing double buffers—it's time to assemble the complete audio recording system.

This is the step where everything either clicks into place with satisfying precision, or you discover that "working individually" and "working together" are entirely different concepts. Embedded systems have a particular talent for this kind of revelation.

**Context**: This is Step 12 of 15 in the M5 Capsule Audio Capture learning pathway, and the final step of Phase 4: Integration.

## What You'll Learn

- Architecting the complete recording system
- Managing component initialisation order (it matters more than you'd think)
- Coordinating button input with recording state
- Implementing clean start/stop transitions
- Creating a robust main loop structure
- Testing the integrated system end-to-end

## Prerequisites

You should have completed:

- [Step 9: Basic Audio Capture Sketch](09-basic-audio-capture-sketch.md) - Audio capture to RAM
- [Step 10: WAV File Format](10-wav-file-format.md) - Creating valid WAV files
- [Step 11: Buffered Writing](11-buffered-writing.md) - Double-buffered SD card writing

## The Complete System Architecture

Before diving into code, let's establish what we're building:

```
┌─────────────────────────────────────────────────────────────┐
│                    M5 Capsule Device                        │
├─────────────────────────────────────────────────────────────┤
│                                                             │
│   ┌─────────┐     ┌──────────────┐     ┌────────────────┐  │
│   │ Button  │────▶│ Main Control │────▶│  LED Feedback  │  │
│   └─────────┘     │    Loop      │     └────────────────┘  │
│                   └──────┬───────┘                         │
│                          │                                 │
│              ┌───────────┴───────────┐                     │
│              ▼                       ▼                     │
│   ┌──────────────────┐    ┌──────────────────┐            │
│   │  Capture Task    │    │   Write Task     │            │
│   │  (Core 0)        │    │   (Core 1)       │            │
│   │                  │    │                  │            │
│   │  I2S ──▶ Buffer  │───▶│  Buffer ──▶ SD   │            │
│   └──────────────────┘    └──────────────────┘            │
│              │                       │                     │
│              └───────────┬───────────┘                     │
│                          ▼                                 │
│                 ┌────────────────┐                         │
│                 │   WAV File     │                         │
│                 │   on SD Card   │                         │
│                 └────────────────┘                         │
└─────────────────────────────────────────────────────────────┘
```

## Step 1: The Header Files and Configuration

Let's start with a clean, well-organised structure:

```cpp
// M5 Capsule Complete Audio Recorder
// Combines all components from Steps 1-11

#include <M5Capsule.h>
#include <SD.h>
#include <driver/i2s.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/semphr.h>

// =============================================================================
// CONFIGURATION - Adjust these for your hardware
// =============================================================================

// Audio settings
#define SAMPLE_RATE       16000
#define BITS_PER_SAMPLE   16
#define CHANNELS          1
#define I2S_PORT          I2S_NUM_0

// Pin assignments - UPDATE FOR YOUR M5 CAPSULE REVISION
#define I2S_CLK_PIN       -1    // Internal for PDM, set if needed
#define I2S_DATA_PIN      GPIO_NUM_41  // PDM data pin - VERIFY THIS
#define SD_CS_PIN         GPIO_NUM_13  // SD card CS - VERIFY THIS
#define SD_MOSI_PIN       GPIO_NUM_15  // VERIFY THIS
#define SD_MISO_PIN       GPIO_NUM_39  // VERIFY THIS
#define SD_SCK_PIN        GPIO_NUM_14  // VERIFY THIS
#define BUTTON_PIN        GPIO_NUM_0   // Main button

// Buffer configuration
#define DMA_BUFFER_COUNT  8
#define DMA_BUFFER_LEN    1024
#define RING_BUFFER_SIZE  (SAMPLE_RATE * 2)  // 2 seconds of samples

// Recording limits
#define MAX_RECORDING_SECONDS  300   // 5 minute limit per file
#define MIN_FREE_SPACE_MB      50    // Stop if less than this available

// Task configuration
#define CAPTURE_TASK_STACK  4096
#define WRITE_TASK_STACK    8192
#define CAPTURE_TASK_PRIORITY  2
#define WRITE_TASK_PRIORITY    1
```

## Step 2: Global State and WAV Header

```cpp
// =============================================================================
// WAV HEADER STRUCTURE
// =============================================================================

struct __attribute__((packed)) WavHeader {
    // RIFF chunk
    char     riff[4]        = {'R', 'I', 'F', 'F'};
    uint32_t chunkSize      = 0;  // File size - 8
    char     wave[4]        = {'W', 'A', 'V', 'E'};

    // Format sub-chunk
    char     fmt[4]         = {'f', 'm', 't', ' '};
    uint32_t subchunk1Size  = 16;
    uint16_t audioFormat    = 1;  // PCM
    uint16_t numChannels    = CHANNELS;
    uint32_t sampleRate     = SAMPLE_RATE;
    uint32_t byteRate       = SAMPLE_RATE * CHANNELS * (BITS_PER_SAMPLE / 8);
    uint16_t blockAlign     = CHANNELS * (BITS_PER_SAMPLE / 8);
    uint16_t bitsPerSample  = BITS_PER_SAMPLE;

    // Data sub-chunk
    char     data[4]        = {'d', 'a', 't', 'a'};
    uint32_t dataSize       = 0;  // Actual audio data size
};

// =============================================================================
// GLOBAL STATE
// =============================================================================

// Recording state
enum class RecorderState {
    IDLE,
    STARTING,
    RECORDING,
    STOPPING,
    ERROR
};

volatile RecorderState currentState = RecorderState::IDLE;
volatile bool captureRunning = false;
volatile bool writeRunning = false;

// Ring buffer
int16_t* ringBuffer = nullptr;
volatile size_t writeIndex = 0;
volatile size_t readIndex = 0;
SemaphoreHandle_t bufferMutex = nullptr;

// File handling
File wavFile;
String currentFilename;
uint32_t samplesRecorded = 0;

// Task handles
TaskHandle_t captureTaskHandle = nullptr;
TaskHandle_t writeTaskHandle = nullptr;

// Error tracking
String lastError = "";
```

## Step 3: Ring Buffer Operations

These should be familiar from Step 11, but here they're integrated into the complete system:

```cpp
// =============================================================================
// RING BUFFER OPERATIONS
// =============================================================================

size_t ringBufferAvailable() {
    size_t w = writeIndex;
    size_t r = readIndex;

    if (w >= r) {
        return w - r;
    } else {
        return RING_BUFFER_SIZE - r + w;
    }
}

size_t ringBufferFree() {
    return RING_BUFFER_SIZE - 1 - ringBufferAvailable();
}

bool ringBufferWrite(const int16_t* data, size_t count) {
    if (count > ringBufferFree()) {
        return false;  // Would overflow
    }

    for (size_t i = 0; i < count; i++) {
        ringBuffer[writeIndex] = data[i];
        writeIndex = (writeIndex + 1) % RING_BUFFER_SIZE;
    }

    return true;
}

size_t ringBufferRead(int16_t* data, size_t maxCount) {
    size_t available = ringBufferAvailable();
    size_t toRead = (maxCount < available) ? maxCount : available;

    for (size_t i = 0; i < toRead; i++) {
        data[i] = ringBuffer[readIndex];
        readIndex = (readIndex + 1) % RING_BUFFER_SIZE;
    }

    return toRead;
}

void ringBufferClear() {
    writeIndex = 0;
    readIndex = 0;
}
```

## Step 4: Hardware Initialisation

The order matters. Get it wrong and you'll spend hours debugging phantom issues:

```cpp
// =============================================================================
// INITIALISATION FUNCTIONS
// =============================================================================

bool initSerial() {
    Serial.begin(115200);
    delay(1000);  // Give USB CDC time to enumerate
    Serial.println("\n\n=== M5 Capsule Audio Recorder ===");
    Serial.println("Firmware version: 1.0.0");
    return true;
}

bool initM5() {
    Serial.print("Initialising M5Capsule... ");
    M5Capsule.begin();
    Serial.println("OK");
    return true;
}

bool initSDCard() {
    Serial.print("Initialising SD card... ");

    SPI.begin(SD_SCK_PIN, SD_MISO_PIN, SD_MOSI_PIN, SD_CS_PIN);

    if (!SD.begin(SD_CS_PIN)) {
        Serial.println("FAILED - No card or mount error");
        lastError = "SD card mount failed";
        return false;
    }

    uint64_t cardSize = SD.cardSize() / (1024 * 1024);
    uint64_t freeSpace = (SD.totalBytes() - SD.usedBytes()) / (1024 * 1024);

    Serial.printf("OK - %lluMB total, %lluMB free\n", cardSize, freeSpace);

    if (freeSpace < MIN_FREE_SPACE_MB) {
        Serial.println("WARNING: Low disk space!");
        lastError = "Low disk space";
        return false;
    }

    return true;
}

bool initI2S() {
    Serial.print("Initialising I2S... ");

    i2s_config_t i2s_config = {
        .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX | I2S_MODE_PDM),
        .sample_rate = SAMPLE_RATE,
        .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
        .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT,
        .communication_format = I2S_COMM_FORMAT_STAND_I2S,
        .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
        .dma_buf_count = DMA_BUFFER_COUNT,
        .dma_buf_len = DMA_BUFFER_LEN,
        .use_apll = false,
        .tx_desc_auto_clear = false,
        .fixed_mclk = 0
    };

    i2s_pin_config_t pin_config = {
        .bck_io_num = I2S_PIN_NO_CHANGE,
        .ws_io_num = I2S_CLK_PIN,
        .data_out_num = I2S_PIN_NO_CHANGE,
        .data_in_num = I2S_DATA_PIN
    };

    esp_err_t err = i2s_driver_install(I2S_PORT, &i2s_config, 0, NULL);
    if (err != ESP_OK) {
        Serial.printf("FAILED - Driver install error: %d\n", err);
        lastError = "I2S driver install failed";
        return false;
    }

    err = i2s_set_pin(I2S_PORT, &pin_config);
    if (err != ESP_OK) {
        Serial.printf("FAILED - Pin config error: %d\n", err);
        lastError = "I2S pin config failed";
        i2s_driver_uninstall(I2S_PORT);
        return false;
    }

    Serial.println("OK");
    return true;
}

bool initBuffers() {
    Serial.print("Allocating ring buffer... ");

    // Try PSRAM first, fall back to internal RAM
    ringBuffer = (int16_t*)ps_malloc(RING_BUFFER_SIZE * sizeof(int16_t));

    if (ringBuffer == nullptr) {
        Serial.print("PSRAM unavailable, trying internal... ");
        ringBuffer = (int16_t*)malloc(RING_BUFFER_SIZE * sizeof(int16_t));
    }

    if (ringBuffer == nullptr) {
        Serial.println("FAILED - Out of memory");
        lastError = "Buffer allocation failed";
        return false;
    }

    bufferMutex = xSemaphoreCreateMutex();
    if (bufferMutex == nullptr) {
        Serial.println("FAILED - Mutex creation failed");
        free(ringBuffer);
        ringBuffer = nullptr;
        lastError = "Mutex creation failed";
        return false;
    }

    ringBufferClear();
    Serial.printf("OK - %d bytes\n", RING_BUFFER_SIZE * sizeof(int16_t));
    return true;
}
```

## Step 5: File Operations

```cpp
// =============================================================================
// FILE OPERATIONS
// =============================================================================

String generateFilename() {
    // Create timestamped filename: REC_YYYYMMDD_HHMMSS.wav
    // Without RTC, we use sequential numbering

    int fileNum = 0;
    String filename;

    do {
        fileNum++;
        filename = String("/REC_") + String(fileNum, DEC) + ".wav";
    } while (SD.exists(filename) && fileNum < 9999);

    return filename;
}

bool createWavFile() {
    currentFilename = generateFilename();

    Serial.printf("Creating file: %s\n", currentFilename.c_str());

    wavFile = SD.open(currentFilename, FILE_WRITE);
    if (!wavFile) {
        Serial.println("ERROR: Failed to create file");
        lastError = "File creation failed";
        return false;
    }

    // Write placeholder header
    WavHeader header;
    size_t written = wavFile.write((uint8_t*)&header, sizeof(header));

    if (written != sizeof(header)) {
        Serial.println("ERROR: Failed to write header");
        lastError = "Header write failed";
        wavFile.close();
        SD.remove(currentFilename);
        return false;
    }

    wavFile.flush();
    samplesRecorded = 0;

    Serial.println("File created, header written");
    return true;
}

bool finaliseWavFile() {
    if (!wavFile) {
        return false;
    }

    Serial.printf("Finalising file: %lu samples\n", samplesRecorded);

    // Calculate sizes
    uint32_t dataSize = samplesRecorded * sizeof(int16_t);
    uint32_t fileSize = dataSize + sizeof(WavHeader) - 8;

    // Update header with actual sizes
    WavHeader header;
    header.chunkSize = fileSize;
    header.dataSize = dataSize;

    // Seek to beginning and rewrite header
    wavFile.seek(0);
    wavFile.write((uint8_t*)&header, sizeof(header));
    wavFile.flush();
    wavFile.close();

    float durationSecs = (float)samplesRecorded / SAMPLE_RATE;
    Serial.printf("Recording complete: %.1f seconds, %lu bytes\n",
                  durationSecs, dataSize + sizeof(WavHeader));

    return true;
}
```

## Step 6: The FreeRTOS Tasks

```cpp
// =============================================================================
// FREERTOS TASKS
// =============================================================================

void captureTask(void* parameter) {
    Serial.println("[Capture] Task started on core " + String(xPortGetCoreID()));

    const size_t readSize = DMA_BUFFER_LEN;
    int16_t* captureBuffer = (int16_t*)malloc(readSize * sizeof(int16_t));

    if (captureBuffer == nullptr) {
        Serial.println("[Capture] ERROR: Buffer allocation failed");
        captureRunning = false;
        vTaskDelete(NULL);
        return;
    }

    size_t bytesRead;
    uint32_t overflows = 0;

    while (captureRunning) {
        esp_err_t err = i2s_read(I2S_PORT, captureBuffer,
                                  readSize * sizeof(int16_t),
                                  &bytesRead, portMAX_DELAY);

        if (err != ESP_OK || bytesRead == 0) {
            continue;
        }

        size_t samplesRead = bytesRead / sizeof(int16_t);

        if (xSemaphoreTake(bufferMutex, pdMS_TO_TICKS(10)) == pdTRUE) {
            if (!ringBufferWrite(captureBuffer, samplesRead)) {
                overflows++;
                if (overflows % 100 == 1) {
                    Serial.printf("[Capture] WARNING: Buffer overflow #%lu\n", overflows);
                }
            }
            xSemaphoreGive(bufferMutex);
        }
    }

    free(captureBuffer);
    Serial.printf("[Capture] Task ended. Overflows: %lu\n", overflows);
    vTaskDelete(NULL);
}

void writeTask(void* parameter) {
    Serial.println("[Write] Task started on core " + String(xPortGetCoreID()));

    const size_t writeChunkSize = 512;  // Samples per write
    int16_t* writeBuffer = (int16_t*)malloc(writeChunkSize * sizeof(int16_t));

    if (writeBuffer == nullptr) {
        Serial.println("[Write] ERROR: Buffer allocation failed");
        writeRunning = false;
        vTaskDelete(NULL);
        return;
    }

    uint32_t maxRecordingSamples = MAX_RECORDING_SECONDS * SAMPLE_RATE;
    uint32_t lastProgressReport = 0;

    while (writeRunning) {
        size_t available = ringBufferAvailable();

        if (available >= writeChunkSize) {
            if (xSemaphoreTake(bufferMutex, pdMS_TO_TICKS(10)) == pdTRUE) {
                size_t read = ringBufferRead(writeBuffer, writeChunkSize);
                xSemaphoreGive(bufferMutex);

                if (read > 0 && wavFile) {
                    wavFile.write((uint8_t*)writeBuffer, read * sizeof(int16_t));
                    samplesRecorded += read;

                    // Progress report every 5 seconds
                    uint32_t currentSecond = samplesRecorded / SAMPLE_RATE;
                    if (currentSecond >= lastProgressReport + 5) {
                        lastProgressReport = currentSecond;
                        Serial.printf("[Write] Progress: %lu seconds recorded\n", currentSecond);
                        wavFile.flush();  // Periodic flush for safety
                    }

                    // Check recording limit
                    if (samplesRecorded >= maxRecordingSamples) {
                        Serial.println("[Write] Maximum recording length reached");
                        currentState = RecorderState::STOPPING;
                    }
                }
            }
        } else {
            // No data to write, yield briefly
            vTaskDelay(pdMS_TO_TICKS(5));
        }
    }

    // Drain remaining buffer
    while (ringBufferAvailable() > 0) {
        if (xSemaphoreTake(bufferMutex, pdMS_TO_TICKS(100)) == pdTRUE) {
            size_t read = ringBufferRead(writeBuffer, writeChunkSize);
            xSemaphoreGive(bufferMutex);

            if (read > 0 && wavFile) {
                wavFile.write((uint8_t*)writeBuffer, read * sizeof(int16_t));
                samplesRecorded += read;
            }
        }
    }

    free(writeBuffer);
    Serial.println("[Write] Task ended");
    vTaskDelete(NULL);
}
```

## Step 7: Recording State Machine

```cpp
// =============================================================================
// RECORDING CONTROL
// =============================================================================

bool startRecording() {
    if (currentState != RecorderState::IDLE) {
        Serial.println("Cannot start: not in IDLE state");
        return false;
    }

    currentState = RecorderState::STARTING;
    Serial.println("\n>>> STARTING RECORDING <<<");

    // Clear any old data
    ringBufferClear();

    // Create new file
    if (!createWavFile()) {
        currentState = RecorderState::ERROR;
        return false;
    }

    // Start tasks
    captureRunning = true;
    writeRunning = true;

    xTaskCreatePinnedToCore(
        captureTask,
        "CaptureTask",
        CAPTURE_TASK_STACK,
        NULL,
        CAPTURE_TASK_PRIORITY,
        &captureTaskHandle,
        0  // Core 0
    );

    xTaskCreatePinnedToCore(
        writeTask,
        "WriteTask",
        WRITE_TASK_STACK,
        NULL,
        WRITE_TASK_PRIORITY,
        &writeTaskHandle,
        1  // Core 1
    );

    currentState = RecorderState::RECORDING;

    // Visual feedback
    M5Capsule.dis.drawpix(0xFF0000);  // Red = recording

    Serial.println("Recording started");
    return true;
}

void stopRecording() {
    if (currentState != RecorderState::RECORDING &&
        currentState != RecorderState::STOPPING) {
        return;
    }

    currentState = RecorderState::STOPPING;
    Serial.println("\n>>> STOPPING RECORDING <<<");

    // Signal tasks to stop
    captureRunning = false;

    // Wait for capture task to finish
    vTaskDelay(pdMS_TO_TICKS(100));

    // Let write task drain the buffer
    writeRunning = false;

    // Wait for write task to finish
    vTaskDelay(pdMS_TO_TICKS(500));

    // Finalise the file
    finaliseWavFile();

    // Visual feedback
    M5Capsule.dis.drawpix(0x00FF00);  // Green = idle/ready

    currentState = RecorderState::IDLE;
    Serial.println("Recording stopped\n");
}
```

## Step 8: Button Handling

```cpp
// =============================================================================
// BUTTON HANDLING
// =============================================================================

unsigned long lastButtonPress = 0;
const unsigned long DEBOUNCE_MS = 200;
bool buttonWasPressed = false;

void handleButton() {
    bool buttonPressed = M5Capsule.Btn.isPressed();

    // Debounce
    if (millis() - lastButtonPress < DEBOUNCE_MS) {
        return;
    }

    // Detect rising edge (button just pressed)
    if (buttonPressed && !buttonWasPressed) {
        lastButtonPress = millis();

        Serial.println("Button pressed!");

        if (currentState == RecorderState::IDLE) {
            startRecording();
        } else if (currentState == RecorderState::RECORDING) {
            stopRecording();
        }
    }

    buttonWasPressed = buttonPressed;
}
```

## Step 9: The Main Program

```cpp
// =============================================================================
// MAIN PROGRAM
// =============================================================================

void setup() {
    // Initialisation order matters!
    bool allOK = true;

    allOK &= initSerial();
    allOK &= initM5();
    allOK &= initSDCard();
    allOK &= initI2S();
    allOK &= initBuffers();

    if (allOK) {
        Serial.println("\n=== All systems initialised ===");
        Serial.println("Press button to start/stop recording");
        Serial.println("================================\n");

        // Ready indicator
        M5Capsule.dis.drawpix(0x00FF00);  // Green = ready
        currentState = RecorderState::IDLE;
    } else {
        Serial.println("\n!!! INITIALISATION FAILED !!!");
        Serial.println("Error: " + lastError);
        Serial.println("!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n");

        // Error indicator
        M5Capsule.dis.drawpix(0xFF00FF);  // Purple = error
        currentState = RecorderState::ERROR;
    }
}

void loop() {
    // Update M5 button state
    M5Capsule.update();

    // Handle button input
    if (currentState != RecorderState::ERROR) {
        handleButton();
    }

    // Status LED pulse while recording
    if (currentState == RecorderState::RECORDING) {
        static unsigned long lastPulse = 0;
        if (millis() - lastPulse > 500) {
            lastPulse = millis();
            // Pulse between bright and dim red
            static bool bright = true;
            M5Capsule.dis.drawpix(bright ? 0xFF0000 : 0x400000);
            bright = !bright;
        }
    }

    // Small delay to prevent busy-waiting
    delay(10);
}
```

## Step 10: Testing the Complete System

### Test Sequence

1. **Power-On Test**
   - Upload firmware
   - Open serial monitor at 115200 baud
   - Verify all components initialise (should see "OK" for each)
   - LED should show green (ready)

2. **Recording Test**
   - Press button once
   - LED should turn red (recording)
   - Serial should show "STARTING RECORDING"
   - Speak into microphone for 10-15 seconds
   - Press button again
   - LED should return to green
   - Serial should show recording duration and file size

3. **File Verification**
   - Remove SD card safely
   - Insert into computer
   - Locate `REC_1.wav` (or similar)
   - Verify file plays correctly in any audio player
   - Check audio quality and volume level

### Expected Serial Output

```
=== M5 Capsule Audio Recorder ===
Firmware version: 1.0.0
Initialising M5Capsule... OK
Initialising SD card... OK - 16000MB total, 14500MB free
Initialising I2S... OK
Allocating ring buffer... OK - 64000 bytes

=== All systems initialised ===
Press button to start/stop recording
================================

Button pressed!

>>> STARTING RECORDING <<<
Creating file: /REC_1.wav
File created, header written
[Capture] Task started on core 0
[Write] Task started on core 1
Recording started

[Write] Progress: 5 seconds recorded
[Write] Progress: 10 seconds recorded

Button pressed!

>>> STOPPING RECORDING <<<
[Capture] Task ended. Overflows: 0
[Write] Task ended
Finalising file: 240000 samples
Recording complete: 15.0 seconds, 480044 bytes
Recording stopped
```

## Troubleshooting

| Symptom                  | Likely Cause                      | Solution                                       |
| ------------------------ | --------------------------------- | ---------------------------------------------- |
| LED stays purple         | Initialisation failed             | Check serial output for which component failed |
| No audio in file         | I2S pin configuration wrong       | Verify I2S_DATA_PIN matches your hardware      |
| Audio distorted/clipping | Gain too high or PDM config wrong | Check i2s_config settings                      |
| File corrupt/won't play  | Header not updated                | Ensure finaliseWavFile() completes             |
| Buffer overflows         | Write task too slow               | Increase write priority or buffer size         |
| Recording cuts short     | MAX_RECORDING_SECONDS limit       | Increase limit or implement file splitting     |
| Random crashes           | Stack overflow                    | Increase task stack sizes                      |

## Complete Source File

For convenience, here's the full source as a single file you can copy into your project:

**Create as: `src/main.cpp`**

The complete code is approximately 450 lines. All the sections above, when combined in order, form the complete program.

## Verification Checklist

Before moving on, verify:

- [ ] Device powers on with green LED
- [ ] Button press toggles recording (green ↔ red LED)
- [ ] Serial output shows all phases working
- [ ] WAV files are created on SD card
- [ ] Files play correctly on computer
- [ ] Audio quality is acceptable for speech
- [ ] No buffer overflows during normal recording
- [ ] Multiple recording sessions work correctly

## What's Next

You have a working audio recorder. It functions. It records. It saves files. Victory, surely?

Well, yes. But also: what happens when the battery runs low? When the SD card fills up? When someone trips over the device mid-recording? When cosmic rays flip a bit in your buffer?

In [Step 13: Power Management](13-power-management.md), we'll address energy efficiency—because a wearable device that needs charging every 20 minutes is really more of a "stationary device that you carry to a power outlet".

## Quick Reference

### LED States

| Colour        | State      |
| ------------- | ---------- |
| Green         | Ready/Idle |
| Red (pulsing) | Recording  |
| Purple        | Error      |

### Button Actions

| Current State | Button Press | Result                        |
| ------------- | ------------ | ----------------------------- |
| IDLE          | Press        | Start recording               |
| RECORDING     | Press        | Stop recording                |
| ERROR         | Press        | Nothing (fix the error first) |

### File Naming

Files are named `REC_N.wav` where N increments with each recording (1, 2, 3, etc.).

### Memory Usage

- Ring buffer: ~64KB (in PSRAM if available)
- Capture task stack: 4KB
- Write task stack: 8KB
- Total: ~80KB minimum
