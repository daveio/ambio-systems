# Step 5.2: Error Handling & Robustness

## Overview

In the world of embedded systems, optimism is not a virtue — it's a bug waiting to happen. Your audio recorder will encounter SD card errors, memory corruption, power brownouts, and edge cases you haven't imagined. The difference between a prototype and a product is how gracefully it handles the inevitable failures.

This guide covers defensive programming for embedded systems: anticipating failures, detecting them early, recovering when possible, and failing safely when recovery isn't an option. Think of it as the firmware equivalent of a seatbelt — you hope you never need it, but you'd be foolish not to have it.

## What You'll Learn

- Detecting and handling SD card failures
- Managing memory corruption and leaks
- Implementing watchdog timers
- Safe file operations during power loss
- Error logging for post-mortem debugging
- Graceful degradation strategies

## Prerequisites

- Completed [Step 5.1: Power Management](13-power-management.md)
- Working duty-cycled recording system
- Understanding of FreeRTOS basics (from previous guides)

---

## Step 1: SD Card Failure Modes

SD cards fail in creative and surprising ways. Let's catalogue the disappointments:

### 1.1 Common SD Card Failures

| Failure Mode   | Symptom                    | Cause                                      |
| -------------- | -------------------------- | ------------------------------------------ |
| Mount failure  | `SD.begin()` returns false | Card removed, dirty contacts, dead card    |
| Write failure  | `file.write()` returns 0   | Card full, file system corrupt, card dying |
| Read failure   | Unexpected data            | Bit rot, card dying, electrical noise      |
| Slow writes    | Occasional long delays     | Card doing internal wear levelling         |
| Complete death | Nothing works              | Card has given up on life                  |

### 1.2 Robust SD Card Initialisation

Don't just check if `begin()` succeeds — verify the card is actually usable:

```cpp
enum SDStatus {
    SD_OK,
    SD_MOUNT_FAILED,
    SD_NO_CARD,
    SD_CARD_ERROR,
    SD_WRITE_TEST_FAILED,
    SD_LOW_SPACE
};

SDStatus initializeSDCard() {
    Serial.println("Initialising SD card...");

    // Attempt mount with retries
    int retries = 3;
    while (retries > 0) {
        if (SD.begin(SD_CS_PIN)) {
            break;
        }
        Serial.printf("SD mount failed, retrying (%d)...\n", retries);
        delay(100);
        retries--;
    }

    if (retries == 0) {
        Serial.println("SD mount failed after retries");
        return SD_MOUNT_FAILED;
    }

    // Check card type
    uint8_t cardType = SD.cardType();
    if (cardType == CARD_NONE) {
        Serial.println("No SD card inserted");
        return SD_NO_CARD;
    }

    Serial.printf("Card type: %s\n",
        cardType == CARD_MMC ? "MMC" :
        cardType == CARD_SD ? "SDSC" :
        cardType == CARD_SDHC ? "SDHC" : "Unknown");

    // Check card size and free space
    uint64_t cardSize = SD.cardSize() / (1024 * 1024);
    uint64_t usedSpace = SD.usedBytes() / (1024 * 1024);
    uint64_t freeSpace = cardSize - usedSpace;

    Serial.printf("Card size: %lluMB, Used: %lluMB, Free: %lluMB\n",
                  cardSize, usedSpace, freeSpace);

    // Warn if low space
    if (freeSpace < 100) {  // Less than 100MB free
        Serial.println("Warning: Low disk space");
        return SD_LOW_SPACE;
    }

    // Write test - verify we can actually write
    if (!performWriteTest()) {
        Serial.println("SD write test failed");
        return SD_WRITE_TEST_FAILED;
    }

    Serial.println("SD card initialised successfully");
    return SD_OK;
}

bool performWriteTest() {
    const char* testFile = "/.writetest";
    const char* testData = "ESP32 write test";

    // Write test file
    File f = SD.open(testFile, FILE_WRITE);
    if (!f) {
        Serial.println("Failed to create test file");
        return false;
    }

    size_t written = f.println(testData);
    f.close();

    if (written == 0) {
        Serial.println("Failed to write test data");
        return false;
    }

    // Read it back
    f = SD.open(testFile, FILE_READ);
    if (!f) {
        Serial.println("Failed to open test file for reading");
        return false;
    }

    String readBack = f.readStringUntil('\n');
    f.close();

    // Clean up
    SD.remove(testFile);

    // Verify
    if (readBack != testData) {
        Serial.println("Read-back verification failed");
        return false;
    }

    return true;
}
```

### 1.3 Handling SD Card Removal Mid-Recording

The card can be removed at any time. Detect and handle it:

```cpp
class RobustRecorder {
private:
    bool sdHealthy = true;
    uint32_t consecutiveWriteFailures = 0;
    const uint32_t MAX_WRITE_FAILURES = 3;

public:
    bool writeAudioChunk(const int16_t* buffer, size_t samples) {
        if (!sdHealthy) {
            // Already in failed state - attempt recovery periodically
            return attemptSDRecovery();
        }

        if (!audioFile) {
            Serial.println("No file open!");
            return false;
        }

        size_t bytesToWrite = samples * sizeof(int16_t);
        size_t bytesWritten = audioFile.write((uint8_t*)buffer, bytesToWrite);

        if (bytesWritten != bytesToWrite) {
            consecutiveWriteFailures++;
            Serial.printf("Write failed: wrote %u of %u bytes (failure #%u)\n",
                         bytesWritten, bytesToWrite, consecutiveWriteFailures);

            if (consecutiveWriteFailures >= MAX_WRITE_FAILURES) {
                Serial.println("Too many write failures - marking SD as unhealthy");
                sdHealthy = false;
                handleSDFailure();
            }
            return false;
        }

        // Reset failure counter on success
        consecutiveWriteFailures = 0;
        return true;
    }

private:
    void handleSDFailure() {
        // Close file if possible (may fail, that's okay)
        if (audioFile) {
            audioFile.close();
        }

        // Stop recording
        stopCapture();

        // Visual indication of failure
        indicateError(ERROR_SD_FAILED);

        // Log to RTC memory for post-mortem
        logErrorToRTC("SD card failure during recording");
    }

    bool attemptSDRecovery() {
        static uint32_t lastRecoveryAttempt = 0;
        const uint32_t RECOVERY_INTERVAL_MS = 5000;

        if (millis() - lastRecoveryAttempt < RECOVERY_INTERVAL_MS) {
            return false;
        }
        lastRecoveryAttempt = millis();

        Serial.println("Attempting SD card recovery...");

        // Unmount
        SD.end();
        delay(100);

        // Attempt remount
        if (initializeSDCard() == SD_OK) {
            Serial.println("SD card recovered!");
            sdHealthy = true;
            consecutiveWriteFailures = 0;
            return true;
        }

        Serial.println("SD recovery failed");
        return false;
    }
};
```

---

## Step 2: Watchdog Timers

When your code hangs — and it will — the watchdog timer ensures it doesn't stay hung forever.

### 2.1 Hardware Watchdog Overview

The ESP32-S3 has hardware watchdog timers that reset the system if not "fed" periodically:

```cpp
#include "esp_task_wdt.h"

const uint32_t WDT_TIMEOUT_S = 30;  // Reset if no feed for 30 seconds

void setupWatchdog() {
    // Initialise watchdog with timeout
    esp_task_wdt_init(WDT_TIMEOUT_S, true);  // true = trigger panic on timeout

    // Subscribe current task to watchdog
    esp_task_wdt_add(NULL);  // NULL = current task

    Serial.printf("Watchdog initialised with %d second timeout\n", WDT_TIMEOUT_S);
}

void feedWatchdog() {
    esp_task_wdt_reset();
}
```

### 2.2 Integrating Watchdog with Recording

Feed the watchdog during long operations:

```cpp
void recordingTask(void* parameter) {
    // Subscribe this task to watchdog
    esp_task_wdt_add(NULL);

    while (recording) {
        // Read audio buffer (this should be quick with DMA)
        size_t bytesRead;
        i2s_read(I2S_NUM, audioBuffer, BUFFER_SIZE, &bytesRead, pdMS_TO_TICKS(100));

        // Feed watchdog - we're still alive
        esp_task_wdt_reset();

        if (bytesRead > 0) {
            // Process audio...
            processBuffer(audioBuffer, bytesRead);
        }
    }

    // Unsubscribe before task exits
    esp_task_wdt_delete(NULL);
    vTaskDelete(NULL);
}
```

### 2.3 Handling Watchdog Resets

Detect when the system has been reset by watchdog:

```cpp
#include "esp_system.h"

void checkResetReason() {
    esp_reset_reason_t reason = esp_reset_reason();

    switch (reason) {
        case ESP_RST_POWERON:
            Serial.println("Reset: Power-on");
            break;
        case ESP_RST_SW:
            Serial.println("Reset: Software reset");
            break;
        case ESP_RST_PANIC:
            Serial.println("Reset: Exception/panic");
            logCrashDump();
            break;
        case ESP_RST_INT_WDT:
            Serial.println("Reset: Interrupt watchdog");
            logWatchdogEvent("interrupt");
            break;
        case ESP_RST_TASK_WDT:
            Serial.println("Reset: Task watchdog");
            logWatchdogEvent("task");
            break;
        case ESP_RST_WDT:
            Serial.println("Reset: Other watchdog");
            logWatchdogEvent("other");
            break;
        case ESP_RST_DEEPSLEEP:
            Serial.println("Reset: Deep sleep wake");
            break;
        case ESP_RST_BROWNOUT:
            Serial.println("Reset: Brownout");
            logBrownoutEvent();
            break;
        default:
            Serial.printf("Reset: Unknown reason (%d)\n", reason);
    }
}
```

---

## Step 3: Power Loss Protection

Power can disappear at any moment. Plan accordingly.

### 3.1 File System Corruption Prevention

The FAT32 file system is particularly vulnerable to corruption if power is lost during writes:

```cpp
class SafeFileWriter {
private:
    File currentFile;
    String tempFilename;
    String finalFilename;
    uint32_t lastFlushTime = 0;
    const uint32_t FLUSH_INTERVAL_MS = 5000;  // Flush every 5 seconds

public:
    bool beginRecording(const char* filename) {
        finalFilename = filename;
        tempFilename = String(filename) + ".tmp";

        // Write to temp file first
        currentFile = SD.open(tempFilename.c_str(), FILE_WRITE);
        if (!currentFile) {
            Serial.println("Failed to create temp file");
            return false;
        }

        // Write WAV header (will update later)
        writeInitialHeader();
        lastFlushTime = millis();

        return true;
    }

    bool writeChunk(const int16_t* buffer, size_t samples) {
        size_t bytesToWrite = samples * sizeof(int16_t);
        size_t written = currentFile.write((uint8_t*)buffer, bytesToWrite);

        if (written != bytesToWrite) {
            return false;
        }

        // Periodic flush to SD card
        if (millis() - lastFlushTime > FLUSH_INTERVAL_MS) {
            currentFile.flush();
            lastFlushTime = millis();
            Serial.println("Flushed to SD");
        }

        return true;
    }

    bool finaliseRecording() {
        if (!currentFile) {
            return false;
        }

        // Update WAV header with final size
        updateHeaderSize();

        // Final flush and close
        currentFile.flush();
        currentFile.close();

        // Remove old final file if exists
        if (SD.exists(finalFilename.c_str())) {
            SD.remove(finalFilename.c_str());
        }

        // Rename temp to final
        if (!SD.rename(tempFilename.c_str(), finalFilename.c_str())) {
            Serial.println("Failed to rename temp file");
            return false;
        }

        Serial.printf("Recording finalised: %s\n", finalFilename.c_str());
        return true;
    }

    // Call this on startup to clean up interrupted recordings
    static void cleanupInterruptedRecordings() {
        File root = SD.open("/");
        while (File entry = root.openNextFile()) {
            String name = entry.name();
            entry.close();

            if (name.endsWith(".tmp")) {
                Serial.printf("Removing interrupted recording: %s\n", name.c_str());
                SD.remove(("/" + name).c_str());
            }
        }
        root.close();
    }
};
```

### 3.2 Chunked Recording

Instead of one long file, write in chunks. If power fails, you only lose the current chunk:

```cpp
class ChunkedRecorder {
private:
    const uint32_t CHUNK_DURATION_S = 60;  // One minute chunks
    uint32_t chunkNumber = 0;
    String baseFilename;
    File currentChunk;
    uint32_t samplesInChunk = 0;

public:
    bool begin(const char* basename) {
        baseFilename = basename;
        chunkNumber = findNextChunkNumber();
        return startNewChunk();
    }

    bool writeAudio(const int16_t* buffer, size_t samples) {
        size_t written = currentChunk.write((uint8_t*)buffer, samples * sizeof(int16_t));
        if (written != samples * sizeof(int16_t)) {
            return false;
        }

        samplesInChunk += samples;

        // Check if chunk is full
        uint32_t samplesPerChunk = SAMPLE_RATE * CHUNK_DURATION_S;
        if (samplesInChunk >= samplesPerChunk) {
            finaliseCurrentChunk();
            startNewChunk();
        }

        return true;
    }

    void stop() {
        finaliseCurrentChunk();
    }

private:
    uint32_t findNextChunkNumber() {
        // Find highest existing chunk number
        uint32_t highest = 0;
        File root = SD.open("/");
        while (File entry = root.openNextFile()) {
            String name = entry.name();
            entry.close();

            if (name.startsWith(baseFilename)) {
                // Extract chunk number from filename
                int num = extractChunkNumber(name);
                if (num > highest) highest = num;
            }
        }
        root.close();
        return highest + 1;
    }

    bool startNewChunk() {
        String filename = String("/") + baseFilename + "_" +
                         String(chunkNumber, DEC) + ".wav";

        currentChunk = SD.open(filename.c_str(), FILE_WRITE);
        if (!currentChunk) {
            Serial.printf("Failed to create chunk: %s\n", filename.c_str());
            return false;
        }

        writeWavHeader();
        samplesInChunk = 0;

        Serial.printf("Started chunk: %s\n", filename.c_str());
        return true;
    }

    void finaliseCurrentChunk() {
        if (!currentChunk) return;

        updateWavHeader();
        currentChunk.close();

        Serial.printf("Finalised chunk %d (%d samples)\n",
                     chunkNumber, samplesInChunk);
        chunkNumber++;
    }
};
```

---

## Step 4: Error Logging

When things go wrong in the field, you need to know what happened.

### 4.1 RTC Memory Error Log

Persist errors across resets using RTC memory:

```cpp
// Error log entry structure
struct ErrorEntry {
    uint32_t timestamp;      // millis() at error time
    uint32_t resetCount;     // Which boot this error occurred on
    uint8_t errorCode;       // Error type
    char message[32];        // Brief description
};

// Circular buffer of errors in RTC memory
const size_t MAX_ERRORS = 10;
RTC_DATA_ATTR ErrorEntry errorLog[MAX_ERRORS];
RTC_DATA_ATTR size_t errorLogIndex = 0;
RTC_DATA_ATTR uint32_t totalResets = 0;

enum ErrorCode {
    ERR_NONE = 0,
    ERR_SD_MOUNT_FAILED,
    ERR_SD_WRITE_FAILED,
    ERR_SD_FULL,
    ERR_I2S_INIT_FAILED,
    ERR_I2S_READ_FAILED,
    ERR_MEMORY_ALLOC_FAILED,
    ERR_WATCHDOG_RESET,
    ERR_BROWNOUT,
    ERR_UNKNOWN
};

void logError(ErrorCode code, const char* message) {
    ErrorEntry& entry = errorLog[errorLogIndex];
    entry.timestamp = millis();
    entry.resetCount = totalResets;
    entry.errorCode = code;
    strncpy(entry.message, message, sizeof(entry.message) - 1);
    entry.message[sizeof(entry.message) - 1] = '\0';

    errorLogIndex = (errorLogIndex + 1) % MAX_ERRORS;

    Serial.printf("ERROR [%d]: %s\n", code, message);
}

void dumpErrorLog() {
    Serial.println("=== Error Log ===");
    Serial.printf("Total resets: %d\n", totalResets);

    for (size_t i = 0; i < MAX_ERRORS; i++) {
        ErrorEntry& entry = errorLog[(errorLogIndex + i) % MAX_ERRORS];
        if (entry.errorCode != ERR_NONE) {
            Serial.printf("[Reset %d, t=%dms] Error %d: %s\n",
                         entry.resetCount, entry.timestamp,
                         entry.errorCode, entry.message);
        }
    }
    Serial.println("=================");
}

void setup() {
    totalResets++;

    // Dump any errors from previous run
    if (totalResets > 1) {
        dumpErrorLog();
    }

    // ... rest of setup
}
```

### 4.2 SD Card Error Log

For more detailed logging, write to the SD card:

```cpp
class SDLogger {
private:
    const char* logFilename = "/error_log.txt";
    bool sdAvailable = false;

public:
    void begin(bool sdReady) {
        sdAvailable = sdReady;
    }

    void log(const char* level, const char* format, ...) {
        char buffer[256];

        // Format timestamp
        int offset = snprintf(buffer, sizeof(buffer),
                             "[%lu][%s] ", millis(), level);

        // Format message
        va_list args;
        va_start(args, format);
        vsnprintf(buffer + offset, sizeof(buffer) - offset, format, args);
        va_end(args);

        // Always print to serial
        Serial.println(buffer);

        // Write to SD if available
        if (sdAvailable) {
            writeToSD(buffer);
        }
    }

    void error(const char* format, ...) {
        va_list args;
        va_start(args, format);
        logv("ERROR", format, args);
        va_end(args);
    }

    void warn(const char* format, ...) {
        va_list args;
        va_start(args, format);
        logv("WARN", format, args);
        va_end(args);
    }

    void info(const char* format, ...) {
        va_list args;
        va_start(args, format);
        logv("INFO", format, args);
        va_end(args);
    }

private:
    void writeToSD(const char* message) {
        File logFile = SD.open(logFilename, FILE_APPEND);
        if (logFile) {
            logFile.println(message);
            logFile.close();
        }
    }

    void logv(const char* level, const char* format, va_list args) {
        char buffer[256];
        int offset = snprintf(buffer, sizeof(buffer),
                             "[%lu][%s] ", millis(), level);
        vsnprintf(buffer + offset, sizeof(buffer) - offset, format, args);
        Serial.println(buffer);
        if (sdAvailable) writeToSD(buffer);
    }
};

SDLogger logger;
```

---

## Step 5: Memory Management

Memory leaks in embedded systems are fatal — there's no swap file to save you.

### 5.1 Monitoring Heap Usage

```cpp
void printMemoryStats() {
    Serial.println("=== Memory Stats ===");
    Serial.printf("Total heap: %d bytes\n", ESP.getHeapSize());
    Serial.printf("Free heap: %d bytes\n", ESP.getFreeHeap());
    Serial.printf("Min free heap ever: %d bytes\n", ESP.getMinFreeHeap());
    Serial.printf("Max alloc heap: %d bytes\n", ESP.getMaxAllocHeap());

    // PSRAM stats (if available)
    if (psramFound()) {
        Serial.printf("PSRAM size: %d bytes\n", ESP.getPsramSize());
        Serial.printf("Free PSRAM: %d bytes\n", ESP.getFreePsram());
    }
    Serial.println("====================");
}

// Call periodically to detect leaks
void checkForMemoryLeaks() {
    static uint32_t lastFreeHeap = 0;
    uint32_t currentFreeHeap = ESP.getFreeHeap();

    if (lastFreeHeap > 0) {
        int32_t delta = (int32_t)currentFreeHeap - (int32_t)lastFreeHeap;
        if (delta < -1000) {  // Lost more than 1KB
            logger.warn("Memory decreasing: %d bytes lost", -delta);
        }
    }

    lastFreeHeap = currentFreeHeap;

    // Critical low memory warning
    if (currentFreeHeap < 10000) {  // Less than 10KB free
        logger.error("Critical: Low memory (%d bytes free)", currentFreeHeap);
    }
}
```

### 5.2 Safe Dynamic Allocation

Always check allocation results:

```cpp
int16_t* allocateAudioBuffer(size_t samples) {
    size_t size = samples * sizeof(int16_t);

    // Try PSRAM first (if available and large allocation)
    int16_t* buffer = nullptr;
    if (psramFound() && size > 4096) {
        buffer = (int16_t*)ps_malloc(size);
        if (buffer) {
            Serial.printf("Allocated %d bytes from PSRAM\n", size);
            return buffer;
        }
    }

    // Fall back to internal RAM
    buffer = (int16_t*)malloc(size);
    if (buffer) {
        Serial.printf("Allocated %d bytes from heap\n", size);
        return buffer;
    }

    // Allocation failed
    logger.error("Failed to allocate %d bytes for audio buffer", size);
    return nullptr;
}

// Usage with error handling
void startRecording() {
    audioBuffer = allocateAudioBuffer(BUFFER_SAMPLES);
    if (!audioBuffer) {
        indicateError(ERROR_MEMORY);
        return;  // Don't continue without buffer
    }

    // Proceed with recording...
}
```

---

## Step 6: Graceful Degradation

When things go wrong, degrade gracefully rather than crashing.

### 6.1 Error Response Strategy

```cpp
enum SystemState {
    STATE_NORMAL,
    STATE_DEGRADED,
    STATE_ERROR,
    STATE_FATAL
};

class SystemHealth {
private:
    SystemState state = STATE_NORMAL;
    uint32_t errorCount = 0;

public:
    void reportError(ErrorCode code) {
        errorCount++;

        switch (code) {
            case ERR_SD_WRITE_FAILED:
                // Single write failure - recoverable
                if (errorCount < 5) {
                    state = STATE_DEGRADED;
                    // Maybe reduce recording quality or buffer more aggressively
                } else {
                    state = STATE_ERROR;
                    // Stop recording, await recovery
                }
                break;

            case ERR_SD_MOUNT_FAILED:
                state = STATE_ERROR;
                // Can't record without SD card
                break;

            case ERR_I2S_INIT_FAILED:
                state = STATE_FATAL;
                // Can't record without I2S
                break;

            case ERR_MEMORY_ALLOC_FAILED:
                state = STATE_FATAL;
                // Need to restart
                break;
        }

        handleStateChange();
    }

    void reportSuccess() {
        if (errorCount > 0) {
            errorCount--;
        }
        if (errorCount == 0 && state == STATE_DEGRADED) {
            state = STATE_NORMAL;
            handleStateChange();
        }
    }

private:
    void handleStateChange() {
        switch (state) {
            case STATE_NORMAL:
                indicateLED(LED_GREEN);
                break;

            case STATE_DEGRADED:
                indicateLED(LED_YELLOW);
                logger.warn("System degraded - %d errors", errorCount);
                break;

            case STATE_ERROR:
                indicateLED(LED_RED);
                logger.error("System error - stopping recording");
                stopRecording();
                scheduleRecoveryAttempt();
                break;

            case STATE_FATAL:
                indicateLED(LED_RED_BLINK);
                logger.error("Fatal error - system restart required");
                saveStateToRTC();
                delay(1000);
                ESP.restart();
                break;
        }
    }
};

SystemHealth health;
```

---

## Verification Checklist

Before moving on, verify:

- [ ] SD card initialisation includes write test
- [ ] Write failures are detected and counted
- [ ] Watchdog timer prevents infinite hangs
- [ ] Reset reason is logged on startup
- [ ] Temporary files are cleaned up after interrupted recordings
- [ ] Error log persists across resets (RTC memory)
- [ ] Memory usage is monitored periodically
- [ ] System degrades gracefully rather than crashing

---

## Common Issues

### Watchdog Keeps Resetting

**Symptom:** System resets every 30 seconds

**Causes:**

- Long-running operation without feeding watchdog
- Task is blocked waiting for something that never happens

**Solutions:**

```cpp
// Feed watchdog during long operations
for (int i = 0; i < 1000000; i++) {
    // Long operation...
    if (i % 10000 == 0) {
        esp_task_wdt_reset();  // Feed the dog
    }
}
```

### Lost Recordings After Power Loss

**Symptom:** Files exist but are corrupt or truncated

**Causes:**

- WAV header not updated before power loss
- File system not flushed

**Solutions:**

- Use chunked recording (lose at most one chunk)
- Flush frequently (every 5 seconds)
- Update WAV header periodically, not just at end

### Memory Keeps Decreasing

**Symptom:** Free heap shrinks over time

**Causes:**

- String concatenation creating temporaries
- Forgotten `free()` calls
- Libraries with internal leaks

**Solutions:**

```cpp
// Avoid String concatenation in loops
// BAD:
String result = "";
for (int i = 0; i < 100; i++) {
    result += String(i);  // Creates temporary each iteration
}

// GOOD:
char result[1024];
int offset = 0;
for (int i = 0; i < 100; i++) {
    offset += snprintf(result + offset, sizeof(result) - offset, "%d", i);
}
```

---

## What's Next

You now have a robust system that handles failures gracefully. But your recording logic is probably a tangled mess of conditionals. In [Step 5.3: State Machine Implementation](15-state-machine-implementation.md), we'll refactor everything into a clean, maintainable state machine that's easier to understand, debug, and extend.

---

## Quick Reference

### Watchdog Setup

```cpp
esp_task_wdt_init(TIMEOUT_S, true);  // Init with timeout
esp_task_wdt_add(NULL);               // Add current task
esp_task_wdt_reset();                 // Feed the watchdog
esp_task_wdt_delete(NULL);            // Remove before task exits
```

### Reset Reason

```cpp
esp_reset_reason_t reason = esp_reset_reason();
// ESP_RST_POWERON, ESP_RST_PANIC, ESP_RST_TASK_WDT, ESP_RST_BROWNOUT, etc.
```

### Memory Monitoring

```cpp
ESP.getFreeHeap();       // Current free
ESP.getMinFreeHeap();    // Lowest ever
ESP.getMaxAllocHeap();   // Largest allocatable block
```

### RTC Memory

```cpp
RTC_DATA_ATTR int persistentVar = 0;  // Survives deep sleep
```
