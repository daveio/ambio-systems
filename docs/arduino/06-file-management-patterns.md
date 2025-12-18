# 2.3 File Management Patterns

## Overview

So you can write to an SD card. Wonderful. But a pendant recording audio all day will generate hundreds of files, potentially gigabytes of data, and will need to survive the battery dying mid-write. "It works on my desk" is not the same as "it works in production."

This guide covers the unglamorous but essential art of not losing data.

## What You'll Learn

- File naming strategies that won't drive you mad
- Handling FAT32's idiosyncrasies
- Safe file operations that survive power loss
- Rotation and cleanup strategies

## Prerequisites

- Completed [Step 2.2: SD Card Library](./05-sd-card-library.md)
- Successfully written and read files from your SD card
- A willingness to think about edge cases that "probably won't happen"

---

## Step 1: File Naming Strategies

When your device creates files automatically, naming them becomes surprisingly complicated.

### Option 1: Sequential Numbers

```cpp
// Find next available number
int findNextFileNumber(const char* prefix, const char* extension) {
    int num = 0;
    char filename[64];

    while (num < 100000) {
        snprintf(filename, sizeof(filename), "/%s%05d.%s", prefix, num, extension);
        if (!SD.exists(filename)) {
            return num;
        }
        num++;
    }
    return -1;  // All numbers exhausted (unlikely but possible)
}

// Usage
int n = findNextFileNumber("REC", "wav");
if (n >= 0) {
    char filename[64];
    snprintf(filename, sizeof(filename), "/REC%05d.wav", n);
    File f = SD.open(filename, FILE_WRITE);
}
```

**Pros:** Simple, predictable, no external dependencies
**Cons:** Gaps appear when files are deleted, scanning takes longer as numbers grow

### Option 2: Timestamps

If you have access to real time (via NTP, RTC, or phone sync):

```cpp
#include <time.h>

String generateTimestampFilename(const char* prefix, const char* extension) {
    time_t now;
    struct tm timeinfo;

    time(&now);
    localtime_r(&now, &timeinfo);

    char filename[64];
    snprintf(filename, sizeof(filename), "/%s_%04d%02d%02d_%02d%02d%02d.%s",
             prefix,
             timeinfo.tm_year + 1900,
             timeinfo.tm_mon + 1,
             timeinfo.tm_mday,
             timeinfo.tm_hour,
             timeinfo.tm_min,
             timeinfo.tm_sec,
             extension);

    return String(filename);
}

// Result: /REC_20251218_143052.wav
```

**Pros:** Human-readable, sortable, no scanning required
**Cons:** Requires RTC or time sync, potential for duplicates if clock not set

### Option 3: Hybrid Approach

Combine timestamps with a sequence counter for uniqueness:

```cpp
String generateFilename(const char* prefix, const char* extension) {
    static int sessionCounter = 0;
    unsigned long bootTime = millis();

    char filename[64];
    snprintf(filename, sizeof(filename), "/%s_%lu_%04d.%s",
             prefix, bootTime / 1000, sessionCounter++, extension);

    return String(filename);
}

// Result: /REC_1234_0001.wav (1234 seconds since boot, first file this session)
```

This survives reboots (different boot times) and multiple recordings in one session.

### FAT32 Filename Restrictions

FAT32 has opinions about filenames:

| Constraint         | Limit                | Consequence             |
| ------------------ | -------------------- | ----------------------- |
| Filename length    | 255 characters (LFN) | Keep paths short        |
| Path length        | ~260 characters      | Don't nest deeply       |
| Characters allowed | A-Z, 0-9, \_ - space | No : \* ? " < > \|      |
| Case sensitivity   | None                 | "FILE.txt" = "file.TXT" |

Stick to alphanumeric characters, underscores, and hyphens. Your future self will thank you.

## Step 2: Directory Organisation

Don't dump everything in root. Organise by date or session:

```cpp
bool ensureDirectory(const char* path) {
    if (SD.exists(path)) {
        return true;
    }
    return SD.mkdir(path);
}

String createDailyDirectory() {
    time_t now;
    struct tm timeinfo;
    time(&now);
    localtime_r(&now, &timeinfo);

    char dirname[32];
    snprintf(dirname, sizeof(dirname), "/%04d%02d%02d",
             timeinfo.tm_year + 1900,
             timeinfo.tm_mon + 1,
             timeinfo.tm_mday);

    ensureDirectory(dirname);
    return String(dirname);
}

// Then create files within:
// /20251218/REC_143052.wav
```

Benefits:

- Easier to find recordings from a specific day
- Easier to delete old data
- Avoids hitting directory entry limits

## Step 3: The FAT32 4GB File Size Limit

FAT32 files cannot exceed 4 GB (4,294,967,295 bytes, to be precise). For audio at 16kHz/16-bit mono:

- 32,000 bytes per second
- 1.92 MB per minute
- 115.2 MB per hour
- 2.77 GB per day of continuous recording

So you'll hit the 4 GB limit after about 34 hours of continuous recording into a single file. The solution: split files automatically.

```cpp
#define MAX_FILE_SIZE (3UL * 1024 * 1024 * 1024)  // 3 GB (leave some margin)

class SafeFileWriter {
private:
    File currentFile;
    String baseFilename;
    int partNumber;
    size_t bytesWritten;

public:
    bool begin(const char* filename) {
        baseFilename = filename;
        partNumber = 0;
        bytesWritten = 0;
        return openNextPart();
    }

    bool openNextPart() {
        if (currentFile) {
            currentFile.close();
        }

        String partFilename = baseFilename;
        if (partNumber > 0) {
            // Insert part number before extension
            int dotPos = partFilename.lastIndexOf('.');
            if (dotPos > 0) {
                partFilename = partFilename.substring(0, dotPos) +
                               "_part" + String(partNumber) +
                               partFilename.substring(dotPos);
            }
        }

        currentFile = SD.open(partFilename.c_str(), FILE_WRITE);
        partNumber++;
        bytesWritten = 0;

        return (bool)currentFile;
    }

    size_t write(const uint8_t* data, size_t length) {
        if (!currentFile) return 0;

        // Check if we need to split
        if (bytesWritten + length > MAX_FILE_SIZE) {
            if (!openNextPart()) {
                return 0;
            }
        }

        size_t written = currentFile.write(data, length);
        bytesWritten += written;
        return written;
    }

    void flush() {
        if (currentFile) currentFile.flush();
    }

    void close() {
        if (currentFile) currentFile.close();
    }
};
```

## Step 4: Surviving Power Loss

The nightmare scenario: battery dies mid-write. The file is corrupted, or worse, the entire filesystem is damaged.

### Strategy 1: Flush Frequently

```cpp
#define FLUSH_INTERVAL_MS 5000  // Flush every 5 seconds

unsigned long lastFlushTime = 0;

void loop() {
    // ... write data ...

    if (millis() - lastFlushTime > FLUSH_INTERVAL_MS) {
        audioFile.flush();
        lastFlushTime = millis();
    }
}
```

`flush()` forces buffered data to the SD card. Without it, data sits in RAM and dies with the power.

Trade-off: Frequent flushes increase SD card wear and momentarily block the CPU.

### Strategy 2: Write Complete Chunks

Instead of streaming continuously, write in self-contained chunks:

```cpp
#define CHUNK_DURATION_SEC 60  // One file per minute

void recordingLoop() {
    while (recording) {
        String filename = generateTimestampFilename("REC", "wav");
        File chunk = SD.open(filename.c_str(), FILE_WRITE);

        writeWavHeader(chunk);  // Write header with known duration

        unsigned long chunkStart = millis();
        while (recording && millis() - chunkStart < CHUNK_DURATION_SEC * 1000) {
            // Record audio...
        }

        updateWavHeader(chunk);  // Fix header with actual size
        chunk.close();
    }
}
```

If power dies, you lose at most one minute of audio, and all previous chunks are complete.

### Strategy 3: Journaling / Write-Ahead Log

For the truly paranoid:

```cpp
// Write a marker file before starting a recording
SD.remove("/recording.lock");
File lock = SD.open("/recording.lock", FILE_WRITE);
lock.println(currentFilename);
lock.close();

// ... do recording ...

// Remove marker when done
audioFile.close();
SD.remove("/recording.lock");

// On startup, check for orphaned recordings
if (SD.exists("/recording.lock")) {
    File lock = SD.open("/recording.lock", FILE_READ);
    String orphan = lock.readStringUntil('\n');
    lock.close();
    SD.remove("/recording.lock");

    // The file in 'orphan' was not properly closed
    // Could attempt recovery or just log the issue
    Serial.printf("WARNING: Previous recording may be corrupted: %s\n", orphan.c_str());
}
```

## Step 5: Space Management

SD cards fill up. Handle this gracefully.

### Check Space Before Recording

```cpp
bool hasSpaceForRecording(size_t estimatedSize) {
    uint64_t freeSpace = SD.totalBytes() - SD.usedBytes();
    return freeSpace > estimatedSize + (10 * 1024 * 1024);  // 10 MB buffer
}

// Before starting recording:
if (!hasSpaceForRecording(CHUNK_DURATION_SEC * 32000)) {
    Serial.println("WARNING: Low disk space");
    // Could delete old files, stop recording, or alert user
}
```

### Automatic Cleanup

Delete oldest files when space runs low:

```cpp
void cleanupOldFiles(size_t targetFreeSpace) {
    while (true) {
        uint64_t freeSpace = SD.totalBytes() - SD.usedBytes();
        if (freeSpace >= targetFreeSpace) {
            return;  // Enough space now
        }

        String oldest = findOldestRecording();
        if (oldest.length() == 0) {
            Serial.println("ERROR: No files to delete but still low on space");
            return;
        }

        Serial.printf("Deleting old recording: %s\n", oldest.c_str());
        SD.remove(oldest.c_str());
    }
}

String findOldestRecording() {
    // Implementation depends on your naming scheme
    // For timestamp names, find the lowest number
    // For sequential, find the lowest sequence number
    // etc.
}
```

---

## Putting It Together

Here's a complete file management class:

```cpp
class RecordingFileManager {
private:
    String currentFilename;
    File currentFile;
    size_t bytesWritten;
    unsigned long lastFlushTime;
    int sessionNumber;

    static const size_t MAX_FILE_SIZE = 3UL * 1024 * 1024 * 1024;
    static const unsigned long FLUSH_INTERVAL = 5000;
    static const size_t MIN_FREE_SPACE = 100 * 1024 * 1024;  // 100 MB

public:
    bool begin() {
        sessionNumber = 0;
        return startNewFile();
    }

    bool startNewFile() {
        if (currentFile) {
            currentFile.close();
        }

        // Check space
        if (SD.totalBytes() - SD.usedBytes() < MIN_FREE_SPACE) {
            Serial.println("ERROR: Insufficient space");
            return false;
        }

        // Generate filename
        currentFilename = String("/REC_") + String(millis() / 1000) +
                          "_" + String(sessionNumber++) + ".raw";

        currentFile = SD.open(currentFilename.c_str(), FILE_WRITE);
        if (!currentFile) {
            Serial.printf("ERROR: Failed to create %s\n", currentFilename.c_str());
            return false;
        }

        bytesWritten = 0;
        lastFlushTime = millis();

        Serial.printf("Started recording: %s\n", currentFilename.c_str());
        return true;
    }

    bool write(const uint8_t* data, size_t length) {
        if (!currentFile) return false;

        // Check if we need a new file
        if (bytesWritten + length > MAX_FILE_SIZE) {
            if (!startNewFile()) return false;
        }

        // Write data
        size_t written = currentFile.write(data, length);
        if (written != length) {
            Serial.printf("ERROR: Write incomplete (%d/%d)\n", written, length);
            return false;
        }

        bytesWritten += written;

        // Periodic flush
        if (millis() - lastFlushTime > FLUSH_INTERVAL) {
            currentFile.flush();
            lastFlushTime = millis();
        }

        return true;
    }

    void stop() {
        if (currentFile) {
            currentFile.flush();
            currentFile.close();
            Serial.printf("Stopped recording: %s (%d bytes)\n",
                          currentFilename.c_str(), bytesWritten);
        }
    }

    String getCurrentFilename() { return currentFilename; }
    size_t getBytesWritten() { return bytesWritten; }
};
```

---

## Verification

Before moving on, confirm:

- [ ] You can generate unique filenames that don't collide
- [ ] Files are flushed periodically (test by pulling power mid-recording)
- [ ] Your code handles "disk full" gracefully
- [ ] Files can be found and organised logically

---

## What's Next

You now have robust file management. But we've been writing text files. Audio is different — it comes as a continuous stream of samples at a fixed rate, and it won't wait politely for you to finish writing the previous buffer.

In [Step 3.1: I2S Protocol Understanding](./07-i2s-protocol-understanding.md), we enter the realm of digital audio. This is where the project gets interesting. Or frustrating. The line between those is thin.

---

## Quick Reference

```cpp
// Check if file/directory exists
bool exists = SD.exists("/path/file.txt");

// Create directory
SD.mkdir("/newdir");

// Delete file
SD.remove("/path/file.txt");

// Delete directory (must be empty)
SD.rmdir("/emptydir");

// Rename/move file
SD.rename("/old.txt", "/new.txt");

// Flush data to card
file.flush();

// Get free space
uint64_t freeBytes = SD.totalBytes() - SD.usedBytes();
```
