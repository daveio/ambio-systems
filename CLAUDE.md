# Ambio (Pendant)

Embedded firmware for M5Stack microcontrollers. Modular hardware test application for M5Stack StampS3 and Atom boards, validating all peripherals before building actual pendant functionality.

## Quick Reference

```bash
# Build
pio run -e m5stack-stamps3      # Primary target
pio run -e m5stack-atom         # Secondary target

# Upload firmware + filesystem
pio run -e m5stack-stamps3 -t upload -t uploadfs

# Monitor
pio device monitor -b 115200

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
| Filesystem   | LittleFS (^2.0.0)                       |

## Architecture

### Modular Structure (Post-Refactoring)

```plaintext
src/
├── main.cpp              # Application entry point (105 lines)
│                         # setup(), loop(), app_main()
├── hardware.cpp/h        # M5 init, board/IMU detection
├── display.cpp/h         # Display management, rendering
├── buttons.cpp/h         # Button handling with LED/audio feedback
├── audio.cpp/h           # Speaker, tone generation, WAV playback
└── sensors.cpp/h         # Battery, RTC, IMU monitoring

include/
├── types.h               # Shared constants, enums, button colors
├── hardware.h            # Hardware module interface
├── display.h             # Display module interface
├── buttons.h             # Button module interface
├── audio.h               # Audio module interface
└── sensors.h             # Sensor module interface

data/
└── audio/
    └── startup.wav       # Startup sound (LittleFS filesystem)

platformio.ini            # Build config + LittleFS settings
```

### Module Responsibilities

| Module   | Purpose                  | Key Functions                                 | Dependencies       |
| -------- | ------------------------ | --------------------------------------------- | ------------------ |
| hardware | M5Unified initialization | `hardware_init()`, board/IMU detection        | M5Unified          |
| display  | Graphics rendering       | `display_init()`, frame batching              | M5GFX, hardware    |
| buttons  | Input handling           | `buttons_update()`, template processing       | display, audio     |
| audio    | Sound output             | `audio_init()`, WAV playback, tone generation | LittleFS, hardware |
| sensors  | Peripheral monitoring    | `sensors_update()`, battery/RTC/IMU polling   | display, hardware  |

### Entry Points

| Function     | Location          | Purpose                                     |
| ------------ | ----------------- | ------------------------------------------- |
| `setup()`    | `src/main.cpp:44` | Orchestrates module initialization sequence |
| `loop()`     | `src/main.cpp:77` | Main update cycle: buttons + sensors        |
| `app_main()` | `src/main.cpp:98` | ESP-IDF compatibility wrapper               |

## Hardware Components

Currently testing/supporting:

- **Display**: M5UnitOLED (enabled), auto-detection for LCD/GLASS/AtomDisplay
- **Buttons**: BtnA/B/C, BtnPWR, BtnEXT, touch support (6 state types)
- **Audio**: Speaker with tone generation (783Hz-2000Hz), WAV playback from LittleFS
- **Power**: Battery level monitoring (0-100%, change detection)
- **RTC**: Real-time clock with external Unit RTC support, system time fallback
- **IMU**: MPU6050/6886/9250, BMI270, SH200Q auto-detection (6-axis visualization)
- **LED**: System LED with brightness/color control

## Code Patterns

### Module Initialization Order

Critical initialization sequence in `setup()`:

```cpp
void setup(void) {
    hardware_init();    // 1. M5Unified (must be first)
    display_init();     // 2. Display system
    buttons_init();     // 3. Button handlers
    audio_init();       // 4. Audio + LittleFS mount
    sensors_init();     // 5. Sensor subsystems

    display_print_board_info(get_board_name(), get_imu_name());
}
```

### Configuration-Driven Init (hardware.cpp)

```cpp
auto cfg = M5.config();
cfg.serial_baudrate = 115200;
cfg.internal_imu = true;
cfg.internal_rtc = true;
cfg.internal_spk = true;
cfg.led_brightness = 64;
cfg.external_display.unit_oled = true;
M5.begin(cfg);

// Set display priority
M5.setPrimaryDisplayType({m5::board_t::board_M5UnitOLED});
```

### State Tracking with Statics (sensors.cpp)

```cpp
static int prev_battery = INT_MAX;
static int prev_xpos[IMU_GRAPH_CHANNELS] = {0};

void update_battery() {
    int battery = M5.Power.getBatteryLevel();
    if (prev_battery != battery) {
        prev_battery = battery;
        // Update only on change (differential rendering)
    }
}
```

### Display Frame Batching (display.cpp)

```cpp
M5.Display.startWrite();
// ... batch all updates ...
M5.Display.endWrite();
```

### Template-Based Button Processing (buttons.cpp)

```cpp
template<typename ButtonType>
static int process_button(
    ButtonType& button,
    const char* name,
    uint16_t tone_freq,
    int display_row
) {
    int state = button.wasHold() ? 1
              : button.wasClicked() ? 2
              : button.wasPressed() ? 3
              : button.wasReleased() ? 4
              : button.wasDecideClickCount() ? 5
              : 0;

    if (state) {
        M5.Led.setAllColor(BUTTON_STATE_COLORS[state]);
        M5.Speaker.tone(tone_freq, BUTTON_TONE_DURATION_MS);
        // ... display update ...
    }
    return state;
}
```

### LittleFS WAV Playback (audio.cpp)

```cpp
void audio_play_startup() {
    File wav_file = LittleFS.open("/audio/startup.wav", "r");
    size_t file_size = wav_file.size();

    // Read into buffer
    wav_file.read(wav_buffer, file_size);
    wav_file.close();

    // Skip WAV header (44 bytes), play PCM data
    const uint8_t* pcm_data = wav_buffer + 44;
    size_t pcm_size = file_size - 44;
    M5.Speaker.playRaw(pcm_data, pcm_size, 44100, false);
}
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

| Environment       | Board            | Use Case                   | Filesystem |
| ----------------- | ---------------- | -------------------------- | ---------- |
| `m5stack-stamps3` | ESP32-S3 StampS3 | Primary development target | LittleFS   |
| `m5stack-atom`    | ESP32 Atom       | Secondary/legacy support   | LittleFS   |

Both environments:

- Share identical library dependencies (M5Unified, M5GFX, LittleFS)
- Use `default.csv` partition table for filesystem
- Auto-format LittleFS on first mount if needed

## Dependencies

### PlatformIO Libraries

- **M5Unified**: Hardware abstraction across 30+ M5Stack board variants
- **M5GFX**: Graphics rendering, multi-display support, sprites/canvas
- **LittleFS**: Filesystem for audio assets (better than SPIFFS: wear leveling, faster mount, power-loss resilient)

### Display Module Support

Enabled in `main.cpp`:

- `M5UnitOLED.h` (currently enabled)
- `M5UnitLCD.h`, `M5UnitGLASS.h`, `M5AtomDisplay.h` (commented, available)

## Development Notes

### Serial Monitor

- Baud rate: 115200
- Shows: board type, IMU detection, button events, sensor readings, LittleFS mount status

### Module Testing

Each module has clear responsibilities and minimal dependencies, enabling:

- **Isolated testing**: Test each module independently
- **Mock interfaces**: Easy to mock dependencies for unit tests
- **Fast iteration**: Change one module without affecting others

### Refactoring History

**Before (monolithic)**:

- `main.cpp`: 3,564 lines (all functionality embedded)
- 46KB WAV array in source code (bloats binary)
- No separation of concerns
- Difficult to maintain/test

**After (modular)**:

- `main.cpp`: 105 lines (orchestration only)
- WAV data in LittleFS filesystem
- Clean module boundaries with headers
- Easy to maintain/test/extend

**Metrics**:

- 97% reduction in main.cpp complexity
- 6 focused modules vs 1 monolith
- ~550 total lines of application code (excluding libraries)
- Firmware binary similar size (M5GFX/M5Unified dominate)

## Hardware Requirements

- M5Stack StampS3 or Atom board
- USB-C cable for programming
- Optional: M5Stack Unit OLED display (auto-detected)
- Optional: External IMU (auto-detected if internal unavailable)

## Project History

1. Started M5 Capsule audio pendant project, named _Ambio_
2. Built comprehensive hardware test suite (monolithic)
3. **Refactored to modular architecture** (hardware, display, buttons, audio, sensors)
4. **Migrated WAV data to LittleFS filesystem**
5. Next: Build actual pendant functionality

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

| Purpose        | Path                                                       |
| -------------- | ---------------------------------------------------------- |
| Main source    | `src/main.cpp`                                             |
| Module sources | `src/{hardware,display,buttons,audio,sensors}.cpp`         |
| Module headers | `include/{types,hardware,display,buttons,audio,sensors}.h` |
| Audio assets   | `data/audio/startup.wav`                                   |
| Build config   | `platformio.ini`                                           |
| Debug config   | `.vscode/launch.json`                                      |
| Dependencies   | `.pio/libdeps/` (gitignored)                               |
| Build output   | `.pio/build/` (gitignored)                                 |

## Key Technical Decisions

### Why LittleFS over SPIFFS?

- Better wear leveling → longer flash lifetime
- Faster mount times → quicker boot
- More resilient to power loss → safer for embedded use
- Future-proof for ESP-IDF 5.x+ (SPIFFS deprecated)

### Why Template Functions for Buttons?

- Type-safe polymorphism without virtual functions (no vtable overhead)
- Works with different M5 button types (Button_Class, TouchButton_Class)
- Compile-time code generation → zero runtime cost

### Why Static State in Modules?

- Avoids global namespace pollution
- Encapsulates state within module scope
- Clear ownership of data
- No heap allocations needed

### Why Module Headers?

- Clear public API vs implementation details
- Enables forward declarations
- Allows mock implementations for testing
- Documents module interfaces

## Recommended Next Steps

1. **Hardware verification**: Upload firmware + filesystem to device, test all peripherals
2. **Add unit tests**: Create test suite for module logic (button state machine, battery change detection, RTC formatting)
3. **Implement pendant features**: Build on modular foundation (BLE, sensors, custom UI)
4. **Optimize power**: Add sleep modes, wake-on-button, battery management
