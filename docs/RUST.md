# Switching to Rust for ESP32-S3 Development

This document provides a comprehensive guide to transitioning the Ambio project from Arduino/C++ to Rust, covering setup, tooling, architecture decisions, and practical migration strategies.

## Table of Contents

- [Why Rust for ESP32-S3?](#why-rust-for-esp32-s3)
- [Two Development Approaches](#two-development-approaches)
- [Getting Started](#getting-started)
- [Project Setup](#project-setup)
- [Hardware Access Patterns](#hardware-access-patterns)
- [Async Programming with Embassy](#async-programming-with-embassy)
- [Comparison with Arduino/C++](#comparison-with-arduinoc)
- [Migration Strategy](#migration-strategy)
- [Resources and Documentation](#resources-and-documentation)

## Why Rust for ESP32-S3?

### Memory Safety Without Runtime Cost

Rust's ownership and borrowing system eliminates entire classes of bugs at compile time:

- **No use-after-free errors**: The compiler prevents accessing freed memory
- **No data races**: Concurrent access is validated at compile time
- **No null pointer dereferences**: The `Option` type makes null explicit
- **No buffer overflows**: Bounds checking is enforced

These guarantees come with **zero runtime cost**—the safety checks happen during compilation, not execution.

### Performance Equivalent to C/C++

- Both Rust and C++ compile to efficient machine code through LLVM
- Zero-cost abstractions mean high-level code compiles to optimal assembly
- Binary sizes: 100-500 KB for bare-metal projects (comparable to optimized C++)
- Deterministic execution with no garbage collection pauses

### Production-Ready Ecosystem

- **Official Espressif support** through the esp-rs project
- **Growing adoption**: Microsoft, Google, Amazon, Volvo use Rust for embedded systems
- **Mature tooling**: cargo package manager, excellent IDE support, hardware debuggers
- **Active community**: esp-rs Matrix channel, comprehensive documentation

## Two Development Approaches

Espressif supports two fundamentally different Rust development paths for ESP32-S3:

### Option 1: Bare-Metal (`no_std`) with esp-hal

**What it is**: Direct hardware access through a Hardware Abstraction Layer, no operating system.

**Advantages**:

- Smaller binaries (100-500 KB typical)
- Deterministic execution—no RTOS task switching
- Full control over timing and resources
- Simple build process (just `cargo build`)
- Future-facing: aligns with embedded Rust ecosystem direction

**Disadvantages**:

- More hands-on hardware knowledge required
- WiFi stack reimplemented in Rust (functional but less mature than ESP-IDF)
- Some advanced features require more work than IDF approach

**Best for**:

- Real-time applications requiring predictable timing
- Battery-powered devices needing minimal overhead
- Projects wanting full control over hardware
- Learning embedded systems at a low level

### Option 2: Standards Library (`std`) with esp-idf-hal

**What it is**: Rust bindings to ESP-IDF (Espressif's C framework with FreeRTOS).

**Advantages**:

- Access to Rust standard library (`Vec`, `String`, `HashMap`, threading)
- Mature WiFi/Bluetooth stacks from ESP-IDF
- Advanced features: OTA updates, secure boot, WiFi provisioning
- Familiar threading model for systems programmers

**Disadvantages**:

- Larger binaries (1-3 MB due to RTOS kernel)
- Non-deterministic task scheduling
- More complex build process
- Debugging sometimes requires understanding C layer

**Best for**:

- IoT applications heavily using WiFi/Bluetooth
- Projects needing ESP-IDF's mature networking features
- Teams familiar with RTOS development
- Rapid prototyping with standard library conveniences

### Recommendation for Ambio

**Start with bare-metal esp-hal** for these reasons:

1. The pendant is a real-time audio device—deterministic timing matters
2. Battery life benefits from minimal overhead
3. Current Arduino code already works at hardware level (easy mental model)
4. Embassy async framework provides concurrency without RTOS complexity
5. Smaller binaries leave more room for audio data and future features

You can always switch to esp-idf-hal later if you need mature WiFi features.

## Getting Started

### 1. Install Rust Toolchain

The ESP32-S3 uses the Xtensa architecture, which requires Espressif's custom toolchain:

```bash
# Install espup (manages Xtensa Rust toolchain)
cargo install espup

# Install the Xtensa toolchain (5-10 GB, takes 15-30 minutes)
espup install

# Source the environment (add to your shell's rc file)
source ~/export-esp.sh
```

**Note**: RISC-V ESP32 variants (ESP32-C3, C6, H2) use standard Rust toolchains:

```bash
rustup target add riscv32imc-unknown-none-elf
```

### 2. Install Flashing Tool

```bash
# Install espflash for uploading firmware
cargo install espflash

# Add your user to dialout group (Linux only)
sudo usermod -a -G dialout $USER
# Log out and back in for group changes to take effect
```

### 3. Install Debugging Tools (Optional but Recommended)

```bash
# Install probe-rs for hardware debugging
cargo install probe-rs-tools

# Install VS Code extension: "probe-rs-debugger"
```

### 4. Install Project Generator

```bash
# Install cargo-generate for creating projects from templates
cargo install cargo-generate
```

## Project Setup

### Create New Project

```bash
# Generate from official template
cargo generate esp-rs/esp-template

# Answer prompts:
# - Project name: ambio-rust
# - ESP32 variant: esp32s3
# - Approach: bare-metal (no_std)
# - Enable heap: yes
# - Enable logging: yes (defmt)
# - WiFi/Bluetooth: no (for now)
# - DevContainers: no
# - CI: no (for now)

cd ambio-rust
```

### Project Structure

```plaintext
ambio-rust/
├── .cargo/
│   └── config.toml        # Target and linker configuration
├── src/
│   └── main.rs           # Entry point
├── Cargo.toml            # Dependencies and features
├── rust-toolchain.toml   # Locks toolchain version
└── .vscode/
    └── settings.json     # Editor configuration
```

### Key Configuration Files

**`.cargo/config.toml`**:

```toml
[build]
target = "xtensa-esp32s3-none-elf"

[target.xtensa-esp32s3-none-elf]
runner = "espflash flash --monitor"
```

**`Cargo.toml`** (bare-metal example):

```toml
[dependencies]
esp-hal = { version = "0.21", features = ["esp32s3"] }
esp-backtrace = { version = "0.14", features = ["esp32s3", "panic-handler", "exception-handler", "println"] }
esp-println = { version = "0.12", features = ["esp32s3", "log"] }
log = "0.4"

# For async support
embassy-executor = { version = "0.6", features = ["arch-xtensa", "executor-thread"] }
embassy-time = "0.3"

[profile.release]
opt-level = "z"      # Optimize for size
lto = true           # Link-time optimization
strip = true         # Strip debug symbols
```

### Build and Flash

```bash
# Build optimized binary
cargo build --release

# Flash to device and monitor serial output
cargo run --release

# Or flash without monitoring
espflash flash target/xtensa-esp32s3-none-elf/release/ambio-rust
```

## Hardware Access Patterns

### Basic GPIO (LED Blinking)

```rust
#![no_std]
#![no_main]

use esp_hal::{
    gpio::{Level, Output, OutputConfig},
    main,
    delay::Delay,
};

#[main]
fn main() -> ! {
    let peripherals = esp_hal::init(esp_hal::Config::default());

    // Configure GPIO as output (e.g., LED pin)
    let mut led = Output::new(
        peripherals.GPIO0,
        Level::High,
        OutputConfig::default()
    );

    let delay = Delay::new();

    loop {
        led.toggle();
        delay.delay_millis(500);
    }
}
```

### I2C Communication

```rust
use esp_hal::{
    i2c::master::{Config, I2c},
    time::Rate,
};

let config = Config::default()
    .with_frequency(Rate::from_khz(100)); // Standard mode

let mut i2c = I2c::new(peripherals.I2C0, config)
    .unwrap()
    .with_sda(peripherals.GPIO2)
    .with_scl(peripherals.GPIO3);

// Read from device at address 0x77
const DEVICE_ADDR: u8 = 0x77;
let write_buffer = [0xAA, 0xBB];
let mut read_buffer = [0u8; 22];

// Write then read (common sensor pattern)
i2c.write_read(DEVICE_ADDR, &write_buffer, &mut read_buffer)
    .unwrap();
```

### SPI Communication

```rust
use esp_hal::{
    spi::{
        Mode,
        master::{Config, Spi},
    },
    time::Rate,
};

let mut spi = Spi::new(
    peripherals.SPI2,
    Config::default()
        .with_frequency(Rate::from_khz(100))
        .with_mode(Mode::_0),  // CPOL=0, CPHA=0
)
.unwrap()
.with_sck(peripherals.GPIO0)
.with_mosi(peripherals.GPIO4)
.with_miso(peripherals.GPIO2)
.with_cs(peripherals.GPIO5);

let mut data = [0xde, 0xca, 0xfb, 0xad];
spi.transfer(&mut data).unwrap();
```

### Button Input with Interrupts

```rust
use core::cell::RefCell;
use critical_section::Mutex;
use esp_hal::{
    gpio::{Event, Input, InputConfig, Io, Pull},
    handler,
    ram,
};

// Global static for interrupt handler
static BUTTON: Mutex<RefCell<Option<Input>>> =
    Mutex::new(RefCell::new(None));

#[main]
fn main() -> ! {
    let peripherals = esp_hal::init(esp_hal::Config::default());

    let mut io = Io::new(peripherals.IO_MUX);
    io.set_interrupt_handler(interrupt_handler);

    // Configure with pull-up resistor
    let config = InputConfig::default().with_pull(Pull::Up);
    let mut button = Input::new(peripherals.GPIO0, config);

    critical_section::with(|cs| {
        button.listen(Event::FallingEdge);
        BUTTON.borrow_ref_mut(cs).replace(button);
    });

    loop {
        // Main loop continues while interrupts handle button
    }
}

#[handler]
#[ram]  // Run from RAM for faster execution
fn interrupt_handler() {
    critical_section::with(|cs| {
        if let Some(btn) = BUTTON.borrow_ref_mut(cs).as_mut() {
            if btn.is_interrupt_set() {
                esp_println::println!("Button pressed!");
                btn.clear_interrupt();
            }
        }
    });
}
```

## Async Programming with Embassy

Embassy provides cooperative multitasking without an RTOS, perfect for real-time embedded systems.

### Setup Embassy

**Add to `Cargo.toml`**:

```toml
[dependencies]
embassy-executor = { version = "0.6", features = ["arch-xtensa", "executor-thread"] }
embassy-time = "0.3"
esp-hal-embassy = { version = "0.4", features = ["esp32s3"] }
```

### Multiple Concurrent Tasks

```rust
#![no_std]
#![no_main]

use embassy_executor::Spawner;
use embassy_time::{Duration, Timer};
use esp_hal::{
    interrupt::software::SoftwareInterruptControl,
    timer::timg::TimerGroup,
};

// Define async tasks
#[embassy_executor::task]
async fn blink_task() {
    loop {
        esp_println::println!("LED blink");
        Timer::after(Duration::from_millis(500)).await;
    }
}

#[embassy_executor::task]
async fn sensor_task() {
    loop {
        esp_println::println!("Reading sensor...");
        Timer::after(Duration::from_millis(1000)).await;
    }
}

#[esp_hal_embassy::main]
async fn main(spawner: Spawner) {
    let peripherals = esp_hal::init(esp_hal::Config::default());

    // Initialize Embassy timer
    let sw_int = SoftwareInterruptControl::new(peripherals.SW_INTERRUPT);
    let timg0 = TimerGroup::new(peripherals.TIMG0);
    esp_hal_embassy::init(timg0.timer0, sw_int.software_interrupt0);

    // Spawn concurrent tasks
    spawner.spawn(blink_task()).ok();
    spawner.spawn(sensor_task()).ok();

    // Main task loop
    loop {
        esp_println::println!("Main task");
        Timer::after(Duration::from_millis(5_000)).await;
    }
}
```

### Async SPI with DMA

```rust
use embassy_time::{Duration, Timer};
use esp_hal::{
    dma::{DmaRxBuf, DmaTxBuf},
    dma_buffers,
    spi::master::{Config, Spi},
};

#[esp_hal_embassy::main]
async fn main(_spawner: Spawner) {
    let peripherals = esp_hal::init(esp_hal::Config::default());

    // Setup DMA and Embassy
    // ... (initialization code)

    // Allocate DMA buffers
    let (rx_buffer, rx_descriptors, tx_buffer, tx_descriptors) =
        dma_buffers!(32000);
    let dma_rx_buf = DmaRxBuf::new(rx_descriptors, rx_buffer).unwrap();
    let dma_tx_buf = DmaTxBuf::new(tx_descriptors, tx_buffer).unwrap();

    // Configure async SPI with DMA
    let mut spi = Spi::new(peripherals.SPI2, Config::default())
        .unwrap()
        .with_sck(peripherals.GPIO0)
        .with_mosi(peripherals.GPIO4)
        .with_miso(peripherals.GPIO2)
        .with_dma(dma_channel)
        .with_buffers(dma_rx_buf, dma_tx_buf)
        .into_async();

    let send_buffer = [0, 1, 2, 3, 4, 5, 6, 7];

    loop {
        let mut buffer = [0; 8];

        // Non-blocking async transfer
        embedded_hal_async::spi::SpiBus::transfer(
            &mut spi,
            &mut buffer,
            &send_buffer
        ).await.unwrap();

        Timer::after(Duration::from_millis(100)).await;
    }
}
```

## Comparison with Arduino/C++

### Code Size Comparison

| Approach          | Typical Binary Size | Flash Available   |
| ----------------- | ------------------- | ----------------- |
| Arduino C++       | 200-800 KB          | ~3.2 MB remaining |
| Rust bare-metal   | 100-500 KB          | ~3.5 MB remaining |
| Rust with ESP-IDF | 1-3 MB              | ~1-3 MB remaining |

### Safety Comparison

**Arduino/C++ Issues**:

```cpp
// Buffer overflow - compiles, crashes at runtime
char buffer[10];
strcpy(buffer, "This string is way too long");

// Use after free - compiles, undefined behavior
int* ptr = new int(42);
delete ptr;
*ptr = 100;  // Oops!

// Data race - compiles, intermittent bugs
int shared = 0;
// Thread 1: shared++;
// Thread 2: shared++;
```

**Rust Prevents These at Compile Time**:

```rust
// Won't compile: buffer too small
let buffer = [0u8; 10];
let data = b"This string is way too long";
buffer.copy_from_slice(data);  // COMPILE ERROR

// Won't compile: use after move
let value = Box::new(42);
drop(value);
let x = *value;  // COMPILE ERROR: value moved

// Won't compile: data race
let shared = 0;
thread::spawn(|| shared += 1);  // COMPILE ERROR: not thread-safe
thread::spawn(|| shared += 1);
```

### Development Experience

**Arduino Advantages**:

- Immediate familiarity for beginners
- Huge library ecosystem
- Simple IDE setup
- Quick prototyping

**Rust Advantages**:

- Catches bugs at compile time
- Excellent error messages guide you to fixes
- Cargo package manager handles dependencies automatically
- Code that compiles typically works correctly
- Refactoring is safe—compiler catches breaking changes

**Learning Curve**:

- Arduino: Gentle, 1-2 days to productivity
- Rust: Steeper, 1-2 weeks to comfort with ownership/borrowing
- Payoff: Rust prevents entire bug classes, saving debugging time

## Migration Strategy

### Phase 1: Proof of Concept (Week 1)

**Goal**: Verify Rust toolchain works with your hardware.

1. Create new Rust project with template
2. Implement LED blink (simplest possible test)
3. Test serial logging (`esp_println!`)
4. Verify upload/monitor workflow

**Success Criteria**: Can build, flash, and see serial output.

### Phase 2: Core Peripherals (Week 2-3)

**Goal**: Port critical hardware interfaces.

1. **Display**: Implement SPI communication for M5UnitOLED
   - Find or write driver crate for your display controller
   - Port initialization sequence from C++
   - Verify basic drawing operations

2. **Buttons**: Implement GPIO inputs with interrupts
   - Port button debouncing logic
   - Verify all button combinations work

3. **Audio**: Implement I2S or DAC for speaker
   - Port tone generation
   - Test simple audio playback

**Success Criteria**: All peripherals functional with basic operations.

### Phase 3: Advanced Features (Week 4-5)

**Goal**: Implement application logic.

1. **Battery Monitoring**: ADC reading and percentage calculation
2. **RTC**: Time keeping and alarm functionality
3. **IMU**: Sensor reading and data processing
4. **LED**: PWM for brightness/color control

**Success Criteria**: Feature parity with Arduino version.

### Phase 4: Async Migration (Week 6)

**Goal**: Convert to Embassy async for better concurrency.

1. Add Embassy dependencies
2. Convert blocking operations to async
3. Spawn tasks for concurrent operations
4. Optimize task scheduling

**Success Criteria**: Cleaner code, better responsiveness.

### Incremental Migration Strategy

You don't have to rewrite everything at once. Consider:

1. **Dual development**: Keep Arduino version working while building Rust version
2. **Module-by-module**: Port one peripheral at a time, test thoroughly
3. **Feature flags**: Use Cargo features to enable/disable experimental code
4. **Benchmarking**: Compare performance and binary size at each step

## Resources and Documentation

### Official Documentation

- **ESP-RS Book**: <https://docs.esp-rs.org/book>
  - Comprehensive guide to Rust on ESP32
  - Covers both bare-metal and ESP-IDF approaches

- **ESP-HAL Documentation**: <https://docs.esp-rs.org/esp-hal>
  - API reference for bare-metal development
  - Examples for every peripheral

- **ESP-IDF-HAL Documentation**: <https://docs.esp-rs.org/esp-idf-hal>
  - API reference for ESP-IDF bindings
  - Integration with FreeRTOS

### Context7 Resources

Use these library IDs with the Context7 MCP tool:

- **ESP HAL**: `/esp-rs/esp-hal` (201 snippets)
  - Bare-metal Hardware Abstraction Layer

- **ESP-HAL Documentation**: `/websites/espressif_projects_rust_esp-hal_1_0_0_esp32_esp-hal` (3,466 snippets)
  - Complete API docs with examples

- **ESP-IDF Guide**: `/websites/docs_espressif_com-projects-esp-idf-en-v5.4.1-esp32-get-started-index.html` (6,186 snippets)
  - ESP-IDF framework documentation

### Community Resources

- **ESP-RS GitHub**: <https://github.com/esp-rs>
  - Official Espressif Rust organization
  - Templates, HAL crates, tooling

- **Awesome ESP Rust**: <https://github.com/esp-rs/awesome-esp-rust>
  - Curated list of projects, tutorials, libraries

- **Matrix Chat**: <https://matrix.to/#/#esp-rs:matrix.org>
  - Active community support
  - Maintainers answer questions

### Tutorials and Examples

- **Getting Started Guide**: <https://nereux.blog/posts/getting-started-esp32-nostd>
  - Detailed walkthrough for ESP32 bare-metal

- **ESP32 Rust Training**: <https://github.com/esp-rs/std-training>
  - Official Espressif training materials
  - Hands-on exercises

- **Embassy on ESP**: <https://dev.to/theembeddedrustacean/embassy-on-esp-getting-started-27fi>
  - Async programming introduction
  - Real-world examples

### Example Projects

- **ESP32-C3 MQTT Logger**: <https://github.com/ImUrX/esp32c3-rust-std-temperature-logger>
  - Full project with WiFi and cloud connectivity

- **LCD Snake Game**: <https://jamesmcm.github.io/blog/beginner-rust-esp32-lcdsnake>
  - Display programming example
  - Game loop implementation

- **Bevy ECS on ESP32**: <https://developer.espressif.com/blog/2025/04/bevy-ecs-on-esp32-with-rust-no-std>
  - Advanced architecture patterns
  - Entity-component system

### Debugging and Development Tools

- **probe-rs**: <https://probe.rs>
  - Modern embedded debugger
  - VS Code integration

- **espflash**: <https://github.com/esp-rs/espflash>
  - Flashing tool documentation
  - Advanced usage examples

- **cargo-bloat**: <https://github.com/RazrFalcon/cargo-bloat>
  - Analyze binary size
  - Find optimization opportunities

### Performance and Optimization

- **Rust Embedded Book**: <https://docs.rust-embedded.org/book>
  - Best practices for embedded Rust
  - Performance optimization techniques

- **Binary Size Optimization**: <https://rustprojectprimer.com/building/size.html>
  - Comprehensive size reduction guide
  - Profile configuration examples

## Next Steps

1. **Install toolchain** following the "Getting Started" section
2. **Create proof-of-concept project** with LED blink
3. **Read ESP-RS Book** chapters on peripherals you'll use
4. **Join Matrix chat** for community support
5. **Port one peripheral at a time**, testing thoroughly
6. **Benchmark performance** against Arduino version
7. **Consider async migration** once basics work

Remember: The learning curve is real, but the benefits—memory safety, excellent tooling, and modern language features—make Rust a compelling choice for production embedded systems.
