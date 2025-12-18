# 1.2 Your First ESP32-S3 Project

## Overview

Every journey of a thousand miles begins with a single step, and every embedded systems journey begins with blinking an LED. It's traditional. It's uninspiring. It's also the fastest way to confirm that your toolchain, board, and sanity are all functioning correctly.

The M5 Capsule has an RGB LED built in, which means you can blink in colour. Progress.

## What You'll Learn

- The Arduino `setup()` and `loop()` paradigm
- Basic GPIO control on the ESP32-S3
- Using the M5Capsule library
- Confirming your upload workflow actually works

## Prerequisites

- Completed [Step 1.1: PlatformIO Environment Setup](./01-platformio-environment-setup.md)
- The M5 Capsule connected via USB
- Low expectations (they will be met)

---

## Step 1: Understanding Arduino Program Structure

Open `src/main.cpp` in your project. It should contain something minimal:

```cpp
#include <Arduino.h>

void setup() {
    // Runs once at startup
}

void loop() {
    // Runs forever after setup()
}
```

This is the Arduino paradigm:

| Function  | When It Runs               | Purpose                                            |
| --------- | -------------------------- | -------------------------------------------------- |
| `setup()` | Once, at power-on or reset | Initialise hardware, configure pins, set up serial |
| `loop()`  | Repeatedly, forever        | Your main program logic                            |

Think of `setup()` as "before the play starts" and `loop()` as "the play itself, performed infinitely until someone pulls the plug."

## Step 2: Add the M5Capsule Library

The M5 Capsule has various peripherals (LED, buttons, IMU) that M5Stack provides a library to access. Let's add it.

Edit your `platformio.ini`:

```ini
[env:m5stack-stamps3]
platform = espressif32
board = m5stack-stamps3
framework = arduino

monitor_speed = 115200
monitor_filters = esp32_exception_decoder

upload_speed = 921600

build_flags =
    -DARDUINO_USB_MODE=1
    -DARDUINO_USB_CDC_ON_BOOT=1

board_build.partitions = default_16MB.csv

; Add the M5Capsule library
lib_deps =
    m5stack/M5Capsule@^1.0.2
```

The `lib_deps` section tells PlatformIO to download and include the M5Capsule library. On your next build, it will fetch this automatically.

## Step 3: Write the Blink Sketch

Replace the contents of `src/main.cpp` with:

```cpp
#include <Arduino.h>
#include <M5Capsule.h>

void setup() {
    // Initialise the M5Capsule (configures all peripherals)
    M5.begin();

    // Say hello via serial (we'll use this properly in Step 1.3)
    Serial.begin(115200);
    Serial.println("M5 Capsule initialised. Beginning LED blink test.");
}

void loop() {
    // Turn the LED red
    M5.Axp.setLed(1);  // LED on
    Serial.println("LED ON");
    delay(500);         // Wait 500ms

    // Turn the LED off
    M5.Axp.setLed(0);  // LED off
    Serial.println("LED OFF");
    delay(500);         // Wait 500ms
}
```

### What's Happening Here

1. **`#include <M5Capsule.h>`** — Includes the M5 library, which provides the `M5` object
2. **`M5.begin()`** — Initialises all the Capsule's hardware (power management, buttons, etc.)
3. **`M5.Axp.setLed()`** — Controls the power LED via the AXP2101 power management IC
4. **`delay(500)`** — Pauses execution for 500 milliseconds

The `delay()` function is blocking — nothing else happens while it's waiting. This is fine for blinking an LED but will become problematic later when we need to capture audio continuously. File that thought away for Phase 4.

## Step 4: Build and Upload

Connect your M5 Capsule via USB. Then:

```fish
pio run -t upload
```

Or click the upload button in VS Code.

Watch the output. You should see:

1. Compilation progress (lots of text)
2. "Connecting..." (PlatformIO trying to talk to the board)
3. Upload progress (a percentage bar)
4. "Hard resetting via RTS pin..." (upload complete, board rebooting)

If the LED starts blinking, congratulations. You've achieved the bare minimum. It only gets harder from here.

## Step 5: Enhance with RGB Colour

The basic `setLed()` only does on/off. But the M5 Capsule has an RGB LED (likely a NeoPixel/WS2812). Let's use it properly.

Update your code:

```cpp
#include <Arduino.h>
#include <M5Capsule.h>
#include <FastLED.h>

#define LED_PIN 21      // Check M5 documentation for exact pin
#define NUM_LEDS 1
#define LED_TYPE WS2812
#define COLOR_ORDER GRB

CRGB leds[NUM_LEDS];

void setup() {
    M5.begin();
    Serial.begin(115200);

    // Initialise FastLED
    FastLED.addLeds<LED_TYPE, LED_PIN, COLOR_ORDER>(leds, NUM_LEDS);
    FastLED.setBrightness(50);  // Don't blind yourself

    Serial.println("RGB LED test starting...");
}

void loop() {
    // Red
    leds[0] = CRGB::Red;
    FastLED.show();
    Serial.println("Red");
    delay(500);

    // Green
    leds[0] = CRGB::Green;
    FastLED.show();
    Serial.println("Green");
    delay(500);

    // Blue
    leds[0] = CRGB::Blue;
    FastLED.show();
    Serial.println("Blue");
    delay(500);

    // Off
    leds[0] = CRGB::Black;
    FastLED.show();
    Serial.println("Off");
    delay(500);
}
```

Add the FastLED library to `platformio.ini`:

```ini
lib_deps =
    m5stack/M5Capsule@^1.0.2
    fastled/FastLED@^3.6.0
```

**Note:** The LED pin (21 in this example) may vary. Check the [M5 Capsule documentation](https://docs.m5stack.com/en/core/M5Capsule) for the correct pin. If the LED doesn't respond, it's almost certainly the wrong pin number.

## Step 6: Add Button Interaction

The Capsule has a button. Let's make it do something:

```cpp
#include <Arduino.h>
#include <M5Capsule.h>
#include <FastLED.h>

#define LED_PIN 21
#define NUM_LEDS 1
#define LED_TYPE WS2812
#define COLOR_ORDER GRB

CRGB leds[NUM_LEDS];
int colourIndex = 0;
CRGB colours[] = {CRGB::Red, CRGB::Green, CRGB::Blue, CRGB::Yellow, CRGB::Purple, CRGB::Cyan};
int numColours = 6;

void setup() {
    M5.begin();
    Serial.begin(115200);

    FastLED.addLeds<LED_TYPE, LED_PIN, COLOR_ORDER>(leds, NUM_LEDS);
    FastLED.setBrightness(50);

    leds[0] = colours[colourIndex];
    FastLED.show();

    Serial.println("Press the button to change colour.");
}

void loop() {
    // Update button state
    M5.update();

    // Check if button was pressed
    if (M5.BtnA.wasPressed()) {
        colourIndex = (colourIndex + 1) % numColours;
        leds[0] = colours[colourIndex];
        FastLED.show();
        Serial.printf("Colour changed to index %d\n", colourIndex);
    }

    // Small delay to debounce
    delay(10);
}
```

### Key Points

- **`M5.update()`** — Must be called each loop to update button states
- **`M5.BtnA.wasPressed()`** — Returns true once per press (edge detection)
- **`%` modulo operator** — Wraps the index back to 0 after reaching the end

---

## Verification

Before moving on, confirm:

- [ ] The LED blinks when you upload the basic sketch
- [ ] You can see "LED ON" and "LED OFF" in the serial monitor (we'll cover this properly next)
- [ ] If you tried the RGB version, colours cycle correctly
- [ ] If you tried the button version, pressing the button changes the colour

If the LED doesn't blink:

1. Check the USB cable is a data cable
2. Check the board selected in `platformio.ini` is correct
3. Try holding BOOT while pressing RST, then upload again
4. Check the LED pin number matches your hardware

---

## Understanding What We've Built

This simple sketch demonstrates several concepts you'll use throughout the project:

| Concept                 | How We Used It         | Why It Matters Later                                  |
| ----------------------- | ---------------------- | ----------------------------------------------------- |
| Hardware initialisation | `M5.begin()`           | You'll initialise I2S, SD card, etc. the same way     |
| Periodic actions        | `delay()` in loop      | Audio capture needs precise timing (without blocking) |
| State tracking          | `colourIndex` variable | Recording state machine will track similar states     |
| Event detection         | Button press detection | Could trigger recording start/stop                    |
| Serial output           | `Serial.println()`     | Essential for debugging audio capture                 |

---

## What's Next

You've proven the hardware works and your workflow is functional. But right now, your only visibility into what the program is doing is whether an LED is lit. In [Step 1.3: Serial Debugging](./03-serial-debugging.md), we'll make `Serial.println()` your new best friend — the only entity that will truthfully tell you why your code isn't working.

---

## Quick Reference

```cpp
// Include the library
#include <M5Capsule.h>

// Initialise (call once in setup)
M5.begin();

// Update button states (call each loop)
M5.update();

// Check button
if (M5.BtnA.wasPressed()) { /* ... */ }
if (M5.BtnA.isPressed()) { /* held down */ }
if (M5.BtnA.wasReleased()) { /* just released */ }

// Control power LED
M5.Axp.setLed(1);  // On
M5.Axp.setLed(0);  // Off
```
