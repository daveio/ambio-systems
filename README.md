# Ambio

Embedded firmware for M5Stack microcontrollers. A modular hardware test application for M5Stack StampS3 and Atom boards, validating all peripherals before building actual pendant functionality.

## Quick Start

```bash
# Build for primary target
pio run -e m5stack-stamps3

# Upload firmware
pio run -e m5stack-stamps3 -t upload

# Upload filesystem (audio assets)
pio run -e m5stack-stamps3 -t uploadfs

# Monitor serial output
pio device monitor -b 115200
```

## Technology Stack

- **Platform**: ESP32-S3 (240MHz dual-core, 8MB Flash, 320KB RAM)
- **Framework**: Arduino on ESP-IDF
- **Build System**: PlatformIO
- **Language**: C++ (Arduino dialect)
- **Libraries**: M5Unified (^0.2.11), M5GFX (^0.2.17), LittleFS (^2.0.0)
- **Filesystem**: LittleFS for audio assets and configuration

## Project Structure

The codebase follows a modular architecture with clean separation of concerns:

```plaintext
ambio/
├── src/
│   ├── main.cpp           # Application entry point (105 lines)
│   ├── hardware.cpp       # M5 init, board/IMU detection
│   ├── display.cpp        # Display management, rendering
│   ├── buttons.cpp        # Button handling with LED/audio feedback
│   ├── audio.cpp          # Speaker, tone generation, WAV playback
│   └── sensors.cpp        # Battery, RTC, IMU monitoring
├── include/
│   ├── types.h            # Shared constants and enums
│   ├── hardware.h         # Hardware module interface
│   ├── display.h          # Display module interface
│   ├── buttons.h          # Button module interface
│   ├── audio.h            # Audio module interface
│   └── sensors.h          # Sensor module interface
├── data/
│   └── audio/
│       └── startup.wav    # Startup sound (LittleFS)
└── platformio.ini         # Build configuration
```

### Module Responsibilities

| Module       | Purpose                  | Key Features                                            |
| ------------ | ------------------------ | ------------------------------------------------------- |
| **hardware** | M5Unified initialization | Board detection (31+ variants), IMU detection (6 types) |
| **display**  | Graphics rendering       | Multi-display support, frame batching, EPD mode         |
| **buttons**  | Input handling           | Template-based processing, state machine, feedback      |
| **audio**    | Sound output             | Tone generation, LittleFS WAV playback, volume control  |
| **sensors**  | Peripheral monitoring    | Battery tracking, RTC with fallback, IMU visualization  |

## Hardware Support

Currently testing all peripherals:

- **Display**: M5UnitOLED + auto-detection (LCD/GLASS/AtomDisplay)
- **Buttons**: BtnA/B/C, PWR, EXT, touch (6 state types)
- **Audio**: Speaker with tone generation (783Hz-2000Hz), WAV playback from filesystem
- **Power**: Battery level monitoring (0-100%, change detection)
- **RTC**: Real-time clock with external unit support, system time fallback
- **IMU**: MPU6050/6886/9250, BMI270, SH200Q (6-axis visualization)
- **LED**: RGB LED with brightness/color control

## Key Features

### Modular Architecture

- **Clean separation**: Each subsystem in its own module with clear interfaces
- **Minimal coupling**: Modules communicate through well-defined APIs
- **Easy testing**: Focused modules enable comprehensive unit testing

### LittleFS Filesystem

- **Asset storage**: Audio files stored separately from firmware
- **Smaller binaries**: 46KB WAV data not embedded in code
- **Update flexibility**: Change audio assets without recompiling

### Hardware Abstraction

- **31+ board support**: Automatic detection and configuration
- **6 IMU types**: Auto-detection with fallback gracefully
- **Multi-display**: Up to 8 displays with priority system

## Development Workflow

### Building

```bash
# Clean build
pio run -t clean

# Build primary target
pio run -e m5stack-stamps3

# Build secondary target
pio run -e m5stack-atom
```

### Uploading

```bash
# Flash firmware only
pio run -e m5stack-stamps3 -t upload

# Flash filesystem only
pio run -e m5stack-stamps3 -t uploadfs

# Flash both (firmware + filesystem)
pio run -e m5stack-stamps3 -t upload -t uploadfs
```

### Monitoring

```bash
# Serial monitor (115200 baud configured in platformio.ini)
pio device monitor

# Upload and monitor
pio run -e m5stack-stamps3 -t upload && pio device monitor
```

## Recent Improvements

### Code Quality

- ✅ **Refactored from 3,564 lines to modular architecture** (97% reduction in main.cpp)
- ✅ **Moved 46KB WAV data to LittleFS filesystem** (smaller firmware binary)
- ✅ **Implemented clean module boundaries** (hardware, display, buttons, audio, sensors)
- ✅ **Added comprehensive documentation** (inline comments, header docs)

### Architecture Benefits

- **Faster builds**: Incremental compilation of changed modules only
- **Easier maintenance**: Changes isolated to specific modules
- **Better testing**: Each module can be tested independently
- **Clearer code**: Obvious responsibilities and dependencies

## Project Status

1. ✅ Modular architecture implemented
2. ✅ LittleFS filesystem integration
3. 🚧 Hardware test suite (next)
4. 🚧 Building actual pendant functionality

## Documentation

- **For Humans**: This README provides project overview and getting started guide
- **For AI Agents**: See [.github/copilot-instructions.md](.github/copilot-instructions.md) for detailed technical documentation, code patterns, and Context7 resources

## Hardware Requirements

- **Required**: M5Stack StampS3 or Atom board
- **Required**: USB-C cable for programming
- **Optional**: M5Stack Unit OLED display (auto-detected)
- **Optional**: External IMU (auto-detected if internal not available)

## License

MIT. License details in [LICENSE](LICENSE) file.
