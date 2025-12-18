# 2.1 SPI Basics

## Overview

SPI stands for Serial Peripheral Interface, which is a wonderfully literal name for a thing that interfaces with peripherals, serially. It's how your ESP32 will communicate with the SD card, and understanding it will save you hours of confusion when things don't work.

Which they won't. At first.

## What You'll Learn

- How SPI works at a conceptual level
- The four SPI signals and what they do
- SPI modes and why they matter
- How this applies to SD cards specifically

## Prerequisites

- Completed Phase 1 (working PlatformIO environment)
- A basic grasp of what "digital signals" means (high voltage = 1, low voltage = 0)
- Willingness to learn about a protocol designed in the 1980s that somehow still works

---

## Step 1: What Is SPI?

SPI is a synchronous, full-duplex communication protocol. Let's decode that:

- **Synchronous** — Data is sent in time with a clock signal. No guessing when bits arrive.
- **Full-duplex** — Data can flow in both directions simultaneously (though often you only care about one direction at a time).

It was developed by Motorola in the mid-1980s, which explains why it's both elegant and slightly obtuse.

### The Bus Architecture

SPI uses a master-slave architecture:

```
           ┌──────────────────┐
           │     MASTER       │
           │    (ESP32-S3)    │
           └─────┬──┬──┬──┬───┘
                 │  │  │  │
    MOSI ────────┼──┼──┼──┼────────────────────────────────┐
    MISO ────────┼──┼──┼──┼─────────────────────────┐      │
    SCK  ────────┼──┼──┼──┼──────────────────┐      │      │
    CS0  ────────┘  │  │  │                  │      │      │
    CS1  ───────────┘  │  │                  │      │      │
    CS2  ──────────────┘  │                  │      │      │
    CS3  ─────────────────┘                  │      │      │
                                             │      │      │
           ┌──────────────────┐              │      │      │
           │     SLAVE 0      │              │      │      │
           │    (SD Card)     │◄─────────────┼──────┼──────┤
           └──────────────────┘              │      │      │
                                             │      │      │
           ┌──────────────────┐              │      │      │
           │     SLAVE 1      │              │      │      │
           │   (Display)      │◄─────────────┼──────┼──────┤
           └──────────────────┘              │      │      │
                                             │      │      │
           etc...                            ▼      ▼      ▼
```

The master (your ESP32) controls everything. Slaves (SD card, displays, sensors) only respond when spoken to.

## Step 2: The Four SPI Signals

| Signal   | Full Name            | Direction      | Purpose                                   |
| -------- | -------------------- | -------------- | ----------------------------------------- |
| **MOSI** | Master Out, Slave In | Master → Slave | Data from ESP32 to peripheral             |
| **MISO** | Master In, Slave Out | Slave → Master | Data from peripheral to ESP32             |
| **SCK**  | Serial Clock         | Master → Slave | Clock signal that synchronises everything |
| **CS**   | Chip Select          | Master → Slave | Selects which slave is being addressed    |

### MOSI — Master Out, Slave In

This is the data line from master to slave. When you write to an SD card, the data travels on this line.

### MISO — Master In, Slave Out

This is the data line from slave to master. When you read from an SD card, the data comes back on this line.

### SCK — Serial Clock

The clock line pulses high and low at a regular frequency. Data is read/written in time with these pulses. The master controls the clock, so the master controls the pace of communication.

Typical speeds:

- SD cards in SPI mode: 1-25 MHz
- Fast SPI devices: Up to 80 MHz
- The ESP32-S3's SPI peripheral can do up to 80 MHz

### CS — Chip Select

Also called SS (Slave Select) or NSS or CSN depending on who you ask.

This line is normally HIGH. When the master pulls it LOW, it activates that specific slave device. This allows multiple devices to share the same MOSI, MISO, and SCK lines — only the device with CS low will respond.

When CS is high, the slave ignores everything. It's like being on mute in a conference call, except it actually works.

## Step 3: How Data Moves

A single SPI byte transfer looks like this:

```
CS   ────┐                                              ┌────
         └──────────────────────────────────────────────┘

SCK  ────┐  ┌──┐  ┌──┐  ┌──┐  ┌──┐  ┌──┐  ┌──┐  ┌──┐  ┌────
         └──┘  └──┘  └──┘  └──┘  └──┘  └──┘  └──┘  └──┘

MOSI ────< b7 >< b6 >< b5 >< b4 >< b3 >< b2 >< b1 >< b0 >────
              (Data from master to slave)

MISO ────< b7 >< b6 >< b5 >< b4 >< b3 >< b2 >< b1 >< b0 >────
              (Data from slave to master, simultaneously)
```

1. Master pulls CS low to select the slave
2. Master generates 8 clock pulses
3. On each clock edge, one bit moves on MOSI (master→slave) and one bit moves on MISO (slave→master)
4. After 8 bits, one byte has been exchanged in each direction
5. Master pulls CS high to deselect the slave

The key insight: **SPI always exchanges data**. Even if you only want to read, you must send something (typically 0x00 or 0xFF). Even if you only want to write, you'll receive something back (typically ignored).

## Step 4: SPI Modes

Data can be sampled on the rising edge or falling edge of the clock, and the clock can idle high or low. This gives four possible modes:

| Mode | CPOL | CPHA | Clock Idle | Data Sampled On |
| ---- | ---- | ---- | ---------- | --------------- |
| 0    | 0    | 0    | Low        | Rising edge     |
| 1    | 0    | 1    | Low        | Falling edge    |
| 2    | 1    | 0    | High       | Falling edge    |
| 3    | 1    | 1    | High       | Rising edge     |

- **CPOL** (Clock Polarity): What level the clock idles at when not transmitting
- **CPHA** (Clock Phase): Whether data is sampled on the first or second edge

**SD cards use Mode 0** (CPOL=0, CPHA=0). The clock idles low, and data is sampled on the rising edge.

You will likely never need to change this, but when something doesn't work, checking the SPI mode is step 3 (after "is it plugged in?" and "is it powered on?").

## Step 5: SD Cards and SPI

SD cards actually support two communication modes:

1. **SD/SDIO mode** — 4-bit parallel, faster, more complex
2. **SPI mode** — Serial, slower, simpler, what we're using

SPI mode was originally included for compatibility with microcontrollers that couldn't implement the full SD protocol. It's still widely used because it's straightforward and "good enough" for most embedded applications.

### SD Card Pinout in SPI Mode

| SD Pin | SPI Function         | Connect To         |
| ------ | -------------------- | ------------------ |
| 1      | CS                   | GPIO (your choice) |
| 2      | MOSI                 | SPI MOSI pin       |
| 3      | GND                  | Ground             |
| 4      | VCC                  | 3.3V               |
| 5      | SCK                  | SPI SCK pin        |
| 6      | GND                  | Ground             |
| 7      | MISO                 | SPI MISO pin       |
| 8-9    | Not used in SPI mode | Leave unconnected  |

The M5 Capsule has an SD card slot built in, so this wiring is already done for you. You just need to know which GPIO pins are connected to which SPI signals. According to the M5 documentation, you'll need to check their pinout diagram — this varies by hardware revision.

### SPI Speed Considerations

SD cards have speed classes, but in SPI mode, the protocol overhead means you'll never hit the card's rated speed. Realistic expectations:

| Card Speed Class | Marketing | Actual SPI Throughput |
| ---------------- | --------- | --------------------- |
| Class 10         | 10 MB/s   | 1-2 MB/s              |
| UHS-I            | 104 MB/s  | 2-4 MB/s              |
| Doesn't matter   | —         | Still limited by SPI  |

For audio at 16kHz, 16-bit mono, you need 32 KB/s. Even slow SD cards can handle this with room to spare. The bottleneck will be your code, not the card.

---

## Step 6: ESP32-S3 SPI Peripherals

The ESP32-S3 has multiple SPI controllers:

| Peripheral | Name      | Notes                                   |
| ---------- | --------- | --------------------------------------- |
| SPI0       | —         | Reserved for flash memory (don't touch) |
| SPI1       | —         | Also for flash (still don't touch)      |
| SPI2       | FSPI/HSPI | Available for your use                  |
| SPI3       | VSPI      | Also available for your use             |

In Arduino-land, these are typically accessed as `HSPI` and `VSPI`, or simply by letting the library choose sensible defaults.

The M5 Capsule's SD card is likely connected to one of these. When we initialise the SD library in the next step, we'll specify which pins to use, and the library will configure the appropriate SPI peripheral.

---

## Verification

This step is conceptual — there's no code to verify yet. But you should now understand:

- [ ] SPI is a synchronous, clocked protocol
- [ ] Four signals: MOSI, MISO, SCK, CS
- [ ] Data is exchanged one byte at a time, in both directions simultaneously
- [ ] SD cards use SPI Mode 0 (CPOL=0, CPHA=0)
- [ ] The ESP32-S3 has multiple SPI peripherals available

If any of this is unclear, it will become clearer once we write actual code. Sometimes you need to see a thing work before you understand why it works.

---

## What's Next

Theory is lovely, but it doesn't write audio files. In [Step 2.2: SD Card Library](./05-sd-card-library.md), we'll use this knowledge to actually mount an SD card and write files to it. The satisfaction of seeing a file appear on a card you can then plug into your computer is... well, it's something.

---

## Quick Reference

```
SPI Signals:
  MOSI  →  Master Out, Slave In (data to device)
  MISO  ←  Master In, Slave Out (data from device)
  SCK   →  Clock (timing reference)
  CS    →  Chip Select (activates device when LOW)

SPI Mode 0 (used by SD cards):
  - Clock idles LOW
  - Data sampled on RISING edge

ESP32-S3 SPI:
  - SPI0/SPI1: Reserved for flash
  - SPI2 (HSPI): Available
  - SPI3 (VSPI): Available
```
