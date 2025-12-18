# 1.3 Serial Debugging

## Overview

In the world of embedded development, you cannot set breakpoints. You cannot step through code. You cannot inspect variables with a fancy debugger UI. What you can do is print text to a serial connection and stare at it until enlightenment arrives.

This is Serial debugging. It is primitive. It is effective. It is the closest thing to telepathy you will achieve with a microcontroller.

## What You'll Learn

- Serial communication fundamentals
- Formatting output usefully
- Reading the ESP32-S3's crash dumps
- Using Serial as a debugging tool (not just a printf target)

## Prerequisites

- Completed [Step 1.2: Your First ESP32-S3 Project](./02-first-esp32-project.md)
- The M5 Capsule connected and uploading successfully
- Patience (more than you think you have)

---

## Step 1: Serial Fundamentals

Serial communication sends data one bit at a time over a wire. The key parameters are:

| Parameter     | What It Means           | Typical Value |
| ------------- | ----------------------- | ------------- |
| **Baud rate** | Bits per second         | 115200        |
| **Data bits** | Bits per character      | 8             |
| **Parity**    | Error checking bit      | None          |
| **Stop bits** | End of character marker | 1             |

You'll almost always use "115200 8N1" (115200 baud, 8 data bits, no parity, 1 stop bit). This is so universal it's not worth changing unless you have a specific reason.

### ESP32-S3 USB Serial

The ESP32-S3 has native USB support, which is different from older ESP32 models that needed a separate USB-to-serial chip. This means:

1. Serial output goes over the same USB cable you use for programming
2. The build flags we set earlier (`ARDUINO_USB_CDC_ON_BOOT=1`) enable this
3. The board appears as a USB CDC (Communications Device Class) device

This is why we added those flags to `platformio.ini`. Without them, serial output would go to pins you're not connected to, and you'd wonder why nothing is printing.

## Step 2: Basic Serial Usage

Let's write a sketch that demonstrates various Serial functions:

```cpp
#include <Arduino.h>
#include <M5Capsule.h>

unsigned long loopCount = 0;
unsigned long lastPrintTime = 0;

void setup() {
    M5.begin();

    // Initialise serial at 115200 baud
    Serial.begin(115200);

    // Wait for serial connection (important for USB CDC)
    while (!Serial) {
        delay(10);
    }

    // Now we can print
    Serial.println("=================================");
    Serial.println("  M5 Capsule Serial Debug Demo   ");
    Serial.println("=================================");
    Serial.println();

    // Print some system information
    Serial.print("CPU Frequency: ");
    Serial.print(getCpuFrequencyMhz());
    Serial.println(" MHz");

    Serial.print("Free Heap: ");
    Serial.print(ESP.getFreeHeap());
    Serial.println(" bytes");

    Serial.print("Flash Size: ");
    Serial.print(ESP.getFlashChipSize() / 1024 / 1024);
    Serial.println(" MB");

    Serial.println();
    Serial.println("Entering main loop...");
}

void loop() {
    M5.update();
    loopCount++;

    // Print status every second
    if (millis() - lastPrintTime >= 1000) {
        lastPrintTime = millis();

        Serial.printf("Loop count: %lu | Uptime: %lu seconds | Free heap: %lu bytes\n",
                      loopCount,
                      millis() / 1000,
                      ESP.getFreeHeap());

        loopCount = 0;  // Reset counter
    }

    // Check for serial input
    if (Serial.available()) {
        String input = Serial.readStringUntil('\n');
        input.trim();

        Serial.print("Received: ");
        Serial.println(input);

        if (input == "heap") {
            Serial.printf("Free heap: %lu bytes\n", ESP.getFreeHeap());
        } else if (input == "reset") {
            Serial.println("Resetting in 1 second...");
            delay(1000);
            ESP.restart();
        } else if (input == "help") {
            Serial.println("Commands: heap, reset, help");
        }
    }
}
```

### Key Functions

| Function                    | Purpose                              | Example                                   |
| --------------------------- | ------------------------------------ | ----------------------------------------- |
| `Serial.begin(baud)`        | Initialise serial at given baud rate | `Serial.begin(115200)`                    |
| `Serial.print(x)`           | Print without newline                | `Serial.print("Value: ")`                 |
| `Serial.println(x)`         | Print with newline                   | `Serial.println(42)`                      |
| `Serial.printf(fmt, ...)`   | Formatted print (like C printf)      | `Serial.printf("x=%d, y=%d\n", x, y)`     |
| `Serial.available()`        | Check if data waiting to be read     | `if (Serial.available())`                 |
| `Serial.read()`             | Read one byte                        | `char c = Serial.read()`                  |
| `Serial.readStringUntil(c)` | Read until delimiter                 | `String s = Serial.readStringUntil('\n')` |

### The `while (!Serial)` Pattern

```cpp
while (!Serial) {
    delay(10);
}
```

This waits until the serial connection is actually established. On USB CDC devices, there's a brief period after boot where the connection isn't ready. Without this, your first few prints might vanish into the void.

## Step 3: Effective Debug Output

Printing "it works" is not debugging. Effective debugging requires structure:

### Timestamped Output

```cpp
void debugPrint(const char* message) {
    Serial.printf("[%10lu] %s\n", millis(), message);
}

void debugPrintf(const char* format, ...) {
    char buffer[256];
    va_list args;
    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);

    Serial.printf("[%10lu] %s\n", millis(), buffer);
}

// Usage:
debugPrint("Starting audio capture");
debugPrintf("Buffer size: %d bytes", bufferSize);
```

Output:

```
[     12345] Starting audio capture
[     12346] Buffer size: 4096 bytes
```

The timestamp tells you when things happened and how long operations took.

### Log Levels

```cpp
enum LogLevel { LOG_DEBUG, LOG_INFO, LOG_WARN, LOG_ERROR };
LogLevel currentLogLevel = LOG_INFO;

void log(LogLevel level, const char* format, ...) {
    if (level < currentLogLevel) return;

    const char* levelStr[] = {"DEBUG", "INFO ", "WARN ", "ERROR"};

    char buffer[256];
    va_list args;
    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);

    Serial.printf("[%10lu] [%s] %s\n", millis(), levelStr[level], buffer);
}

// Usage:
log(LOG_INFO, "System initialised");
log(LOG_DEBUG, "Raw ADC value: %d", adcValue);  // Only prints if currentLogLevel <= LOG_DEBUG
log(LOG_ERROR, "SD card not found!");
```

### Module Tagging

When you have multiple subsystems, tag your output:

```cpp
#define LOG_SD(fmt, ...) Serial.printf("[%10lu] [SD   ] " fmt "\n", millis(), ##__VA_ARGS__)
#define LOG_I2S(fmt, ...) Serial.printf("[%10lu] [I2S  ] " fmt "\n", millis(), ##__VA_ARGS__)
#define LOG_BT(fmt, ...) Serial.printf("[%10lu] [BT   ] " fmt "\n", millis(), ##__VA_ARGS__)

// Usage:
LOG_SD("Mounted successfully");
LOG_I2S("Sample rate: %d Hz", sampleRate);
LOG_BT("Connected to %s", deviceName);
```

Output:

```
[      1234] [SD   ] Mounted successfully
[      1235] [I2S  ] Sample rate: 16000 Hz
[      5678] [BT   ] Connected to Dave's Phone
```

## Step 4: Reading Crash Dumps

When your ESP32 crashes (and it will), you'll see something like this:

```
Guru Meditation Error: Core  1 panic'ed (LoadProhibited). Exception was unhandled.
Core 1 register dump:
PC      : 0x400d1234  PS      : 0x00060030  A0      : 0x800d5678  A1      : 0x3ffb1234
...
Backtrace: 0x400d1234:0x3ffb1234 0x400d5678:0x3ffb5678 0x400d9abc:0x3ffb9abc
```

This is less helpful than it could be. The numbers are memory addresses, not function names.

### Using the Exception Decoder

Remember this line in `platformio.ini`?

```ini
monitor_filters = esp32_exception_decoder
```

This filter automatically decodes those addresses into function names. Instead of:

```
Backtrace: 0x400d1234:0x3ffb1234
```

You'll see:

```
Backtrace: readFromSD() at src/main.cpp:142
```

Much better.

### Common Crash Causes

| Error Type               | Likely Cause                       | Solution                                     |
| ------------------------ | ---------------------------------- | -------------------------------------------- |
| **LoadProhibited**       | Null pointer dereference           | Check your pointers before using them        |
| **StoreProhibited**      | Writing to null/invalid memory     | Check your pointers before writing           |
| **InstrFetchProhibited** | Jumping to invalid code address    | Corrupted function pointer or stack overflow |
| **Unhandled interrupt**  | ISR took too long or crashed       | Keep ISRs short, use flags for main loop     |
| **Stack overflow**       | Recursion or large local variables | Use heap allocation or increase stack size   |
| **Watchdog timeout**     | Loop blocked for too long          | Yield periodically, don't block in loop      |

### Defensive Programming

```cpp
void processBuffer(uint8_t* buffer, size_t size) {
    // Check for null pointer
    if (buffer == nullptr) {
        log(LOG_ERROR, "processBuffer: null buffer!");
        return;
    }

    // Check for reasonable size
    if (size == 0 || size > 1024 * 1024) {
        log(LOG_ERROR, "processBuffer: invalid size %u", size);
        return;
    }

    // Now safe to proceed
    // ...
}
```

## Step 5: Serial Monitor Tips

### PlatformIO Monitor Commands

While the monitor is running:

- **Ctrl+C** — Exit the monitor
- **Ctrl+T, Ctrl+H** — Show help
- **Ctrl+T, Ctrl+R** — Reset the device (RTS line)
- **Ctrl+T, Ctrl+D** — Toggle DTR line

### Filtering Output

If your output is too verbose, you can grep it:

```fish
pio device monitor | grep -E "\[ERROR\]|\[WARN\]"
```

### Logging to File

Capture a session for later analysis:

```fish
pio device monitor | tee session.log
```

### Multiple Terminals

You can have one terminal uploading while another monitors:

```fish
# Terminal 1: Watch output
pio device monitor

# Terminal 2: Upload (will temporarily disconnect monitor)
pio run -t upload
```

---

## Verification

Before moving on, confirm:

- [ ] You can see serial output in the monitor
- [ ] You understand the difference between `print`, `println`, and `printf`
- [ ] You can read input from the serial monitor
- [ ] You've seen a crash dump and know where to look for decoded output

Try intentionally causing a crash:

```cpp
void setup() {
    Serial.begin(115200);
    while (!Serial) delay(10);

    Serial.println("About to crash...");
    delay(1000);

    // Dereference a null pointer (DO NOT DO THIS IN REAL CODE)
    int* badPointer = nullptr;
    *badPointer = 42;  // BOOM
}
```

Watch the exception decoder turn that crash into useful information.

---

## What's Next

You now have a development environment, a working board, and the ability to see what's happening inside your code. These are the foundations.

In [Step 2.1: SPI Basics](./04-spi-basics.md), we'll start working toward actual functionality — understanding the SPI protocol that connects the ESP32 to the SD card. This is where the theory starts becoming practical.

---

## Quick Reference

```cpp
// Essential serial setup
Serial.begin(115200);
while (!Serial) delay(10);

// Basic output
Serial.print("no newline");
Serial.println("with newline");
Serial.printf("formatted: %d, %s, %.2f\n", intVal, strVal, floatVal);

// Reading input
if (Serial.available()) {
    String input = Serial.readStringUntil('\n');
    input.trim();
}

// Useful ESP32 functions
ESP.getFreeHeap()       // Free RAM
ESP.getFlashChipSize()  // Flash size in bytes
ESP.restart()           // Reboot
getCpuFrequencyMhz()    // CPU speed
millis()                // Milliseconds since boot
```
