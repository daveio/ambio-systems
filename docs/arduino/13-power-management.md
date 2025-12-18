# Step 5.1: Power Management

## Overview

Your M5 Capsule pendant has a battery. Your audio recording firmware has an appetite for electrons. These two facts are on a collision course, and physics — as always — will win. This guide covers how to lose gracefully, stretching your battery life from "embarrassingly brief" to "actually useful."

The ESP32-S3 is not, by nature, a low-power device. It's a dual-core 240MHz processor with WiFi and Bluetooth radios, designed by people who apparently assumed wall sockets would always be nearby. However, it _does_ have various sleep modes and power-saving features that, when properly configured, can dramatically reduce consumption during idle periods.

## What You'll Learn

- ESP32-S3 power consumption characteristics
- Sleep modes and when to use each
- Peripheral power management
- Efficient recording duty cycles
- Wake-up sources and triggers
- Practical battery life calculations

## Prerequisites

- Completed [Step 4.3: Putting It All Together](12-putting-it-all-together.md)
- Working continuous recording system
- Basic understanding of your use case's recording requirements

---

## Step 1: Understanding ESP32-S3 Power States

The ESP32-S3 has several power states, each trading capability for consumption:

### Power Consumption Overview

| State             | Typical Current | What Works             |
| ----------------- | --------------- | ---------------------- |
| Active (WiFi TX)  | 310-380mA       | Everything             |
| Active (CPU only) | 50-80mA         | CPU, peripherals       |
| Modem Sleep       | 20-30mA         | CPU, no radio          |
| Light Sleep       | 0.8-2mA         | RTC, GPIO wake         |
| Deep Sleep        | 8-25µA          | RTC, ULP, limited GPIO |
| Hibernation       | 2.5µA           | RTC timer only         |

For audio recording, you're typically in "Active (CPU only)" territory — roughly 50-80mA depending on what peripherals are running.

### Where the Power Goes

During active recording:

```
CPU cores:           ~30mA (varies with clock speed)
I2S peripheral:      ~5mA
SD card active:      ~30-100mA (varies wildly by card)
SD card idle:        ~0.1-1mA
Microphone:          ~1mA
LED (if on):         ~20mA per colour
USB CDC:             ~5mA
```

The SD card is often the surprise villain — cheap cards can draw 100mA+ during writes, while quality cards with better controllers draw far less.

---

## Step 2: Quick Wins — Reducing Active Power

Before diving into sleep modes, let's reduce consumption during active recording:

### 2.1 Reduce CPU Frequency

You don't need 240MHz to record audio. The I2S peripheral and DMA do the heavy lifting:

```cpp
#include "esp_pm.h"

void setupPowerManagement() {
    // Configure dynamic frequency scaling
    esp_pm_config_esp32s3_t pm_config = {
        .max_freq_mhz = 160,      // Max when busy
        .min_freq_mhz = 80,       // Min when idle
        .light_sleep_enable = false  // We'll handle sleep manually
    };

    esp_err_t ret = esp_pm_configure(&pm_config);
    if (ret != ESP_OK) {
        Serial.printf("PM config failed: %s\n", esp_err_to_name(ret));
    }
}
```

Or simply lock to a lower frequency:

```cpp
#include "esp32-hal-cpu.h"

void setup() {
    setCpuFrequencyMhz(80);  // 80MHz is plenty for audio recording
    Serial.printf("CPU frequency: %d MHz\n", getCpuFrequencyMhz());
}
```

At 80MHz, you'll save roughly 10-15mA compared to 240MHz.

### 2.2 Disable Unused Peripherals

Turn off what you're not using:

```cpp
void disableUnusedPeripherals() {
    // Disable WiFi (if not using)
    WiFi.mode(WIFI_OFF);

    // Disable Bluetooth (if not using)
    btStop();

    // ADC is often initialised by default
    // Only matters if you're not using it
}
```

### 2.3 LED Discipline

That cute RGB LED is a power hog:

```cpp
// DON'T: Leave LED on during recording
void badRecordingIndicator() {
    FastLED.showColor(CRGB::Red);  // 20mA, forever
}

// DO: Brief blink, then off
void goodRecordingIndicator() {
    FastLED.showColor(CRGB::Red);
    delay(100);
    FastLED.showColor(CRGB::Black);
    // Blink every 5 seconds to show still recording
}
```

### 2.4 SD Card Selection Matters

Test your SD card's power consumption:

```cpp
void measureSDPower() {
    // This is approximate - ideally use a current meter
    unsigned long writeStart = millis();

    File f = SD.open("/power_test.bin", FILE_WRITE);
    uint8_t buffer[4096];

    for (int i = 0; i < 100; i++) {  // Write 400KB
        f.write(buffer, sizeof(buffer));
    }
    f.close();

    unsigned long writeTime = millis() - writeStart;
    Serial.printf("Write time: %lu ms\n", writeTime);
    Serial.printf("Throughput: %.1f KB/s\n", 400000.0 / writeTime);
    // Faster = less time at high current = better battery life
}
```

A quality SanDisk or Samsung card often outperforms cheap alternatives significantly.

---

## Step 3: Implementing Recording Duty Cycles

If you don't need continuous 24/7 recording, duty cycling is your best friend.

### 3.1 Simple Periodic Recording

Record for a period, sleep, repeat:

```cpp
#include "esp_sleep.h"

const uint32_t RECORDING_DURATION_S = 60;     // Record for 1 minute
const uint32_t SLEEP_DURATION_S = 4 * 60;     // Sleep for 4 minutes
// Effective: 20% duty cycle

void setup() {
    Serial.begin(115200);

    // Check wake reason
    esp_sleep_wakeup_cause_t wakeup_reason = esp_sleep_get_wakeup_cause();

    switch (wakeup_reason) {
        case ESP_SLEEP_WAKEUP_TIMER:
            Serial.println("Woke from timer - starting recording");
            break;
        default:
            Serial.println("Initial boot or other wake source");
            break;
    }

    // Initialise hardware
    initializeSD();
    initializeI2S();

    // Record for specified duration
    recordAudio(RECORDING_DURATION_S);

    // Clean up
    deinitializeI2S();
    deinitializeSD();

    // Configure wake timer and sleep
    esp_sleep_enable_timer_wakeup(SLEEP_DURATION_S * 1000000ULL);
    Serial.println("Entering deep sleep...");
    Serial.flush();

    esp_deep_sleep_start();
    // Code never reaches here - deep sleep resets on wake
}

void loop() {
    // Never reached in this architecture
}
```

### 3.2 Proper Peripheral Deinitialisation

Before sleeping, cleanly shut down peripherals:

```cpp
void deinitializeI2S() {
    i2s_stop(I2S_NUM);
    i2s_driver_uninstall(I2S_NUM);
    Serial.println("I2S driver uninstalled");
}

void deinitializeSD() {
    // Ensure all files are closed and flushed
    SD.end();

    // Optionally: set SD card pins to known state
    // to prevent floating inputs drawing current
    pinMode(SD_CS_PIN, OUTPUT);
    digitalWrite(SD_CS_PIN, HIGH);  // Deselect card

    Serial.println("SD card unmounted");
}
```

### 3.3 Power Calculation Example

Let's calculate battery life for different scenarios:

```cpp
/*
 * Assumptions:
 * - M5 Capsule battery: ~120mAh (it's small)
 * - Recording current: 60mA average
 * - Deep sleep current: 25µA
 *
 * Scenario 1: Continuous recording
 * Battery life = 120mAh / 60mA = 2 hours
 *
 * Scenario 2: 20% duty cycle (1 min on, 4 min off)
 * Average current = (0.2 × 60mA) + (0.8 × 0.025mA) = 12.02mA
 * Battery life = 120mAh / 12.02mA = 10 hours
 *
 * Scenario 3: 5% duty cycle (3 min on, 57 min off)
 * Average current = (0.05 × 60mA) + (0.95 × 0.025mA) = 3.02mA
 * Battery life = 120mAh / 3.02mA = 40 hours
 */

void printBatteryEstimate(float dutyPercent) {
    const float batteryCapacity = 120.0;  // mAh
    const float activeCurrent = 60.0;      // mA
    const float sleepCurrent = 0.025;      // mA

    float avgCurrent = (dutyPercent / 100.0 * activeCurrent) +
                       ((100.0 - dutyPercent) / 100.0 * sleepCurrent);
    float hoursLife = batteryCapacity / avgCurrent;

    Serial.printf("Duty cycle: %.1f%%\n", dutyPercent);
    Serial.printf("Average current: %.2f mA\n", avgCurrent);
    Serial.printf("Estimated battery life: %.1f hours\n", hoursLife);
}
```

---

## Step 4: Wake-Up Sources

Deep sleep is useless if you can't wake up when needed.

### 4.1 Timer Wake-Up

The simplest option — wake after a fixed duration:

```cpp
void configureTimerWake(uint32_t seconds) {
    uint64_t sleep_us = seconds * 1000000ULL;
    esp_sleep_enable_timer_wakeup(sleep_us);
    Serial.printf("Will wake in %d seconds\n", seconds);
}
```

### 4.2 GPIO Wake-Up (Button Press)

Wake when a button is pressed:

```cpp
#define WAKE_BUTTON_PIN GPIO_NUM_0  // M5 Capsule button - verify for your hardware

void configureButtonWake() {
    // Configure the button pin
    pinMode(WAKE_BUTTON_PIN, INPUT_PULLUP);

    // For deep sleep, use ext0 (single pin) or ext1 (multiple pins)
    // ext0: wake on level
    esp_sleep_enable_ext0_wakeup(WAKE_BUTTON_PIN, 0);  // Wake when LOW

    Serial.println("Button wake configured");
}

void checkWakeReason() {
    esp_sleep_wakeup_cause_t reason = esp_sleep_get_wakeup_cause();

    switch (reason) {
        case ESP_SLEEP_WAKEUP_EXT0:
            Serial.println("Woke from button press");
            break;
        case ESP_SLEEP_WAKEUP_TIMER:
            Serial.println("Woke from timer");
            break;
        case ESP_SLEEP_WAKEUP_UNDEFINED:
        default:
            Serial.println("Not a deep sleep wake (initial boot?)");
            break;
    }
}
```

### 4.3 Multiple Wake Sources

Combine timer and button:

```cpp
void configureMultipleWakeSources() {
    // Wake on timer (e.g., record every 5 minutes)
    esp_sleep_enable_timer_wakeup(5 * 60 * 1000000ULL);

    // Also wake on button press
    esp_sleep_enable_ext0_wakeup(WAKE_BUTTON_PIN, 0);

    // The device wakes on whichever triggers first
    Serial.println("Will wake on timer OR button press");
}
```

---

## Step 5: Light Sleep for Lower Latency

Deep sleep resets the CPU — all your variables are lost. Light sleep preserves state but uses more power (~1-2mA vs ~25µA).

### 5.1 Basic Light Sleep

```cpp
void enterLightSleep(uint32_t duration_ms) {
    // Configure timer wake
    esp_sleep_enable_timer_wakeup(duration_ms * 1000ULL);

    Serial.println("Entering light sleep...");
    Serial.flush();

    // Enter light sleep (blocks until wake)
    esp_light_sleep_start();

    // Execution continues here after wake
    Serial.println("Woke from light sleep");
}
```

### 5.2 Automatic Light Sleep Between Tasks

FreeRTOS can automatically enter light sleep when idle:

```cpp
void setupAutoLightSleep() {
    esp_pm_config_esp32s3_t pm_config = {
        .max_freq_mhz = 160,
        .min_freq_mhz = 10,           // Very low when idle
        .light_sleep_enable = true     // Auto light sleep when idle
    };

    ESP_ERROR_CHECK(esp_pm_configure(&pm_config));
}
```

**Warning:** Auto light sleep can interfere with I2S timing. Test thoroughly before enabling during recording.

---

## Step 6: Preserving Data Across Deep Sleep

Deep sleep resets the CPU, but RTC memory survives:

### 6.1 RTC Memory Variables

```cpp
// This variable survives deep sleep
RTC_DATA_ATTR uint32_t bootCount = 0;
RTC_DATA_ATTR uint32_t totalRecordingSeconds = 0;
RTC_DATA_ATTR char lastFilename[32] = "";

void setup() {
    bootCount++;
    Serial.printf("Boot count: %d\n", bootCount);
    Serial.printf("Total recorded: %d seconds\n", totalRecordingSeconds);

    if (strlen(lastFilename) > 0) {
        Serial.printf("Last file: %s\n", lastFilename);
    }
}

void recordingComplete(const char* filename, uint32_t durationSeconds) {
    totalRecordingSeconds += durationSeconds;
    strncpy(lastFilename, filename, sizeof(lastFilename) - 1);
}
```

### 6.2 RTC Memory Limitations

- Maximum ~8KB of RTC slow memory
- Data is lost on power cycle (only survives deep sleep)
- Don't store pointers (addresses change between boots)

---

## Step 7: Practical Power Management Architecture

Here's a complete example combining these concepts:

```cpp
#include <Arduino.h>
#include "esp_sleep.h"
#include "driver/i2s.h"
#include <SD.h>

// RTC memory for persistence across deep sleep
RTC_DATA_ATTR uint32_t recordingNumber = 0;
RTC_DATA_ATTR uint32_t totalRecordedSeconds = 0;

// Configuration
const uint32_t RECORD_DURATION_S = 60;      // 1 minute recordings
const uint32_t SLEEP_DURATION_S = 4 * 60;   // 4 minutes between recordings
const gpio_num_t WAKE_BUTTON = GPIO_NUM_0;  // UPDATE FOR YOUR HARDWARE

// Forward declarations
void initializeHardware();
void shutdownHardware();
void recordToSD(uint32_t durationSeconds);
void blinkLED(int times, int delayMs);

void setup() {
    Serial.begin(115200);
    delay(100);

    // Check wake reason
    esp_sleep_wakeup_cause_t wakeReason = esp_sleep_get_wakeup_cause();

    switch (wakeReason) {
        case ESP_SLEEP_WAKEUP_TIMER:
            Serial.println("Scheduled wake - starting recording");
            break;
        case ESP_SLEEP_WAKEUP_EXT0:
            Serial.println("Button wake - manual recording");
            break;
        default:
            Serial.println("Fresh boot - starting recording cycle");
            recordingNumber = 0;
            totalRecordedSeconds = 0;
            break;
    }

    // Reduce CPU frequency for power saving
    setCpuFrequencyMhz(80);

    // Brief LED indication
    blinkLED(2, 100);

    // Initialise and record
    initializeHardware();
    recordToSD(RECORD_DURATION_S);
    shutdownHardware();

    // Update stats
    recordingNumber++;
    totalRecordedSeconds += RECORD_DURATION_S;

    Serial.printf("Recording #%d complete. Total recorded: %d seconds\n",
                  recordingNumber, totalRecordedSeconds);

    // Configure wake sources
    esp_sleep_enable_timer_wakeup(SLEEP_DURATION_S * 1000000ULL);
    esp_sleep_enable_ext0_wakeup(WAKE_BUTTON, 0);  // Wake on button LOW

    Serial.printf("Sleeping for %d seconds (or until button press)\n", SLEEP_DURATION_S);
    Serial.flush();

    // LED off before sleep
    blinkLED(1, 50);

    esp_deep_sleep_start();
}

void loop() {
    // Never reached
}

void initializeHardware() {
    // SD card init
    if (!SD.begin(SD_CS_PIN)) {
        Serial.println("SD init failed!");
        // Could enter error sleep state here
        return;
    }

    // I2S init (abbreviated - see previous guides)
    // ...

    Serial.println("Hardware initialised");
}

void shutdownHardware() {
    // Clean I2S shutdown
    i2s_stop(I2S_NUM);
    i2s_driver_uninstall(I2S_NUM);

    // SD card shutdown
    SD.end();

    // Set pins to prevent floating inputs
    pinMode(SD_CS_PIN, OUTPUT);
    digitalWrite(SD_CS_PIN, HIGH);

    Serial.println("Hardware shutdown complete");
}

void blinkLED(int times, int delayMs) {
    for (int i = 0; i < times; i++) {
        // Set LED on (implementation depends on your LED setup)
        delay(delayMs);
        // Set LED off
        delay(delayMs);
    }
}

void recordToSD(uint32_t durationSeconds) {
    // Implementation from previous guides
    Serial.printf("Recording for %d seconds...\n", durationSeconds);
    // ...
}
```

---

## Verification Checklist

Before moving on, verify:

- [ ] CPU frequency reduction works without breaking audio capture
- [ ] Deep sleep current measured (or estimated) at ~25µA
- [ ] Wake from timer functions correctly
- [ ] Wake from button functions correctly
- [ ] RTC variables persist across deep sleep cycles
- [ ] SD card properly deinitialized before sleep
- [ ] I2S properly deinitialized before sleep
- [ ] Battery life estimate calculated for your use case

---

## Common Issues

### High Sleep Current

**Symptom:** Deep sleep draws mA instead of µA

**Causes:**

- Floating GPIO pins
- Peripherals not properly disabled
- External components drawing power

**Solutions:**

```cpp
// Set unused pins to known states before sleep
void configurePinsForSleep() {
    // Example: set unused pins as inputs with pulldowns
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << GPIO_NUM_X) | (1ULL << GPIO_NUM_Y),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_ENABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    gpio_config(&io_conf);
}
```

### Won't Wake from Button

**Symptom:** Timer wake works, button doesn't

**Causes:**

- Wrong GPIO specified
- Pin not RTC-capable
- Button wired incorrectly

**Solutions:**

- Check M5 Capsule schematic for button GPIO
- Only certain GPIOs support deep sleep wake
- Verify button is actually pulling the pin LOW when pressed

### Audio Glitches After Wake

**Symptom:** First recording after wake has noise/glitches

**Causes:**

- I2S peripheral needs settling time
- Microphone needs startup time

**Solutions:**

```cpp
void initializeI2S() {
    // Normal I2S init...

    // Discard first 100ms of audio (mic settling)
    int16_t discardBuffer[256];
    size_t bytesRead;
    unsigned long settleStart = millis();
    while (millis() - settleStart < 100) {
        i2s_read(I2S_NUM, discardBuffer, sizeof(discardBuffer), &bytesRead, 10);
    }

    Serial.println("Microphone settled");
}
```

---

## What's Next

You've now got the tools to extend battery life from hours to days. But what happens when things go wrong? In [Step 5.2: Error Handling & Robustness](14-error-handling-robustness.md), we'll cover recovering from failures gracefully — because in embedded systems, failure is not a possibility but a certainty.

---

## Quick Reference

### Sleep Mode Selection

```
Need instant wake? → Light sleep
Can tolerate 100ms+ wake? → Deep sleep
Recording for hours? → Duty cycle with deep sleep
Waiting for user input? → Deep sleep with GPIO wake
```

### Key Functions

```cpp
// Deep sleep
esp_sleep_enable_timer_wakeup(microseconds);
esp_sleep_enable_ext0_wakeup(gpio_num, level);
esp_deep_sleep_start();

// Light sleep
esp_light_sleep_start();

// Wake reason
esp_sleep_get_wakeup_cause();

// RTC memory
RTC_DATA_ATTR int myVar = 0;

// CPU frequency
setCpuFrequencyMhz(80);
```

### Power Estimation

```
Average current = (duty% × active_mA) + ((100-duty)% × sleep_mA)
Battery hours = capacity_mAh / average_mA
```
