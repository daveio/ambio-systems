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
└── main.cpp              # Single-file application (3,500+ lines)
                          # Contains: setup(), loop(), embedded WAV data

platformio.ini            # Build config: m5stack-stamps3, m5stack-atom
.vscode/launch.json       # Debug configurations
```

### Entry Points

| Function     | Location           | Purpose                                         |
| ------------ | ------------------ | ----------------------------------------------- |
| `setup()`    | `src/main.cpp:75`  | Hardware init, peripheral config, startup tests |
| `loop()`     | `src/main.cpp:368` | Main loop: buttons, battery, RTC, IMU polling   |
| `app_main()` | `src/main.cpp:633` | ESP-IDF compatibility wrapper                   |

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

### Display Double-Buffering

```cpp
M5.Display.startWrite();
// ... batch updates ...
M5.Display.endWrite();
```

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

### Optional Display Includes

Uncomment in `main.cpp` as needed:

- `M5UnitOLED.h` (currently enabled)
- `M5UnitLCD.h`, `M5UnitGLASS.h`, `M5AtomDisplay.h`, etc.

## Development Notes

### Serial Monitor

- Baud rate: 115200
- Shows: board type, IMU detection, button events, sensor readings

### Known Structure Issues

- Single 3,500+ line file needs modularization
- 46KB WAV array embedded in source (should move to filesystem)
- Empty `include/`, `lib/`, `test/` directories

### Recommended Refactoring

1. Split `main.cpp` → `display.cpp`, `buttons.cpp`, `audio.cpp`, `sensors.cpp`
2. Move WAV data to SPIFFS/LittleFS
3. Add unit tests for logic components
4. Create header files for shared constants/types

## Hardware Requirements

- M5Stack StampS3 or Atom board
- USB-C cable for programming
- Optional: M5Stack Unit OLED display (currently configured)

## Project History

1. Started M5 Capsule audio pendant project, named _Ambio_
2. Built comprehensive hardware test suite
3. Next: Build actual pendant functionality

## Context7 Documentation Resources

Use the Context7 MCP tool to access comprehensive documentation for:

### Core Technologies

- **PlatformIO**: `/platformio/platformio-docs` (7,648 snippets, trust 8.4)
  - Complete build system, library manager, and IDE integration docs
  - Alternative: `/websites/platformio_en` (4,569 snippets, trust 7.5)

- **Arduino ESP32**: `/espressif/arduino-esp32` (1,420 snippets, trust 9.1)
  - Official Arduino core for ESP32, versions 3.3.2 and 3.3.4 available
  - Alternative: `/websites/espressif_projects_arduino-esp32_en` (624 snippets)

- **ESP-IDF**: `/websites/docs_espressif_com-projects-esp-idf-en-v5.4.1-esp32-get-started-index.html` (6,186 snippets, trust 7.5)
  - Official Espressif IoT Development Framework for ESP32-S3
  - Alternative: `/websites/espressif_projects_esp-idf_en_stable_esp32c3` (6,766 snippets, trust 10)

- **M5Stack Ecosystem**: `/websites/m5stack_en` (8,094 snippets, trust 7.5)
  - Complete documentation for M5Stack hardware, UiFlow, M5Burner, libraries

- **M5Unified Library**: `/m5stack/m5unified` (36 snippets, trust 8.7)
  - Hardware abstraction layer for M5Stack devices

- **M5GFX Library**: `/m5stack/m5gfx` (18 snippets, trust 8.7)
  - Graphics library for M5Stack displays

### Advanced Resources

- **ESP HAL (Rust)**: `/esp-rs/esp-hal` (201 snippets, trust 7.5)
  - For future Rust-based development considerations

- **NimBLE Arduino**: `/h2zero/nimble-arduino` (99 snippets, trust 9.6)
  - Bluetooth Low Energy library if needed for pendant features

These resources provide authoritative documentation, code examples, API references, and best practices for all major components of this project. Use them liberally to understand platform capabilities, troubleshoot issues, and implement features correctly.

## File Locations

| Purpose      | Path                         |
| ------------ | ---------------------------- |
| Main source  | `src/main.cpp`               |
| Build config | `platformio.ini`             |
| Debug config | `.vscode/launch.json`        |
| Dependencies | `.pio/libdeps/` (gitignored) |
| Build output | `.pio/build/` (gitignored)   |
