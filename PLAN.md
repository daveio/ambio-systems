# Ambio Refactoring Plan: Modularize main.cpp

_This plan has been completed with the exception of tests. It is kept for reference when we do actually implement the tests._

---

## Overview

Refactor the monolithic 3,564-line `src/main.cpp` into modular components, improving maintainability, testability, and build performance.

## Current State Analysis

- **Single file**: 3,564 lines containing all functionality
- **Embedded assets**: 46KB WAV array (2,914 lines) in source code
- **No separation of concerns**: Display, buttons, audio, sensors all mixed in setup()/loop()
- **Static state scattered**: Battery, IMU, timing state variables throughout
- **Empty directories**: `include/`, `lib/`, `test/` unused

## Target Architecture

```
src/
├── main.cpp              # Entry point: setup(), loop(), module orchestration
├── hardware.cpp/h        # M5 init, device detection, board enumeration
├── display.cpp/h         # Display init, rendering, text output, buffer ops
├── buttons.cpp/h         # Button polling, state machine, LED/audio feedback
├── audio.cpp/h           # Speaker init, tone generation, WAV playback
├── sensors.cpp/h         # Battery, RTC, IMU polling and visualization
└── types.h               # Shared constants, enums, common types

data/
└── audio/
    └── startup.wav       # Moved from embedded array to SPIFFS

test/
└── test_*/              # Unit tests for each module
```

## Phase 1: Create Foundation Headers

### 1.1 Create `include/types.h`

**Purpose**: Shared constants, types, and configuration used across modules

**Contents**:

- Common display constants (colors, sizes, positions)
- Button state enums
- Sensor configuration constants
- Frame rate control values
- Type aliases for clarity

**Dependencies**: None (foundation file)

### 1.2 Create `include/hardware.h`

**Purpose**: Hardware initialization and device detection interface

**Public API**:

```cpp
void hardware_init();
const char* get_board_name();
const char* get_imu_name();
bool has_imu();
```

**Moved from main.cpp**:

- Lines 75-196: M5.begin() configuration
- Lines 291-314: Board type detection switch
- Lines 333-353: IMU type detection switch

## Phase 2: Extract Display Module

### 2.1 Create `src/display.cpp` + `include/display.h`

**Public API**:

```cpp
void display_init();
void display_set_brightness(uint8_t level);
void display_clear();
void display_print_board_info(const char* board, const char* imu);
void display_begin_frame();
void display_end_frame();
```

**Moved from main.cpp**:

- Lines 247-266: Display setup (brightness, rotation, multi-display)
- Lines 269-271: Text size configuration
- Lines 327-359: Board/IMU info display
- All `M5.Display.*` operations from loop()

**State Management**:

- Display handle caching
- Current cursor position tracking
- Frame buffer state

## Phase 3: Extract Button Module

### 3.1 Create `src/buttons.cpp` + `include/buttons.h`

**Public API**:

```cpp
void buttons_init();
void buttons_update();  // Poll all buttons, handle state changes
bool button_was_pressed(uint8_t button_id);
```

**Moved from main.cpp**:

- Lines 375-492: Button test logic (BtnPWR, A, B, C, EXT)
- Line 389: Color mapping array for button states
- Line 390: State name strings
- Button state machine: wasHold/wasClicked/wasPressed/wasReleased
- LED feedback integration
- Tone feedback per button

**State Management**:

- Static button state tracking
- Previous state for edge detection
- LED color state

## Phase 4: Extract Audio Module

### 4.1 Create `src/audio.cpp` + `include/audio.h`

**Public API**:

```cpp
void audio_init();
void audio_set_volume(uint8_t level);
void audio_play_tone(uint16_t frequency, uint32_t duration_ms);
void audio_play_startup();  // Plays startup WAV from filesystem
```

**Moved from main.cpp**:

- Lines 212-230: Speaker init and test
- Line 215: Volume setup
- Lines 218, 224: Tone generation
- Line 230: WAV playback logic
- Line 52: WAV data declaration (will reference filesystem instead)

**State Management**:

- Speaker enabled state
- Current volume level

## Phase 5: Extract Sensor Module

### 5.1 Create `src/sensors.cpp` + `include/sensors.h`

**Public API**:

```cpp
void sensors_init();
void sensors_update();  // Poll all sensors, update displays
int  sensors_get_battery_level();
bool sensors_rtc_available();
bool sensors_imu_available();
```

**Moved from main.cpp**:

- Lines 502-520: Battery polling and display
- Lines 521-577: RTC date/time retrieval
- Lines 579-631: IMU accelerometer/gyroscope reading and graphing
- Line 503: Static `prev_battery` state
- Line 524: Weekday array `wd[]`
- Line 583: Static `prev_xpos[6]` for IMU graphing

**State Management**:

- Previous battery level (change detection)
- Previous IMU graph positions
- RTC availability cache

## Phase 6: Move WAV Data to Filesystem

### 6.1 Extract WAV Array to Binary File

**Current state**:

- Lines 651-3564: 46KB embedded array
- Compiled into firmware (wastes flash)

**Target state**:

- Create `data/audio/startup.wav` binary file
- Mount SPIFFS/LittleFS at boot
- Load WAV dynamically in `audio_play_startup()`

**Implementation**:

1. Extract raw bytes from `wav_8bit_44100[]` array
2. Create proper WAV file header (RIFF, fmt, data chunks)
3. Save to `data/audio/startup.wav`
4. Update `platformio.ini` to include SPIFFS upload
5. Modify `audio.cpp` to load from filesystem

**Filesystem config**:

```ini
# platformio.ini addition
board_build.filesystem = littlefs
board_build.partitions = default.csv

# LittleFS upload command:
# pio run -t uploadfs -e m5stack-stamps3
```

**Why LittleFS?**

- Better wear leveling than SPIFFS
- Faster mount times
- More resilient to power loss
- Future-proof for ESP-IDF 5.x+

## Phase 7: Refactor main.cpp

### 7.1 Update `src/main.cpp`

**New structure** (~200 lines):

```cpp
#include "hardware.h"
#include "display.h"sddsff
#include "buttons.h"
#include "audio.h"
#include "sensors.h"

void setup() {
    hardware_init();
    display_init();
    buttons_init();
    audio_init();
    sensors_init();

    display_print_board_info(get_board_name(), get_imu_name());
    audio_play_startup();
}

void loop() {
    M5.update();

    display_begin_frame();
    buttons_update();
    sensors_update();
    display_end_frame();
}

void app_main() {
    setup();
    while (true) {
        loop();
    }
}
```

**Benefits**:

- Clear initialization sequence
- Obvious module dependencies
- Easy to understand control flow
- Simple to extend with new modules

## Phase 8: Add Unit Tests

### 8.1 Test Structure - Full Coverage

Create comprehensive tests for all modules with testable logic:

**`test/test_hardware/test_hardware.cpp`**:

- Device detection accuracy
- M5.begin() configuration validation
- Board/IMU type string mapping

**`test/test_display/test_display.cpp`**:

- Text rendering bounds checking
- Color conversion utilities
- Buffer overflow protection
- Multi-display initialization

**`test/test_buttons/test_buttons.cpp`**:

- State machine transitions (6 states: released, pressed, clicked, etc.)
- Edge detection accuracy (press/release/hold)
- Hold timing thresholds (debounce logic)
- LED color mapping correctness
- Button ID validation

**`test/test_sensors/test_sensors.cpp`**:

- Battery level validation (0-100%, INT_MAX handling)
- RTC date/time parsing and formatting
- Weekday calculation accuracy
- IMU data range checking (-2g to +2g accel, gyro bounds)
- Change detection logic (prev_battery pattern)

**`test/test_audio/test_audio.cpp`**:

- WAV file loading from LittleFS
- File format validation (RIFF header, sample rate)
- Tone frequency bounds (20Hz-20kHz human range)
- Volume level validation (0-255)
- Speaker initialization state

**PlatformIO test config**:

```ini
# platformio.ini additions
test_framework = unity
test_build_src = yes
test_speed = 115200

# Native testing for logic (no hardware needed)
[env:native]
platform = native
test_build_src = yes

# Hardware-in-loop testing (requires device)
[env:test_stamps3]
extends = env:m5stack-stamps3
test_build_src = yes
```

**Test Execution**:

```bash
# Run all tests on native (logic only)
pio test -e native

# Run hardware tests on device
pio test -e test_stamps3

# Run specific test
pio test -e native -f test_buttons
```

## Phase 9: Update Documentation

### 9.1 Update `CLAUDE.md`

- Remove "Known Structure Issues" section
- Update Architecture diagram with new module structure
- Add "Module Responsibilities" section
- Update file locations table

### 9.2 Update `README.md`

- Document new project structure
- Add build instructions for SPIFFS upload
- Explain module organization

## Implementation Order

1. ✅ **Plan creation** (this document)
2. **Foundation** (Phase 1): Create `types.h` and `hardware.h`
3. **Incremental extraction**: One module at a time (Display → Buttons → Audio → Sensors)
4. **Filesystem migration** (Phase 6): Move WAV data
5. **Main refactor** (Phase 7): Update main.cpp to use modules
6. **Testing** (Phase 8): Add unit tests
7. **Documentation** (Phase 9): Update project docs
8. **Verification**: Build, upload, test on hardware

## Critical Files to Modify

- **Create new**: `include/types.h`, `include/hardware.h`, `include/display.h`, `include/buttons.h`, `include/audio.h`, `include/sensors.h`
- **Create new**: `src/hardware.cpp`, `src/display.cpp`, `src/buttons.cpp`, `src/audio.cpp`, `src/sensors.cpp`
- **Modify**: `src/main.cpp` (reduce from 3,564 to ~200 lines)
- **Create new**: `data/audio/startup.wav`
- **Modify**: `platformio.ini` (add filesystem, test framework)
- **Update**: `CLAUDE.md`, `README.md`

## Risk Mitigation

1. **Incremental approach**: Extract one module at a time, build/test after each
2. **Git commits**: Commit after each successful module extraction
3. **Preserve behavior**: Ensure identical functionality after refactoring
4. **Hardware testing**: Upload to device after major changes
5. **Rollback plan**: Each commit is a safe restore point

## Success Criteria

- ✅ `main.cpp` reduced to < 250 lines
- ✅ All functionality preserved (hardware tests pass)
- ✅ WAV data moved to filesystem
- ✅ Clean module boundaries with minimal coupling
- ✅ Unit tests for core logic
- ✅ Documentation updated
- ✅ Builds without errors/warnings
- ✅ Uploads and runs on M5Stack StampS3

## Estimated Impact

**Before**:

- 1 file: 3,564 lines
- Build time: ~45s (rough estimate)
- Cognitive load: High
- Testability: None

**After**:

- 7+ files: ~1,500 lines total (modules) + tests
- Build time: ~30s (incremental builds faster)
- Cognitive load: Low (clear separation)
- Testability: High (unit tests per module)
