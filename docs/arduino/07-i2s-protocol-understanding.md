# 3.1 I2S Protocol Understanding

## Overview

Welcome to Phase 3, where we stop writing polite text files and start shouting at hardware. I2S (Inter-IC Sound, pronounced "eye-squared-ess" or "eye-two-ess" depending on how much you want to annoy people) is the protocol that moves digital audio between chips.

If SPI is a conversation, I2S is a performance. The data must arrive on time, every time, at precisely the right rate. The microphone doesn't care if you're busy doing something else. The samples keep coming.

## What You'll Learn

- What I2S is and why it exists
- The three-wire protocol structure
- Sample rates, bit depths, and channels
- How the M5 Capsule's microphone fits into this

## Prerequisites

- Completed Phase 2 (SD card operations working)
- A willingness to think about timing at the microsecond level
- Coffee, or your stimulant of choice

---

## Step 1: Why I2S Exists

In the 1980s, Philips (now NXP) needed a standard way to connect digital audio devices. They created I2S — a simple, synchronous protocol for moving audio samples between chips.

Before I2S, every manufacturer had their own interface. This was inconvenient for everyone except the manufacturers, who presumably enjoyed the chaos.

I2S solved this by defining:

- How bits are arranged (MSB first)
- When to sample them (clock edges)
- How to tell left from right channels (word select signal)

It's been the standard for digital audio interconnects ever since.

## Step 2: The Three Signals

I2S uses three wires (plus ground):

| Signal  | Name                        | Purpose                                 |
| ------- | --------------------------- | --------------------------------------- |
| **SCK** | Serial Clock (also BCLK)    | Bit clock — one pulse per bit           |
| **WS**  | Word Select (also LRCLK)    | Indicates left (0) or right (1) channel |
| **SD**  | Serial Data (also DIN/DOUT) | The actual audio data                   |

### SCK — Bit Clock

The bit clock runs continuously at a frequency determined by:

```
SCK = Sample Rate × Bits per Sample × Number of Channels
```

For 16kHz, 16-bit stereo:

```
SCK = 16,000 × 16 × 2 = 512,000 Hz (512 kHz)
```

For 44.1kHz, 16-bit stereo:

```
SCK = 44,100 × 16 × 2 = 1,411,200 Hz (1.41 MHz)
```

Each clock pulse moves one bit.

### WS — Word Select

The word select signal toggles to indicate which channel is being transmitted:

- WS = 0 → Left channel
- WS = 1 → Right channel

It changes state one bit clock before the first bit of each sample, giving the receiver time to prepare.

### SD — Serial Data

The audio samples themselves, sent MSB (most significant bit) first. Each sample is a signed integer representing the amplitude at that moment.

## Step 3: I2S Timing Diagram

Here's what a stereo 16-bit transfer looks like:

```
         ┌─┐ ┌─┐ ┌─┐ ┌─┐ ┌─┐ ┌─┐ ┌─┐ ┌─┐ ┌─┐ ┌─┐ ┌─┐ ┌─┐ ┌─┐ ┌─┐ ┌─┐ ┌─┐ ┌─┐
SCK    ──┘ └─┘ └─┘ └─┘ └─┘ └─┘ └─┘ └─┘ └─┘ └─┘ └─┘ └─┘ └─┘ └─┘ └─┘ └─┘ └─┘ └──

                           ┌────────────────────────────────┐
WS     ────────────────────┘                                └────────────────
               LEFT                                                  RIGHT

SD     ─── L15 L14 L13 L12 L11 L10 L09 L08 L07 L06 L05 L04 L03 L02 L01 L00 ───
           MSB ◄─────────────────────────────────────────────────────── LSB
```

The left sample (L15-L00) is transmitted when WS is low, the right sample when WS is high. This repeats 16,000 times per second (at 16kHz) or 44,100 times per second (at 44.1kHz).

## Step 4: Sample Rates and Bit Depths

### Sample Rate

How many times per second the audio is measured:

| Sample Rate | Quality           | Common Use                           |
| ----------- | ----------------- | ------------------------------------ |
| 8 kHz       | Telephone         | Voice-only, legacy                   |
| 16 kHz      | HD Voice          | Voice assistants, speech recognition |
| 22.05 kHz   | Half CD           | Some podcasts                        |
| 44.1 kHz    | CD Quality        | Music, professional audio            |
| 48 kHz      | DVD/Digital Video | Film, broadcast                      |
| 96 kHz      | High-res          | Audiophile mastering                 |

For a pendant recording speech, **16 kHz is ideal**. It captures all voice frequencies (up to 8 kHz per Nyquist) while keeping file sizes manageable.

### Bit Depth

How many bits represent each sample:

| Bit Depth | Dynamic Range | Common Use                    |
| --------- | ------------- | ----------------------------- |
| 8-bit     | ~48 dB        | Ancient, avoid                |
| 16-bit    | ~96 dB        | CD, most applications         |
| 24-bit    | ~144 dB       | Professional recording        |
| 32-bit    | ~192 dB       | Internal processing, overkill |

**16-bit is standard** for most purposes. 24-bit gives more headroom but doubles file sizes compared to 16-bit.

### Channels

- **Mono:** One channel. The pendant mic is mono.
- **Stereo:** Two channels (left, right). The desktop mic array might be stereo.

For the pendant: **16 kHz, 16-bit, mono** gives us:

- 32,000 bytes per second
- 1.92 MB per minute
- 115.2 MB per hour

Very manageable.

## Step 5: I2S Modes and Variants

Standard I2S (Philips format) has the WS transition one bit before the MSB. But variations exist:

| Format              | WS Alignment                | Common In              |
| ------------------- | --------------------------- | ---------------------- |
| **I2S (Philips)**   | WS changes 1 bit before MSB | Most devices, standard |
| **Left Justified**  | WS changes at MSB           | Some Japanese devices  |
| **Right Justified** | LSB aligns with WS change   | Older DACs             |
| **PCM/DSP**         | Various                     | TDM, multi-channel     |

The ESP32-S3 supports all of these. The microphone documentation will specify which format it uses — usually standard I2S.

## Step 6: PDM vs I2S Microphones

Microphones come in two digital flavours:

### I2S Microphones

Output standard I2S audio directly. What you receive is PCM audio ready to use.

- Pros: Simple, direct PCM output
- Cons: Larger, more expensive

### PDM Microphones (Pulse Density Modulation)

Output a 1-bit stream at a very high rate (typically 1-3 MHz). The density of 1s represents the amplitude.

```
Loud:  1111101111110111111011111101111...  (lots of 1s)
Quiet: 1000010001000100001000010001000...  (few 1s)
```

- Pros: Smaller, cheaper, single data line
- Cons: Requires decimation filter to convert to PCM

The M5 Capsule likely uses a **PDM microphone**. The ESP32-S3 has built-in PDM support that handles the conversion automatically — you configure it as PDM input, and the driver outputs PCM samples.

## Step 7: The ESP32-S3 I2S Peripheral

The ESP32-S3 has two I2S controllers (I2S0 and I2S1), each capable of:

- Transmitting (TX) — sending audio to DACs, amplifiers
- Receiving (RX) — receiving audio from microphones
- Both simultaneously (full duplex)

Key features:

- DMA (Direct Memory Access) — audio transfers happen without CPU intervention
- Configurable word length: 8, 16, 24, or 32 bits
- Sample rates from 8 kHz to 96 kHz (and beyond with caveats)
- PDM mode support

### DMA: The Secret to Not Dropping Samples

At 16 kHz, a new sample arrives every 62.5 microseconds. If your code takes 100 microseconds to process a sample, you've lost the next one.

DMA solves this by filling a buffer in the background. While the CPU processes one buffer, DMA fills another. This is called **double buffering** or **ping-pong buffering**.

```
Time:    ─────────────────────────────────────────────────────────►

DMA:     [Fill Buffer A] [Fill Buffer B] [Fill Buffer A] [Fill Buff...
                    ↓                ↓                ↓
CPU:             [Process A]   [Process B]   [Process A]   [Pro...
```

The CPU gets entire buffers of samples to process at once, and as long as it finishes before the DMA fills the next buffer, no samples are lost.

## Step 8: M5 Capsule Microphone Details

According to the M5 documentation, the Capsule uses the internal StampS3 module with a PDM microphone. You'll need to identify:

1. **Data pin** — Where the PDM data comes in (often a GPIO)
2. **Clock pin** — The PDM clock (often generated by the ESP32)
3. **Microphone model** — To understand sensitivity, frequency response

Check the [M5 Capsule schematic](https://docs.m5stack.com/en/core/M5Capsule) for exact pin assignments.

Typical configuration might be:

- PDM_DATA: GPIO X
- PDM_CLK: GPIO Y

The ESP32 generates the clock signal to drive the microphone, and the microphone sends data back synchronised to that clock.

---

## Verification

This step is conceptual understanding. You should now be able to:

- [ ] Explain what the three I2S signals do
- [ ] Calculate the bit clock frequency for a given sample rate
- [ ] Understand the difference between PDM and PCM
- [ ] Know why DMA is essential for audio

If you're feeling confident, look up the M5 Capsule pinout and identify which GPIO pins connect to the microphone. You'll need these in the next step.

---

## What's Next

Theory complete. In [Step 3.2: ESP-IDF I2S Driver](./08-esp-idf-i2s-driver.md), we configure the ESP32-S3's I2S peripheral to actually capture audio from the microphone. This involves ESP-IDF APIs (wrapped by Arduino), configuration structures, and the satisfaction of seeing real audio samples appear in your serial monitor.

Or the frustration of seeing garbage. One of those.

---

## Quick Reference

```
I2S Signals:
  SCK  →  Bit clock (one pulse per bit)
  WS   →  Word select (left/right channel)
  SD   →  Serial data (the audio)

Common Configurations:
  Voice recording:  16 kHz, 16-bit, mono  →  32 KB/s
  Music quality:    44.1 kHz, 16-bit, stereo  →  176 KB/s

Bit Clock Formula:
  SCK = Sample Rate × Bit Depth × Channels

Microphone Types:
  I2S:  Direct PCM output, simpler to use
  PDM:  1-bit stream, requires decimation (ESP32 handles this)

ESP32-S3 I2S:
  - Two controllers: I2S0, I2S1
  - DMA support for buffer-based transfer
  - PDM input support
  - Configurable word length and sample rate
```
