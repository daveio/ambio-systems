# 1.1 PlatformIO Environment Setup

## Overview

Before you can write a single line of code that will inevitably not work on the first attempt, you need somewhere to write it. PlatformIO is that somewhere — an ecosystem for embedded development that wraps around the chaos of toolchains, compilers, and board definitions so you don't have to think about them until something goes wrong.

Think of it as an abstraction layer between you and suffering.

## What You'll Learn

- Installing PlatformIO in VS Code
- Understanding project structure
- Configuring `platformio.ini` for the M5 Capsule
- The build/upload workflow

## Prerequisites

- VS Code installed (or another editor, but let's be honest, it's VS Code)
- A USB cable that actually carries data (not just charging — this distinction has caused more wasted hours than any bug ever written)
- The M5 Capsule, ideally not still in its packaging

---

## Step 1: Install the PlatformIO Extension

1. Open VS Code
2. Navigate to Extensions (Ctrl+Shift+X / Cmd+Shift+X)
3. Search for "PlatformIO IDE"
4. Click Install
5. Wait while it downloads approximately four hundred dependencies
6. Restart VS Code when prompted

The first launch will take longer than seems reasonable. This is normal. PlatformIO is installing compilers, frameworks, and what I can only assume is a small operating system. Go make tea.

## Step 2: Create Your First Project

Once PlatformIO has finished its extensive setup ritual:

1. Click the PlatformIO icon in the sidebar (it looks like an alien or a house, depending on your state of mind)
2. Click "Create New Project"
3. Fill in the wizard:
   - **Name:** `pendant-audio-capture` (or whatever speaks to you)
   - **Board:** Search for `m5stack-stamps3` — this is the ESP32-S3 module inside your Capsule
   - **Framework:** Select `Arduino` (we'll discuss this choice below)
   - **Location:** Wherever you keep projects you intend to finish

Click "Finish" and wait for another round of downloads.

### Why Arduino Framework?

You have two choices for ESP32-S3 development:

| Framework   | Pros                                                                   | Cons                                                                      |
| ----------- | ---------------------------------------------------------------------- | ------------------------------------------------------------------------- |
| **Arduino** | Familiar syntax, vast library ecosystem, abstracts hardware complexity | Hides what's actually happening, less control                             |
| **ESP-IDF** | Full control, better performance, direct hardware access               | Steeper learning curve, more verbose, you will question your life choices |

We're using Arduino because this is a learning pathway, not a punishment. The Arduino framework for ESP32 is actually ESP-IDF under the hood anyway, so you can drop down to the lower level when needed. Which you will. In Phase 3. I'm sorry.

## Step 3: Understand the Project Structure

Your new project contains:

```
pendant-audio-capture/
├── .pio/                  # Build artifacts (ignore this)
├── .vscode/               # VS Code settings (PlatformIO manages this)
├── include/               # Header files (.h)
├── lib/                   # Project-specific libraries
├── src/                   # Your source code lives here
│   └── main.cpp           # The main file — this is where you'll work
├── test/                  # Unit tests (aspirational)
└── platformio.ini         # The configuration file (important)
```

The only files you'll touch regularly are:

- `src/main.cpp` — your code
- `platformio.ini` — your configuration
- `lib/` — when you inevitably write reusable components

## Step 4: Configure platformio.ini

Open `platformio.ini`. It should look something like this:

```ini
[env:m5stack-stamps3]
platform = espressif32
board = m5stack-stamps3
framework = arduino
```

This is the minimum viable configuration. Let's make it actually useful:

```ini
[env:m5stack-stamps3]
platform = espressif32
board = m5stack-stamps3
framework = arduino

; Serial monitor settings
monitor_speed = 115200
monitor_filters = esp32_exception_decoder

; Upload settings
upload_speed = 921600

; Build flags
build_flags =
    -DARDUINO_USB_MODE=1
    -DARDUINO_USB_CDC_ON_BOOT=1

; Partition scheme (we'll need space for audio)
board_build.partitions = default_16MB.csv
```

### What These Options Mean

| Option                    | Purpose                                                                    |
| ------------------------- | -------------------------------------------------------------------------- |
| `monitor_speed`           | Baud rate for serial communication. 115200 is standard.                    |
| `monitor_filters`         | Decodes ESP32 crash dumps into human-readable stack traces. Essential.     |
| `upload_speed`            | How fast to flash the device. Higher = faster, but some cables can't cope. |
| `ARDUINO_USB_MODE`        | Enables USB-OTG mode (the S3 has native USB)                               |
| `ARDUINO_USB_CDC_ON_BOOT` | Serial output goes via USB, not UART. You want this.                       |
| `board_build.partitions`  | Flash memory layout. The Capsule has 16MB; we should use it.               |

## Step 5: The Build/Upload Workflow

With your project configured, here's how you'll spend most of your time:

### Building (Compiling)

**VS Code:** Click the checkmark icon in the bottom toolbar, or press Ctrl+Alt+B

**Terminal:**

```fish
pio run
```

This compiles your code but doesn't upload it. Useful for checking syntax without connecting the device.

### Uploading

**VS Code:** Click the arrow icon in the bottom toolbar, or press Ctrl+Alt+U

**Terminal:**

```fish
pio run -t upload
```

This compiles and uploads to the connected device. The M5 Capsule should enter bootloader mode automatically when you click upload — if it doesn't, hold the boot button while pressing reset.

### Serial Monitor

**VS Code:** Click the plug icon in the bottom toolbar

**Terminal:**

```fish
pio device monitor
```

This shows output from `Serial.print()` statements. Your primary debugging tool. Press Ctrl+C to exit.

### The Holy Trinity Command

When things are going well:

```fish
pio run -t upload && pio device monitor
```

This uploads and immediately opens the monitor. You'll type this approximately four thousand times.

---

## Verification

Before moving on, confirm:

- [ ] PlatformIO extension is installed and functional
- [ ] You can create a new project targeting `m5stack-stamps3`
- [ ] The project builds without errors (even though `main.cpp` is essentially empty)
- [ ] You understand what each section of `platformio.ini` does

If the project doesn't build, check that:

1. PlatformIO finished its initial setup
2. You have an internet connection (it downloads toolchains on first build)
3. Your `platformio.ini` has no syntax errors (YAML-adjacent formats are unforgiving)

---

## Common Issues

### "Board not found"

PlatformIO needs to download board definitions. Run:

```fish
pio platform install espressif32
```

### "Permission denied" on upload (Linux)

Your user isn't in the `dialout` group:

```fish
sudo usermod -a -G dialout \$USER
```

Log out and back in for this to take effect.

### "Failed to connect" on upload

The ESP32-S3 might not be entering bootloader mode. Hold the BOOT button (the small one) while pressing the RST button (the other small one), then release both. Try uploading again within a few seconds.

---

## What's Next

With your environment configured, you're ready to write code that actually does something. In [Step 1.2: Your First ESP32-S3 Project](./02-first-esp32-project.md), we'll blink an LED — the embedded equivalent of printing "Hello, World!", but with more immediate visual feedback and somehow less satisfaction.

---

## Quick Reference

| Action       | VS Code         | Terminal                      |
| ------------ | --------------- | ----------------------------- |
| Build        | Ctrl+Alt+B      | `pio run`                     |
| Upload       | Ctrl+Alt+U      | `pio run -t upload`           |
| Monitor      | Click plug icon | `pio device monitor`          |
| Clean        | -               | `pio run -t clean`            |
| Full rebuild | -               | `pio run -t clean && pio run` |
