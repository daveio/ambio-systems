# 4.1 WAV File Format

## Overview

You've captured audio. It's sitting in RAM as a sequence of 16-bit signed integers, which is profoundly meaningful to computers and profoundly meaningless to humans who just want to double-click a file and hear something.

WAV is the solution — a file format that wraps your raw audio samples in a header that describes what they are. It's the audio equivalent of a shipping label on a box. The box contains your actual stuff; the label tells handlers what's inside.

## What You'll Learn

- The structure of a WAV file
- Writing valid WAV headers
- The relationship between file size and header values
- How to create files that any audio player can open

## Prerequisites

- Completed [Step 3.3: Basic Audio Capture Sketch](./09-basic-audio-capture-sketch.md)
- Understanding that audio is just numbers, and files are just organised bytes
- A hex editor (optional, but satisfying)

---

## Step 1: WAV File Anatomy

A WAV file consists of two parts:

1. **Header** — 44 bytes describing the audio format
2. **Data** — The actual audio samples

That's it. It's not complicated. Which is probably why it's been around since 1991 and shows no signs of going away.

```
┌──────────────────────────────────────┐
│           WAV File Structure         │
├──────────────────────────────────────┤
│  RIFF Header (12 bytes)              │
│    - "RIFF" identifier               │
│    - File size - 8                   │
│    - "WAVE" format                   │
├──────────────────────────────────────┤
│  Format Chunk (24 bytes)             │
│    - "fmt " identifier               │
│    - Chunk size (16)                 │
│    - Audio format (1 = PCM)          │
│    - Number of channels              │
│    - Sample rate                     │
│    - Byte rate                       │
│    - Block align                     │
│    - Bits per sample                 │
├──────────────────────────────────────┤
│  Data Chunk Header (8 bytes)         │
│    - "data" identifier               │
│    - Data size                       │
├──────────────────────────────────────┤
│                                      │
│  Audio Samples (variable)            │
│    - Raw PCM data                    │
│                                      │
└──────────────────────────────────────┘
```

Total header size: 12 + 24 + 8 = 44 bytes.

## Step 2: The Header in Detail

Every WAV file begins with these 44 bytes. Here's what each field means:

| Offset | Size | Field         | Description                                |
| ------ | ---- | ------------- | ------------------------------------------ |
| 0      | 4    | ChunkID       | Always "RIFF" (0x52494646)                 |
| 4      | 4    | ChunkSize     | File size - 8                              |
| 8      | 4    | Format        | Always "WAVE" (0x57415645)                 |
| 12     | 4    | Subchunk1ID   | Always "fmt " (0x666D7420)                 |
| 16     | 4    | Subchunk1Size | 16 for PCM                                 |
| 20     | 2    | AudioFormat   | 1 for PCM (uncompressed)                   |
| 22     | 2    | NumChannels   | 1 = mono, 2 = stereo                       |
| 24     | 4    | SampleRate    | e.g., 16000, 44100                         |
| 28     | 4    | ByteRate      | SampleRate × NumChannels × BitsPerSample/8 |
| 32     | 2    | BlockAlign    | NumChannels × BitsPerSample/8              |
| 34     | 2    | BitsPerSample | e.g., 8, 16, 24, 32                        |
| 36     | 4    | Subchunk2ID   | Always "data" (0x64617461)                 |
| 40     | 4    | Subchunk2Size | NumSamples × NumChannels × BitsPerSample/8 |
| 44     | ...  | Data          | The actual audio samples                   |

### Important Relationships

```
ByteRate = SampleRate × NumChannels × BitsPerSample ÷ 8
BlockAlign = NumChannels × BitsPerSample ÷ 8
ChunkSize = 36 + Subchunk2Size
Subchunk2Size = NumSamples × BlockAlign
```

For our pendant (16 kHz, 16-bit, mono):

```
ByteRate = 16000 × 1 × 16 ÷ 8 = 32000 bytes/second
BlockAlign = 1 × 16 ÷ 8 = 2 bytes/sample
```

## Step 3: Writing a WAV Header in Code

Here's a structure and function to generate valid WAV headers:

```cpp
// WAV header structure (44 bytes)
struct WavHeader {
    // RIFF chunk
    char riff[4] = {'R', 'I', 'F', 'F'};
    uint32_t chunkSize;        // File size - 8
    char wave[4] = {'W', 'A', 'V', 'E'};

    // Format chunk
    char fmt[4] = {'f', 'm', 't', ' '};
    uint32_t subchunk1Size = 16;  // PCM = 16
    uint16_t audioFormat = 1;      // PCM = 1
    uint16_t numChannels;
    uint32_t sampleRate;
    uint32_t byteRate;
    uint16_t blockAlign;
    uint16_t bitsPerSample;

    // Data chunk
    char data[4] = {'d', 'a', 't', 'a'};
    uint32_t subchunk2Size;       // Data size
} __attribute__((packed));

// Create a WAV header for given parameters
WavHeader createWavHeader(uint32_t sampleRate, uint16_t bitsPerSample,
                          uint16_t channels, uint32_t dataSize) {
    WavHeader header;

    header.numChannels = channels;
    header.sampleRate = sampleRate;
    header.bitsPerSample = bitsPerSample;
    header.blockAlign = channels * bitsPerSample / 8;
    header.byteRate = sampleRate * header.blockAlign;
    header.subchunk2Size = dataSize;
    header.chunkSize = 36 + dataSize;

    return header;
}

// Write header to file
bool writeWavHeader(File& file, uint32_t sampleRate, uint16_t bitsPerSample,
                    uint16_t channels, uint32_t dataSize) {
    WavHeader header = createWavHeader(sampleRate, bitsPerSample, channels, dataSize);

    size_t written = file.write((uint8_t*)&header, sizeof(header));
    return written == sizeof(header);
}
```

### The `__attribute__((packed))` Directive

This tells the compiler not to add padding between struct members. Without it, the compiler might insert gaps to align fields to word boundaries, which would make our 44-byte header into something larger and incompatible.

## Step 4: The Size Problem

Here's a snag: to write the header, you need to know the data size. But you don't know the data size until you've finished recording. And you can't write the header until you know the size.

Solutions:

### Option A: Write Header First, Update Later

1. Write a placeholder header with size = 0
2. Record all the audio
3. Seek back to the beginning
4. Rewrite the header with the correct size

```cpp
void finishWavFile(File& file, uint32_t dataSize) {
    // Calculate the correct header values
    WavHeader header = createWavHeader(SAMPLE_RATE, 16, 1, dataSize);

    // Seek to beginning
    file.seek(0);

    // Overwrite the header
    file.write((uint8_t*)&header, sizeof(header));

    // Return to end for any further operations
    file.seek(file.size());
}
```

### Option B: Write Header Last (Reverse File)

Not recommended. Some filesystems don't appreciate this approach.

### Option C: Use Streaming Format (Like FLAC)

FLAC and other formats support unknown lengths. But then you're not using WAV, and playback compatibility decreases.

### Option D: Fixed-Duration Chunks

Record in fixed-duration chunks (e.g., 1 minute each) where you know the size in advance:

```cpp
#define CHUNK_DURATION_SEC 60
#define DATA_SIZE_PER_CHUNK (SAMPLE_RATE * CHANNELS * BITS_PER_SAMPLE / 8 * CHUNK_DURATION_SEC)

// Header for a 60-second chunk is always the same
WavHeader chunkHeader = createWavHeader(SAMPLE_RATE, BITS_PER_SAMPLE, CHANNELS, DATA_SIZE_PER_CHUNK);
```

**Option A is most common** for embedded recording. We'll use it.

## Step 5: Complete WAV Writing Example

Here's a sketch that records 5 seconds and saves it as a proper WAV file:

```cpp
#include <Arduino.h>
#include <M5Capsule.h>
#include <SD.h>
#include <SPI.h>
#include <driver/i2s.h>

// Hardware pins - UPDATE THESE
#define SD_CS    4
#define SD_MOSI  15
#define SD_MISO  2
#define SD_SCK   14
#define I2S_CLK  43
#define I2S_DATA 44

// Audio parameters
#define SAMPLE_RATE     16000
#define BITS_PER_SAMPLE 16
#define CHANNELS        1
#define RECORD_SECONDS  5

// WAV header structure
struct WavHeader {
    char riff[4] = {'R', 'I', 'F', 'F'};
    uint32_t chunkSize;
    char wave[4] = {'W', 'A', 'V', 'E'};
    char fmt[4] = {'f', 'm', 't', ' '};
    uint32_t subchunk1Size = 16;
    uint16_t audioFormat = 1;
    uint16_t numChannels;
    uint32_t sampleRate;
    uint32_t byteRate;
    uint16_t blockAlign;
    uint16_t bitsPerSample;
    char data[4] = {'d', 'a', 't', 'a'};
    uint32_t subchunk2Size;
} __attribute__((packed));

SPIClass spi = SPIClass(HSPI);

WavHeader createHeader(uint32_t dataSize) {
    WavHeader h;
    h.numChannels = CHANNELS;
    h.sampleRate = SAMPLE_RATE;
    h.bitsPerSample = BITS_PER_SAMPLE;
    h.blockAlign = CHANNELS * BITS_PER_SAMPLE / 8;
    h.byteRate = SAMPLE_RATE * h.blockAlign;
    h.subchunk2Size = dataSize;
    h.chunkSize = 36 + dataSize;
    return h;
}

void setupSD() {
    spi.begin(SD_SCK, SD_MISO, SD_MOSI, SD_CS);
    if (!SD.begin(SD_CS, spi, 4000000)) {
        Serial.println("SD mount failed!");
        while (true) delay(1000);
    }
    Serial.println("SD card mounted.");
}

void setupI2S() {
    i2s_config_t cfg = {
        .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX | I2S_MODE_PDM),
        .sample_rate = SAMPLE_RATE,
        .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
        .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT,
        .communication_format = I2S_COMM_FORMAT_STAND_I2S,
        .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
        .dma_buf_count = 4,
        .dma_buf_len = 1024,
        .use_apll = false,
        .tx_desc_auto_clear = false,
        .fixed_mclk = 0
    };

    i2s_pin_config_t pins = {
        .bck_io_num = I2S_PIN_NO_CHANGE,
        .ws_io_num = I2S_CLK,
        .data_out_num = I2S_PIN_NO_CHANGE,
        .data_in_num = I2S_DATA
    };

    ESP_ERROR_CHECK(i2s_driver_install(I2S_NUM_0, &cfg, 0, NULL));
    ESP_ERROR_CHECK(i2s_set_pin(I2S_NUM_0, &pins));
    Serial.println("I2S configured.");
}

void recordWav() {
    // Generate filename
    char filename[32];
    snprintf(filename, sizeof(filename), "/rec_%lu.wav", millis() / 1000);

    Serial.printf("Recording to: %s\n", filename);

    // Open file
    File file = SD.open(filename, FILE_WRITE);
    if (!file) {
        Serial.println("Failed to create file!");
        return;
    }

    // Write placeholder header (we'll update it later)
    WavHeader header = createHeader(0);
    file.write((uint8_t*)&header, sizeof(header));

    // Record audio
    uint32_t bytesWritten = 0;
    uint32_t targetBytes = SAMPLE_RATE * CHANNELS * (BITS_PER_SAMPLE / 8) * RECORD_SECONDS;
    int16_t buffer[512];

    Serial.println("Recording...");
    unsigned long startTime = millis();

    while (bytesWritten < targetBytes) {
        size_t bytesRead = 0;
        esp_err_t err = i2s_read(I2S_NUM_0, buffer, sizeof(buffer), &bytesRead, pdMS_TO_TICKS(100));

        if (err == ESP_OK && bytesRead > 0) {
            file.write((uint8_t*)buffer, bytesRead);
            bytesWritten += bytesRead;
        }
    }

    unsigned long duration = millis() - startTime;
    Serial.printf("Recorded %u bytes in %lu ms\n", bytesWritten, duration);

    // Update header with correct size
    header = createHeader(bytesWritten);
    file.seek(0);
    file.write((uint8_t*)&header, sizeof(header));

    // Close file
    file.close();

    Serial.printf("Saved: %s (%u bytes total)\n", filename, bytesWritten + 44);
    Serial.println("Remove SD card and play the file on your computer!");
}

void setup() {
    M5.begin();
    Serial.begin(115200);
    while (!Serial) delay(10);

    Serial.println("\n=== WAV Recording Test ===\n");

    setupSD();
    setupI2S();

    Serial.println("\nPress button to record 5 seconds...\n");
}

void loop() {
    M5.update();

    if (M5.BtnA.wasPressed()) {
        recordWav();
        Serial.println("\nPress button to record again...\n");
    }

    delay(10);
}
```

## Step 6: Verifying Your WAV File

After recording, remove the SD card and plug it into your computer. The file should:

1. **Open in any audio player** — Windows Media Player, VLC, QuickTime, Audacity
2. **Show correct duration** — About 5 seconds
3. **Play back your voice** — Or whatever you recorded
4. **Display correct properties** — 16 kHz, 16-bit, mono

### Using a Hex Editor

If you want to verify the header manually:

```
Offset  Hex                              ASCII
00000   52 49 46 46 94 FE 00 00          RIFF....
00008   57 41 56 45 66 6D 74 20          WAVEfmt
00010   10 00 00 00 01 00 01 00          ........
00018   80 3E 00 00 00 7D 00 00          .>...}..
00020   02 00 10 00 64 61 74 61          ....data
00028   70 FE 00 00 [audio data...]      p...
```

The values should match your configuration.

---

## Verification

Before moving on, confirm:

- [ ] WAV file is created on SD card
- [ ] File opens in standard audio players
- [ ] Duration matches what you recorded
- [ ] Audio sounds like what you said (not silence, not noise)

---

## What's Next

You can record a WAV file. Excellent. But this naive approach has a problem: if you're recording for an hour and the battery dies at minute 59, you lose everything because the header was never updated.

In [Step 4.2: Buffered Writing](./11-buffered-writing.md), we'll implement proper buffering strategies to handle continuous recording without losing data to power failures, SD card latency, or the general malevolence of embedded systems.

---

## Quick Reference

```cpp
// WAV header for 16kHz, 16-bit, mono
// Total: 44 bytes header + N bytes audio data

struct WavHeader {
    // RIFF chunk (12 bytes)
    char riff[4] = {'R','I','F','F'};
    uint32_t chunkSize;              // = 36 + dataSize
    char wave[4] = {'W','A','V','E'};

    // Format chunk (24 bytes)
    char fmt[4] = {'f','m','t',' '};
    uint32_t subchunk1Size = 16;
    uint16_t audioFormat = 1;        // PCM
    uint16_t numChannels = 1;        // Mono
    uint32_t sampleRate = 16000;
    uint32_t byteRate = 32000;       // sampleRate × channels × bitsPerSample/8
    uint16_t blockAlign = 2;         // channels × bitsPerSample/8
    uint16_t bitsPerSample = 16;

    // Data chunk (8 bytes)
    char data[4] = {'d','a','t','a'};
    uint32_t subchunk2Size;          // = numSamples × blockAlign
} __attribute__((packed));
```
