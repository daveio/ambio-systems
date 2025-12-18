# 2.2 SD Card Library

## Overview

Now that you understand SPI at a conceptual level, we can use someone else's code to hide all that complexity. This is the beauty of libraries — decades of low-level protocol debugging, distilled into a few function calls that mostly work most of the time.

The Arduino `SD` library will handle the SPI communication, the FAT32 filesystem, and all the fiddly timing requirements. Your job is to call `open()`, `write()`, and `close()` without messing up.

You'll mess up. We all mess up. That's what this guide is for.

## What You'll Learn

- Initialising the SD card on the M5 Capsule
- Creating, writing, and reading files
- Handling SD card errors gracefully
- Understanding the difference between `SD` and `SD_MMC` libraries

## Prerequisites

- Completed [Step 2.1: SPI Basics](./04-spi-basics.md)
- An SD card inserted into your M5 Capsule (formatted FAT32)
- An SD card reader for your computer (to verify files actually exist)

---

## Step 1: Choose Your Library

The ESP32 Arduino core provides two SD card libraries:

| Library  | Interface | Speed  | Pins     | When to Use                             |
| -------- | --------- | ------ | -------- | --------------------------------------- |
| `SD`     | SPI       | Slower | Flexible | Generic microcontrollers, custom wiring |
| `SD_MMC` | SDMMC     | Faster | Fixed    | Native SD slot, 1-bit or 4-bit mode     |

The M5 Capsule's SD slot may support either mode. We'll start with `SD` (SPI) because it's more universal and easier to debug. If you need more speed later, you can migrate to `SD_MMC`.

### Adding the Library

The SD library comes built into the Arduino-ESP32 framework, so you don't need to add it to `lib_deps`. Just include it:

```cpp
#include <SD.h>
#include <SPI.h>
```

## Step 2: Find Your Pin Numbers

Before initialising the SD card, you need to know which GPIO pins connect to the SD card slot. This information is in the M5 Capsule documentation.

Based on typical M5 hardware, the pins are often:

| Function | Likely GPIO   | M5 Capsule (verify!) |
| -------- | ------------- | -------------------- |
| SD_CS    | GPIO 4 or 13  | Check docs           |
| SD_MOSI  | GPIO 15 or 23 | Check docs           |
| SD_MISO  | GPIO 2 or 19  | Check docs           |
| SD_SCK   | GPIO 14 or 18 | Check docs           |

**Important:** These pins vary between M5 products. Check the [M5 Capsule schematic](https://docs.m5stack.com/en/core/M5Capsule) before proceeding. Using the wrong pins is the #1 cause of "SD card not found" errors.

For this guide, I'll use placeholder definitions. You'll need to update them:

```cpp
#define SD_CS   4    // UPDATE THIS
#define SD_MOSI 15   // UPDATE THIS
#define SD_MISO 2    // UPDATE THIS
#define SD_SCK  14   // UPDATE THIS
```

## Step 3: Basic SD Card Initialisation

Let's write a sketch that mounts the SD card and prints information about it:

```cpp
#include <Arduino.h>
#include <M5Capsule.h>
#include <SD.h>
#include <SPI.h>

// SD card pins - UPDATE THESE FOR YOUR HARDWARE
#define SD_CS   4
#define SD_MOSI 15
#define SD_MISO 2
#define SD_SCK  14

SPIClass spi = SPIClass(HSPI);

void setup() {
    M5.begin();
    Serial.begin(115200);
    while (!Serial) delay(10);

    Serial.println("SD Card Initialisation Test");
    Serial.println("============================");

    // Initialise SPI with our custom pins
    spi.begin(SD_SCK, SD_MISO, SD_MOSI, SD_CS);

    // Attempt to mount the SD card
    if (!SD.begin(SD_CS, spi, 4000000)) {  // 4 MHz to start (conservative)
        Serial.println("SD card mount FAILED!");
        Serial.println("Check:");
        Serial.println("  - Is a card inserted?");
        Serial.println("  - Is it formatted as FAT32?");
        Serial.println("  - Are the pin definitions correct?");
        while (true) delay(1000);  // Halt
    }

    Serial.println("SD card mounted successfully!");

    // Print card information
    uint8_t cardType = SD.cardType();

    Serial.print("Card type: ");
    switch (cardType) {
        case CARD_NONE: Serial.println("No card"); break;
        case CARD_MMC:  Serial.println("MMC"); break;
        case CARD_SD:   Serial.println("SDSC"); break;
        case CARD_SDHC: Serial.println("SDHC"); break;
        default:        Serial.println("Unknown"); break;
    }

    uint64_t cardSize = SD.cardSize() / (1024 * 1024);
    Serial.printf("Card size: %llu MB\n", cardSize);

    uint64_t totalBytes = SD.totalBytes() / (1024 * 1024);
    Serial.printf("Total space: %llu MB\n", totalBytes);

    uint64_t usedBytes = SD.usedBytes() / (1024 * 1024);
    Serial.printf("Used space: %llu MB\n", usedBytes);
}

void loop() {
    // Nothing to do here
    delay(1000);
}
```

### What's Happening

1. **`SPIClass spi = SPIClass(HSPI)`** — Creates an SPI instance using the HSPI peripheral
2. **`spi.begin(SCK, MISO, MOSI, CS)`** — Initialises SPI with our pin assignments
3. **`SD.begin(CS, spi, 4000000)`** — Mounts the SD card using our SPI instance at 4 MHz
4. **`SD.cardType()`** — Identifies the card type
5. **`SD.cardSize()`** — Total card capacity
6. **`SD.totalBytes()`** / **`SD.usedBytes()`** — Filesystem space

Upload this and check the serial monitor. If it works, you'll see card information. If it doesn't, re-check your pin definitions.

## Step 4: Writing Files

Now let's create a file:

```cpp
#include <Arduino.h>
#include <M5Capsule.h>
#include <SD.h>
#include <SPI.h>

#define SD_CS   4
#define SD_MOSI 15
#define SD_MISO 2
#define SD_SCK  14

SPIClass spi = SPIClass(HSPI);

void setup() {
    M5.begin();
    Serial.begin(115200);
    while (!Serial) delay(10);

    Serial.println("SD Card Write Test");
    Serial.println("==================");

    spi.begin(SD_SCK, SD_MISO, SD_MOSI, SD_CS);

    if (!SD.begin(SD_CS, spi, 4000000)) {
        Serial.println("SD card mount failed!");
        while (true) delay(1000);
    }

    Serial.println("SD card mounted.");

    // Create/overwrite a file
    Serial.println("Creating test file...");

    File file = SD.open("/test.txt", FILE_WRITE);

    if (!file) {
        Serial.println("Failed to open file for writing!");
        while (true) delay(1000);
    }

    // Write some text
    file.println("Hello from M5 Capsule!");
    file.println("This is line 2.");
    file.printf("Timestamp: %lu ms since boot\n", millis());

    // Always close the file!
    file.close();

    Serial.println("File written successfully.");
    Serial.println("Remove the SD card and check it on your computer.");
}

void loop() {
    delay(1000);
}
```

### File Modes

| Mode          | Behaviour                                                |
| ------------- | -------------------------------------------------------- |
| `FILE_READ`   | Open for reading (fails if file doesn't exist)           |
| `FILE_WRITE`  | Open for writing (creates file, overwrites if exists)    |
| `FILE_APPEND` | Open for appending (creates file, adds to end if exists) |

### Critical: Always Close Your Files

```cpp
file.close();
```

This isn't optional. The SD library buffers writes for performance. If you don't close the file:

- Data may not be flushed to the card
- The file allocation table may be corrupted
- Your file will appear empty or damaged

If your device crashes or loses power before `close()` is called, data loss is likely. We'll address this in Phase 4 with more robust buffering strategies.

## Step 5: Reading Files

Let's read back what we wrote:

```cpp
void setup() {
    // ... (same SD initialisation as before)

    // Open file for reading
    File file = SD.open("/test.txt", FILE_READ);

    if (!file) {
        Serial.println("Failed to open file for reading!");
        return;
    }

    Serial.println("File contents:");
    Serial.println("---------------");

    // Read line by line
    while (file.available()) {
        String line = file.readStringUntil('\n');
        Serial.println(line);
    }

    Serial.println("---------------");
    Serial.printf("File size: %d bytes\n", file.size());

    file.close();
}
```

### Reading Methods

| Method                    | Returns           | Use Case                  |
| ------------------------- | ----------------- | ------------------------- |
| `file.read()`             | Single byte (int) | Binary data, byte-by-byte |
| `file.read(buf, len)`     | Bytes read        | Binary data, buffered     |
| `file.readStringUntil(c)` | String            | Text files, line-by-line  |
| `file.available()`        | Bytes remaining   | Loop condition            |
| `file.size()`             | File size         | Pre-allocation            |

## Step 6: Listing Directory Contents

```cpp
void listDirectory(fs::FS &fs, const char *dirname, uint8_t levels) {
    Serial.printf("Listing directory: %s\n", dirname);

    File root = fs.open(dirname);
    if (!root) {
        Serial.println("Failed to open directory");
        return;
    }
    if (!root.isDirectory()) {
        Serial.println("Not a directory");
        return;
    }

    File file = root.openNextFile();
    while (file) {
        if (file.isDirectory()) {
            Serial.printf("  DIR:  %s\n", file.name());
            if (levels > 0) {
                listDirectory(fs, file.path(), levels - 1);
            }
        } else {
            Serial.printf("  FILE: %s  (%d bytes)\n", file.name(), file.size());
        }
        file = root.openNextFile();
    }
}

void setup() {
    // ... (SD initialisation)

    listDirectory(SD, "/", 2);  // List root, 2 levels deep
}
```

## Step 7: Error Handling

SD cards are unreliable. They can be:

- Removed mid-operation
- Full
- Corrupted
- Formatted with an incompatible filesystem
- Just... not working, for reasons known only to them

Robust code anticipates this:

```cpp
bool writeToFile(const char* filename, const uint8_t* data, size_t length) {
    // Check if SD card is still mounted
    if (SD.cardType() == CARD_NONE) {
        Serial.println("ERROR: No SD card detected");
        return false;
    }

    // Check available space
    uint64_t freeSpace = SD.totalBytes() - SD.usedBytes();
    if (freeSpace < length + 1024) {  // Leave 1KB buffer
        Serial.println("ERROR: Insufficient space on SD card");
        return false;
    }

    // Attempt to open file
    File file = SD.open(filename, FILE_APPEND);
    if (!file) {
        Serial.printf("ERROR: Failed to open %s\n", filename);
        return false;
    }

    // Write data
    size_t written = file.write(data, length);

    // Verify write
    if (written != length) {
        Serial.printf("ERROR: Write incomplete (%d of %d bytes)\n", written, length);
        file.close();
        return false;
    }

    // Flush and close
    file.flush();  // Force data to card
    file.close();

    return true;
}
```

---

## Verification

Create a complete test sketch:

```cpp
#include <Arduino.h>
#include <M5Capsule.h>
#include <SD.h>
#include <SPI.h>

#define SD_CS   4
#define SD_MOSI 15
#define SD_MISO 2
#define SD_SCK  14

SPIClass spi = SPIClass(HSPI);

void listDir(File dir, int indent = 0) {
    while (true) {
        File entry = dir.openNextFile();
        if (!entry) break;

        for (int i = 0; i < indent; i++) Serial.print("  ");
        Serial.print(entry.name());

        if (entry.isDirectory()) {
            Serial.println("/");
            listDir(entry, indent + 1);
        } else {
            Serial.printf(" (%d bytes)\n", entry.size());
        }
        entry.close();
    }
}

void setup() {
    M5.begin();
    Serial.begin(115200);
    while (!Serial) delay(10);

    Serial.println("\n=== SD Card Full Test ===\n");

    // Initialise
    spi.begin(SD_SCK, SD_MISO, SD_MOSI, SD_CS);
    if (!SD.begin(SD_CS, spi, 4000000)) {
        Serial.println("FAIL: SD mount");
        while (true) delay(1000);
    }
    Serial.println("PASS: SD mounted");

    // Write test
    File f = SD.open("/capsule_test.txt", FILE_WRITE);
    if (!f) {
        Serial.println("FAIL: Open for write");
        while (true) delay(1000);
    }
    f.println("Test file from M5 Capsule");
    f.printf("Created at %lu ms\n", millis());
    f.close();
    Serial.println("PASS: File written");

    // Read test
    f = SD.open("/capsule_test.txt", FILE_READ);
    if (!f) {
        Serial.println("FAIL: Open for read");
        while (true) delay(1000);
    }
    Serial.println("PASS: File read:");
    while (f.available()) {
        Serial.write(f.read());
    }
    f.close();

    // List directory
    Serial.println("\nDirectory listing:");
    File root = SD.open("/");
    listDir(root);
    root.close();

    Serial.println("\n=== All tests passed ===");
    Serial.println("Check the SD card on your computer for capsule_test.txt");
}

void loop() {
    delay(1000);
}
```

Before moving on, confirm:

- [ ] SD card mounts successfully
- [ ] You can write a file
- [ ] You can read the file back via serial
- [ ] The file appears on your computer when you remove the card
- [ ] The file contains the expected content

---

## What's Next

You can now write to an SD card. But writing proper audio files requires understanding file formats and managing files that grow over time. In [Step 2.3: File Management Patterns](./06-file-management-patterns.md), we'll explore naming schemes, size limits, and patterns for robust file handling.

---

## Quick Reference

```cpp
// Include libraries
#include <SD.h>
#include <SPI.h>

// Initialisation
SPIClass spi = SPIClass(HSPI);
spi.begin(SCK, MISO, MOSI, CS);
SD.begin(CS, spi, frequency);

// File operations
File f = SD.open("/path/file.txt", FILE_WRITE);  // or FILE_READ, FILE_APPEND
f.print("text");
f.println("text with newline");
f.printf("formatted %d", value);
f.write(buffer, length);
size_t bytesRead = f.read(buffer, length);
f.flush();
f.close();

// Card info
SD.cardType();    // CARD_NONE, CARD_MMC, CARD_SD, CARD_SDHC
SD.cardSize();    // Total bytes
SD.totalBytes();  // Filesystem bytes
SD.usedBytes();   // Used bytes
```
