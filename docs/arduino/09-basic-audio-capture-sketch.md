# 3.3 Basic Audio Capture Sketch

## Overview

You've configured I2S. Audio samples are appearing in RAM, flickering briefly into existence before being overwritten by the next batch. It's like performance art, except less pretentious and more useful.

Now we'll capture a fixed duration of audio into a buffer, examine it, and confirm that what we're receiving actually resembles sound rather than random noise or the memory contents of someone else's program.

## What You'll Learn

- Capturing multiple seconds of audio into memory
- Analysing audio for DC offset and noise floor
- Simple signal validation techniques
- Preparing for the transition to file-based recording

## Prerequisites

- Completed [Step 3.2: ESP-IDF I2S Driver](./08-esp-idf-i2s-driver.md)
- I2S initialising successfully
- Sample values appearing that respond to sound

---

## Step 1: How Much Audio Can We Store?

The ESP32-S3 has about 512 KB of RAM (varies by model and configuration). Let's see how much audio that holds:

| Duration   | Sample Rate | Bit Depth | Size   |
| ---------- | ----------- | --------- | ------ |
| 1 second   | 16 kHz      | 16-bit    | 32 KB  |
| 5 seconds  | 16 kHz      | 16-bit    | 160 KB |
| 10 seconds | 16 kHz      | 16-bit    | 320 KB |
| 16 seconds | 16 kHz      | 16-bit    | 512 KB |

So we can comfortably store about 10 seconds in RAM, leaving headroom for the rest of the program. This is useful for testing but not for continuous recording — that requires writing to SD as we capture.

## Step 2: Capture Buffer Setup

Let's create a sketch that captures 5 seconds of audio:

```cpp
#include <Arduino.h>
#include <M5Capsule.h>
#include <driver/i2s.h>

// Pin configuration - UPDATE FOR YOUR HARDWARE
#define I2S_MIC_CLK  GPIO_NUM_43
#define I2S_MIC_DATA GPIO_NUM_44

// Audio parameters
#define SAMPLE_RATE     16000
#define BITS_PER_SAMPLE 16
#define CHANNELS        1
#define DMA_BUF_COUNT   4
#define DMA_BUF_LEN     1024

// Recording parameters
#define RECORD_DURATION_SEC 5
#define TOTAL_SAMPLES       (SAMPLE_RATE * RECORD_DURATION_SEC)
#define BUFFER_SIZE_BYTES   (TOTAL_SAMPLES * sizeof(int16_t))

// Allocate buffer in PSRAM if available, otherwise regular RAM
int16_t* audioBuffer = nullptr;
size_t samplesRecorded = 0;
bool recordingComplete = false;

void setupI2S() {
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

    ESP_ERROR_CHECK(i2s_driver_install(I2S_NUM_0, &i2s_config, 0, NULL));
    ESP_ERROR_CHECK(i2s_set_pin(I2S_NUM_0, &pin_config));
}

void setup() {
    M5.begin();
    Serial.begin(115200);
    while (!Serial) delay(10);

    Serial.println("\n=== Audio Capture Test ===\n");
    Serial.printf("Recording duration: %d seconds\n", RECORD_DURATION_SEC);
    Serial.printf("Sample rate: %d Hz\n", SAMPLE_RATE);
    Serial.printf("Total samples: %d\n", TOTAL_SAMPLES);
    Serial.printf("Buffer size: %d bytes\n", BUFFER_SIZE_BYTES);

    // Allocate audio buffer
    // Try PSRAM first (if available), then fall back to regular RAM
    if (psramFound()) {
        audioBuffer = (int16_t*)ps_malloc(BUFFER_SIZE_BYTES);
        Serial.println("Using PSRAM for audio buffer");
    } else {
        audioBuffer = (int16_t*)malloc(BUFFER_SIZE_BYTES);
        Serial.println("Using internal RAM for audio buffer");
    }

    if (audioBuffer == nullptr) {
        Serial.println("ERROR: Failed to allocate audio buffer!");
        while (true) delay(1000);
    }

    Serial.printf("Free heap after allocation: %d bytes\n", ESP.getFreeHeap());

    setupI2S();

    Serial.println("\nReady to record.");
    Serial.println("Press the button to start recording...\n");
}

void recordAudio() {
    Serial.println("Recording started!");
    Serial.println("Speak now...\n");

    samplesRecorded = 0;
    unsigned long startTime = millis();

    while (samplesRecorded < TOTAL_SAMPLES) {
        size_t bytesRead = 0;
        size_t samplesToRead = min((size_t)(TOTAL_SAMPLES - samplesRecorded), (size_t)DMA_BUF_LEN);

        esp_err_t err = i2s_read(
            I2S_NUM_0,
            &audioBuffer[samplesRecorded],
            samplesToRead * sizeof(int16_t),
            &bytesRead,
            pdMS_TO_TICKS(100)
        );

        if (err != ESP_OK) {
            Serial.printf("Read error: %s\n", esp_err_to_name(err));
            continue;
        }

        samplesRecorded += bytesRead / sizeof(int16_t);

        // Progress indicator
        int progress = (samplesRecorded * 100) / TOTAL_SAMPLES;
        static int lastProgress = -1;
        if (progress != lastProgress && progress % 10 == 0) {
            Serial.printf("  %d%% (%d samples)\n", progress, samplesRecorded);
            lastProgress = progress;
        }
    }

    unsigned long duration = millis() - startTime;
    Serial.printf("\nRecording complete in %lu ms\n", duration);
    Serial.printf("Expected duration: %d ms\n", RECORD_DURATION_SEC * 1000);

    recordingComplete = true;
}

void analyseAudio() {
    if (!recordingComplete || samplesRecorded == 0) {
        Serial.println("No audio to analyse");
        return;
    }

    Serial.println("\n=== Audio Analysis ===\n");

    // Calculate statistics
    int64_t sum = 0;
    int16_t minSample = 32767;
    int16_t maxSample = -32768;
    int64_t sumSquares = 0;
    int zeroCrossings = 0;

    for (size_t i = 0; i < samplesRecorded; i++) {
        int16_t sample = audioBuffer[i];

        sum += sample;
        sumSquares += (int64_t)sample * sample;

        if (sample < minSample) minSample = sample;
        if (sample > maxSample) maxSample = sample;

        // Count zero crossings (indicates frequency content)
        if (i > 0) {
            if ((audioBuffer[i-1] >= 0 && sample < 0) ||
                (audioBuffer[i-1] < 0 && sample >= 0)) {
                zeroCrossings++;
            }
        }
    }

    // DC offset (should be near 0 for good audio)
    double dcOffset = (double)sum / samplesRecorded;

    // RMS (root mean square) - measure of signal level
    double rms = sqrt((double)sumSquares / samplesRecorded);

    // Estimated fundamental frequency from zero crossings
    double estFreq = (double)zeroCrossings / 2 / RECORD_DURATION_SEC;

    Serial.printf("Samples analysed: %d\n", samplesRecorded);
    Serial.printf("Min sample: %d\n", minSample);
    Serial.printf("Max sample: %d\n", maxSample);
    Serial.printf("Peak-to-peak: %d\n", maxSample - minSample);
    Serial.printf("DC offset: %.2f\n", dcOffset);
    Serial.printf("RMS level: %.2f\n", rms);
    Serial.printf("Zero crossings: %d\n", zeroCrossings);
    Serial.printf("Est. frequency: %.1f Hz\n", estFreq);

    // Quality assessment
    Serial.println("\n=== Quality Assessment ===\n");

    // Check DC offset
    if (abs(dcOffset) > 1000) {
        Serial.println("⚠️  High DC offset detected. May need correction.");
    } else if (abs(dcOffset) > 100) {
        Serial.println("⚡ Moderate DC offset. Acceptable for voice.");
    } else {
        Serial.println("✓ DC offset looks good.");
    }

    // Check signal level
    if (rms < 100) {
        Serial.println("⚠️  Very low signal level. Check mic connection.");
    } else if (rms > 20000) {
        Serial.println("⚠️  Very high signal level. Possible clipping.");
    } else {
        Serial.println("✓ Signal level looks reasonable.");
    }

    // Check for clipping
    int clippedSamples = 0;
    for (size_t i = 0; i < samplesRecorded; i++) {
        if (audioBuffer[i] >= 32767 || audioBuffer[i] <= -32768) {
            clippedSamples++;
        }
    }

    if (clippedSamples > 0) {
        double clipPercent = (double)clippedSamples / samplesRecorded * 100;
        Serial.printf("⚠️  Clipping detected: %d samples (%.2f%%)\n", clippedSamples, clipPercent);
    } else {
        Serial.println("✓ No clipping detected.");
    }

    // Print first few samples for manual inspection
    Serial.println("\n=== First 50 Samples ===\n");
    for (int i = 0; i < 50 && i < samplesRecorded; i++) {
        Serial.printf("%6d ", audioBuffer[i]);
        if ((i + 1) % 10 == 0) Serial.println();
    }
    Serial.println();
}

void printWaveform() {
    if (!recordingComplete || samplesRecorded == 0) {
        Serial.println("No audio to display");
        return;
    }

    Serial.println("\n=== Waveform Preview (decimated) ===\n");

    // Print a text-based waveform, heavily decimated
    int step = samplesRecorded / 80;  // 80 lines of output

    for (int i = 0; i < 80; i++) {
        int idx = i * step;
        int16_t sample = audioBuffer[idx];

        // Normalise to 0-40 range
        int level = map(sample, -32768, 32767, 0, 40);

        Serial.printf("%5d: ", idx);
        for (int j = 0; j < level; j++) Serial.print(" ");
        Serial.println("█");
    }
}

void loop() {
    M5.update();

    if (M5.BtnA.wasPressed() && !recordingComplete) {
        recordAudio();
        analyseAudio();
        printWaveform();

        Serial.println("\n=== Done ===");
        Serial.println("Press button to record again (will overwrite).");
    }

    if (M5.BtnA.wasPressed() && recordingComplete) {
        recordingComplete = false;
        Serial.println("\nReady to record again...");
    }

    delay(10);
}
```

## Step 3: Understanding the Analysis

### DC Offset

Ideally, audio samples should oscillate around zero. A DC offset means the average isn't zero — the waveform is shifted up or down.

```
Good (DC offset ≈ 0):          Bad (DC offset = 5000):
       ╱╲                             ╱╲
      ╱  ╲                           ╱  ╲
─────╱────╲────── 0              ╱────╲────────────── 5000
    ╱      ╲                    ╱      ╲
   ╱        ╲                  ╱        ╲
                         ────────────────────── 0
```

Many microphones have some DC offset. It doesn't affect audio quality much but wastes dynamic range. We can remove it later by subtracting the mean.

### RMS Level

Root Mean Square is a measure of signal power. Higher RMS = louder audio.

| RMS Level    | What It Means                  |
| ------------ | ------------------------------ |
| < 100        | Very quiet, possibly no signal |
| 100 - 1000   | Background noise, quiet speech |
| 1000 - 5000  | Normal conversation            |
| 5000 - 15000 | Loud speech, music             |
| > 15000      | Very loud, check for clipping  |

### Clipping

Clipping occurs when the signal exceeds the maximum representable value. The waveform gets "clipped" at the top and bottom:

```
Normal:              Clipped:
    ╱╲                  ──╱────
   ╱  ╲                  ╱
  ╱    ╲                ╱
 ╱      ╲              ╱
╱        ╲            ╱
                          ────╲──
                               ╲
```

Clipped audio sounds harsh and distorted. If you're seeing clipping, either the sound source is too loud or the microphone gain is too high.

## Step 4: Removing DC Offset

If your audio has significant DC offset, here's how to remove it:

```cpp
void removeDCOffset(int16_t* buffer, size_t length) {
    // Calculate mean
    int64_t sum = 0;
    for (size_t i = 0; i < length; i++) {
        sum += buffer[i];
    }
    int16_t dcOffset = sum / length;

    // Subtract from all samples
    for (size_t i = 0; i < length; i++) {
        int32_t corrected = buffer[i] - dcOffset;
        // Clamp to int16 range
        buffer[i] = constrain(corrected, -32768, 32767);
    }

    Serial.printf("Removed DC offset of %d\n", dcOffset);
}
```

This is a simple approach. More sophisticated methods use high-pass filters to remove low-frequency drift while preserving voice frequencies.

## Step 5: Validation Checklist

Before declaring victory, verify:

### ✓ Samples vary with sound

Record in silence, then record while talking. The RMS should be notably higher when you're speaking.

### ✓ DC offset is reasonable

Should be within ±2000 or so. If it's consistently at +32000 or -32000, something is wrong with the configuration.

### ✓ No constant patterns

If every sample is identical, or there's a repeating pattern that doesn't match sound, the data isn't audio.

### ✓ Timing is correct

Recording 5 seconds should take approximately 5 seconds. If it takes much longer, you're dropping samples. If it takes much less, something is misconfigured.

## Step 6: Quick Playback Test

If you have a way to play raw audio (Audacity, ffplay, etc.), you can export the buffer and listen:

```cpp
void dumpToSerial() {
    Serial.println("AUDIO_DATA_START");

    for (size_t i = 0; i < samplesRecorded; i++) {
        // Output as hex for easy parsing
        Serial.printf("%04X\n", (uint16_t)audioBuffer[i]);
    }

    Serial.println("AUDIO_DATA_END");
}
```

Then on your computer, capture the serial output and convert:

```fish
# Capture the hex dump to a file, then convert
cat audio_hex.txt | sed -n '/AUDIO_DATA_START/,/AUDIO_DATA_END/p' | \
  grep -v AUDIO | xxd -r -p > audio.raw

# Play with ffplay (install ffmpeg)
ffplay -f s16le -ar 16000 -ac 1 audio.raw
```

If you hear your voice, congratulations. You've captured audio.

---

## Verification

Before moving on, confirm:

- [ ] Recording completes in the expected time
- [ ] DC offset is within acceptable range
- [ ] RMS level changes when you speak vs silence
- [ ] No clipping during normal speech
- [ ] (Optional) Audio plays back correctly on your computer

---

## What's Next

You've proven that audio capture works. The samples are real audio, not noise or garbage. But storing 5 seconds in RAM and then looking at it isn't particularly useful for a wearable recorder.

In Phase 4, we'll combine everything: capturing audio continuously and writing it to the SD card in WAV format. [Step 4.1: WAV File Format](./10-wav-file-format.md) covers the format you'll use to store your audio — because raw PCM is fine for computers, but humans expect files they can double-click.

---

## Quick Reference

```cpp
// Check for PSRAM
if (psramFound()) {
    buffer = (int16_t*)ps_malloc(size);  // Use PSRAM
} else {
    buffer = (int16_t*)malloc(size);     // Use internal RAM
}

// Basic audio statistics
int64_t sum = 0;
for (int i = 0; i < n; i++) sum += samples[i];
double dcOffset = (double)sum / n;

int64_t sumSq = 0;
for (int i = 0; i < n; i++) sumSq += (int64_t)samples[i] * samples[i];
double rms = sqrt((double)sumSq / n);

// DC offset removal
int16_t dc = sum / n;
for (int i = 0; i < n; i++) samples[i] -= dc;
```
