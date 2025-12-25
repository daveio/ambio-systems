# Ambio (Pendant)

Embedded firmware for M5Stack microcontrollers. Currently a comprehensive hardware test application for M5Stack StampS3 and Atom boards, validating all peripherals before building actual pendant functionality.

## Quick Start

```bash
# Build for primary target
pio run -e m5stack-stamps3

# Upload and monitor
pio run -e m5stack-stamps3 -t upload && pio device monitor -b 115200
```

## Technology Stack

- **Platform**: ESP32-S3 (240MHz dual-core, 8MB Flash, 320KB RAM)
- **Framework**: Arduino on ESP-IDF
- **Build System**: PlatformIO
- **Language**: C++ (Arduino dialect)
- **Libraries**: M5Unified (^0.2.11), M5GFX (^0.2.17)

## Hardware Support

Currently testing all peripherals:

- Display (M5UnitOLED + auto-detection)
- Buttons (BtnA/B/C, PWR, EXT, touch)
- Audio (speaker, tone generation, WAV playback)
- Power (battery monitoring)
- RTC (real-time clock with external unit support)
- IMU (MPU6050/6886/9250, BMI270, SH200Q)
- LED (brightness/color control)

## Development Resources

For detailed documentation, architecture, code patterns, and Context7 documentation resources, see [CLAUDE.md](CLAUDE.md).

## Project Status

1. ✅ Hardware test suite complete
2. 🚧 Building actual pendant functionality (next)

## Documentation

- **For AI Agents**: See [CLAUDE.md](CLAUDE.md) for complete technical documentation
- **For Humans**: This README provides a high-level overview
