# Ambio (Pendant)

Embedded firmware for M5Stack microcontrollers. Currently a comprehensive hardware test application for M5Stack StampS3 and Atom boards, validating all peripherals before building actual pendant functionality.

## Quick Reference

```bash
# Build
pio run -e m5stack-stamps3      # Primary target
pio run -e m5stack-atom         # Secondary target

# Upload + Monitor
pio run -e m5stack-stamps3 -t upload && pio device monitor -b 115200

# Clean
pio run -t clean
```

## Technology Stack

| Component    | Technology                              |
| ------------ | --------------------------------------- |
| Platform     | ESP32-S3 (240MHz, 8MB Flash, 320KB RAM) |
| Framework    | Arduino (on ESP-IDF)                    |
| Build System | PlatformIO                              |
| Language     | C++ (Arduino dialect)                   |
| Libraries    | M5Unified (^0.2.11), M5GFX (^0.2.17)    |

## Architecture

```plaintext
src/
├── main.cpp          # Application entry points (setup/loop)
├── hardware.cpp      # M5Unified initialization, board/IMU detection
├── display.cpp       # Display management, rendering utilities
├── buttons.cpp       # Button input handling, LED/audio feedback
├── audio.cpp         # Speaker control, tone generation
└── sensors.cpp       # Battery, RTC, IMU polling and visualization

include/
├── types.h           # Shared constants, enums, structs
├── hardware.h        # Hardware init API
├── display.h         # Display module API
├── buttons.h         # Button handling API
├── audio.h           # Audio module API
└── sensors.h         # Sensor module API

platformio.ini        # Build config: m5stack-stamps3, m5stack-atom
```

### Entry Points

| Function     | Location       | Purpose                            |
| ------------ | -------------- | ---------------------------------- |
| `setup()`    | `src/main.cpp` | Initialize all modules in order    |
| `loop()`     | `src/main.cpp` | Main loop: M5.update(), subsystems |
| `app_main()` | `src/main.cpp` | ESP-IDF compatibility wrapper      |

### Module Initialization Order

```cpp
hardware_init();  // M5.begin(), board detection
display_init();   // Display config, test pattern
buttons_init();   // Button state tracking
audio_init();     // Speaker setup, startup sound
sensors_init();   // Sensor subsystem ready
```

## Critical: M5Unified Include Order

**Display headers MUST be included BEFORE M5Unified.h** for auto-detection to work.

```cpp
// CORRECT - display works:
#include <M5UnitOLED.h>   // Defines __M5GFX_M5UNITOLED__ macro
#include <M5Unified.h>    // Checks for macro, enables OLED support

// WRONG - display fails silently:
#include <M5Unified.h>    // Doesn't see the macro yet
#include <M5UnitOLED.h>   // Too late, M5Unified already compiled without OLED
```

**Why this matters:**

- M5UnitOLED.h defines preprocessor macros (e.g., `__M5GFX_M5UNITOLED__`)
- M5Unified.h checks `#if defined(__M5GFX_M5UNITOLED__)` to enable driver support
- Wrong order = macro not visible = OLED driver not compiled in = display doesn't work
- This is a compile-time issue with no runtime errors - it just silently fails

## Hardware Components

Currently testing/supporting:

- **Display**: M5UnitOLED (enabled), auto-detection for LCD/GLASS/AtomDisplay
- **Buttons**: BtnA/B/C, BtnPWR, BtnEXT, touch support (6 types)
- **Audio**: Speaker with tone generation (783Hz-2000Hz), WAV playback
- **Power**: Battery level monitoring (0-100%)
- **RTC**: Real-time clock with external Unit RTC support
- **IMU**: MPU6050/6886/9250, BMI270, SH200Q auto-detection
- **LED**: System LED with brightness/color control

## Code Patterns

### Configuration-Driven Init

```cpp
auto cfg = M5.config();
cfg.serial_baudrate = 115200;
cfg.internal_imu = true;
cfg.external_display.unit_oled = true;
cfg.led_brightness = 64;
M5.begin(cfg);
```

### State Tracking with Statics

```cpp
static int prev_battery = INT_MAX;
if (battery != prev_battery) {
    prev_battery = battery;
    // Update only on change
}
```

### Display Transactions

```cpp
// Each subsystem manages its own display transactions
M5.Display.startWrite();
// ... batch updates ...
M5.Display.endWrite();

// Final flush at end of loop
M5.Display.display();
```

**Important:** Don't nest `startWrite()`/`endWrite()` calls. Each module should handle its own complete transaction.

### Logging Levels

```cpp
M5_LOGE("error");    // Error
M5_LOGW("warning");  // Warning
M5_LOGI("info");     // Info (default visible)
M5_LOGD("debug");    // Debug
M5_LOGV("verbose");  // Verbose
```

## Build Environments

| Environment       | Board            | Use Case                   |
| ----------------- | ---------------- | -------------------------- |
| `m5stack-stamps3` | ESP32-S3 StampS3 | Primary development target |
| `m5stack-atom`    | ESP32 Atom       | Secondary/legacy support   |

Both share identical library dependencies (M5Unified, M5GFX).

## Dependencies

### PlatformIO Libraries

- **M5Unified**: Hardware abstraction across 30+ M5Stack board variants
- **M5GFX**: Graphics rendering, multi-display support, sprites/canvas

### Display Includes

Enable in source files as needed (remember: include BEFORE M5Unified.h):

- `M5UnitOLED.h` (currently enabled)
- `M5UnitLCD.h`, `M5UnitGLASS.h`, `M5AtomDisplay.h`, etc.

## Development Notes

### Serial Monitor

- Baud rate: 115200
- Shows: board type, IMU detection, button events, sensor readings

### Future Work

- Move WAV data to LittleFS filesystem
- Add unit tests for logic components
- Implement actual pendant functionality

## Hardware Requirements

- M5Stack StampS3 or Atom board
- USB-C cable for programming
- Optional: M5Stack Unit OLED display (currently configured)

## Project History

1. Started M5 Capsule audio pendant project, named _Ambio_
2. Built comprehensive hardware test suite (monolithic)
3. Refactored into modular architecture
4. Next: Build actual pendant functionality

## Context7 Documentation Resources

Use the Context7 MCP tool to access comprehensive documentation for:

### Core Technologies

- **PlatformIO**: `/platformio/platformio-docs` (7,648 snippets, trust 8.4)
- **Arduino ESP32**: `/espressif/arduino-esp32` (1,420 snippets, trust 9.1)
- **ESP-IDF**: `/websites/docs_espressif_com-projects-esp-idf-en-v5.4.1-esp32-get-started-index.html` (6,186 snippets, trust 7.5)
- **M5Stack Ecosystem**: `/websites/m5stack_en` (8,094 snippets, trust 7.5)
- **M5Unified Library**: `/m5stack/m5unified` (36 snippets, trust 8.7)
- **M5GFX Library**: `/m5stack/m5gfx` (18 snippets, trust 8.7)

### Advanced Resources

- **ESP HAL (Rust)**: `/esp-rs/esp-hal` (201 snippets, trust 7.5)
- **NimBLE Arduino**: `/h2zero/nimble-arduino` (99 snippets, trust 9.6)

## File Locations

| Purpose      | Path                         |
| ------------ | ---------------------------- |
| Main source  | `src/main.cpp`               |
| Modules      | `src/*.cpp`                  |
| Headers      | `include/*.h`                |
| Build config | `platformio.ini`             |
| Debug config | `.vscode/launch.json`        |
| Dependencies | `.pio/libdeps/` (gitignored) |
| Build output | `.pio/build/` (gitignored)   |
