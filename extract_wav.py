#!/usr/bin/env python3
"""
Extract WAV data from main.cpp and create a proper WAV file.

The embedded array is 8-bit unsigned PCM at 44.1kHz mono.
We need to create a proper WAV file with RIFF headers.
"""

import struct
import re

def create_wav_header(data_size, sample_rate=44100, bits_per_sample=8, channels=1):
    """
    Create a WAV file header.

    WAV format:
    - RIFF header (12 bytes)
    - fmt chunk (24 bytes for PCM)
    - data chunk (8 bytes + data)
    """
    # Calculate chunk sizes
    fmt_chunk_size = 16  # PCM format
    data_chunk_size = data_size
    riff_chunk_size = 4 + (8 + fmt_chunk_size) + (8 + data_chunk_size)

    # RIFF header
    header = b'RIFF'
    header += struct.pack('<I', riff_chunk_size)  # File size - 8
    header += b'WAVE'

    # fmt chunk
    header += b'fmt '
    header += struct.pack('<I', fmt_chunk_size)
    header += struct.pack('<H', 1)  # Audio format (1 = PCM)
    header += struct.pack('<H', channels)  # Number of channels
    header += struct.pack('<I', sample_rate)  # Sample rate
    header += struct.pack('<I', sample_rate * channels * bits_per_sample // 8)  # Byte rate
    header += struct.pack('<H', channels * bits_per_sample // 8)  # Block align
    header += struct.pack('<H', bits_per_sample)  # Bits per sample

    # data chunk
    header += b'data'
    header += struct.pack('<I', data_chunk_size)

    return header

def extract_wav_data(cpp_file):
    """Extract byte array from C++ file."""
    with open(cpp_file, 'r') as f:
        content = f.read()

    # Find the array data between { and }
    match = re.search(r'const uint8_t wav_8bit_44100\[46000\] = \{([^}]+)\}', content, re.DOTALL)
    if not match:
        raise ValueError("Could not find WAV array in file")

    array_text = match.group(1)

    # Extract hex values
    hex_values = re.findall(r'0x[0-9a-fA-F]{2}', array_text)

    # Convert to bytes
    data = bytes([int(h, 16) for h in hex_values])

    print(f"Extracted {len(data)} bytes from C++ array")
    return data

def main():
    # Extract data from main.cpp
    cpp_file = 'src/main.cpp'
    data = extract_wav_data(cpp_file)

    # Create WAV header
    header = create_wav_header(len(data), sample_rate=44100, bits_per_sample=8, channels=1)

    # Write WAV file
    output_file = 'data/audio/startup.wav'
    with open(output_file, 'wb') as f:
        f.write(header)
        f.write(data)

    print(f"Created {output_file} ({len(header) + len(data)} bytes total)")
    print(f"  Header: {len(header)} bytes")
    print(f"  Data: {len(data)} bytes")
    print(f"  Format: 8-bit PCM, 44.1kHz, Mono")
    print(f"  Duration: {len(data) / 44100:.2f} seconds")

if __name__ == '__main__':
    main()
