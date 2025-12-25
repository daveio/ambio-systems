#ifndef AMBIO_TYPES_H
#define AMBIO_TYPES_H

#include <Arduino.h>
#include <M5Unified.h>

// ============================================================================
// Common Constants and Types for Ambio Pendant Firmware
// ============================================================================

// ----------------------------------------------------------------------------
// Display Constants
// ----------------------------------------------------------------------------

// Text sizes (M5GFX text datum constants)
constexpr uint8_t DEFAULT_TEXT_SIZE = 2;
constexpr uint8_t SMALL_TEXT_SIZE = 1;
constexpr uint8_t LARGE_TEXT_SIZE = 3;

// Display brightness levels (0-255)
constexpr uint8_t DISPLAY_BRIGHTNESS_LOW = 32;
constexpr uint8_t DISPLAY_BRIGHTNESS_MEDIUM = 128;
constexpr uint8_t DISPLAY_BRIGHTNESS_HIGH = 255;

// Display update regions (for partial updates)
constexpr int DISPLAY_HEADER_HEIGHT = 16;
constexpr int DISPLAY_FOOTER_HEIGHT = 16;

// ----------------------------------------------------------------------------
// Button State Constants
// ----------------------------------------------------------------------------

// Button state colors (TFT color codes)
constexpr int BUTTON_STATE_COLORS[] = {
    TFT_WHITE,   // 0: none
    TFT_CYAN,    // 1: wasHold
    TFT_RED,     // 2: wasClicked
    TFT_YELLOW,  // 3: wasPressed
    TFT_BLUE,    // 4: wasReleased
    TFT_GREEN    // 5: wasDecideClickCount
};

// Button state names
constexpr const char* BUTTON_STATE_NAMES[] = {
    "none",
    "wasHold",
    "wasClicked",
    "wasPressed",
    "wasReleased",
    "wasDecideClickCount"
};

// Number of button states
constexpr size_t BUTTON_STATE_COUNT = 6;

// Button tone frequencies (Hz)
constexpr uint16_t BUTTON_TONE_FREQ_LOW = 783;     // G5
constexpr uint16_t BUTTON_TONE_FREQ_MED = 1000;    // B5 (approx)
constexpr uint16_t BUTTON_TONE_FREQ_HIGH = 2000;   // B6 (approx)
constexpr uint32_t BUTTON_TONE_DURATION_MS = 100;

// ----------------------------------------------------------------------------
// Sensor Constants
// ----------------------------------------------------------------------------

// Battery level constants
constexpr int BATTERY_MIN_LEVEL = 0;
constexpr int BATTERY_MAX_LEVEL = 100;
constexpr int BATTERY_INVALID = INT_MAX;

// IMU graph constants
constexpr size_t IMU_GRAPH_CHANNELS = 6;  // 3 accel + 3 gyro
constexpr int IMU_GRAPH_HEIGHT = 32;
constexpr int IMU_GRAPH_WIDTH = 128;

// RTC weekday names
constexpr const char* WEEKDAY_NAMES[] = {
    "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"
};

// ----------------------------------------------------------------------------
// Audio Constants
// ----------------------------------------------------------------------------

// Volume levels (0-255)
constexpr uint8_t AUDIO_VOLUME_MIN = 0;
constexpr uint8_t AUDIO_VOLUME_DEFAULT = 64;
constexpr uint8_t AUDIO_VOLUME_MAX = 255;

// Test tone frequencies
constexpr uint16_t AUDIO_TEST_TONE_LOW = 783;
constexpr uint16_t AUDIO_TEST_TONE_MED = 1000;
constexpr uint16_t AUDIO_TEST_TONE_HIGH = 2000;

// WAV playback constants
constexpr uint32_t AUDIO_WAV_SAMPLE_RATE = 44100;
constexpr const char* AUDIO_STARTUP_WAV_PATH = "/audio/startup.wav";

// ----------------------------------------------------------------------------
// Frame Rate Control
// ----------------------------------------------------------------------------

// Loop timing
constexpr uint32_t LOOP_DELAY_MS = 0;  // Run as fast as possible
constexpr uint32_t SENSOR_UPDATE_INTERVAL_MS = 100;  // Update sensors 10x/sec

// ----------------------------------------------------------------------------
// Hardware Configuration
// ----------------------------------------------------------------------------

// Serial communication
constexpr uint32_t SERIAL_BAUDRATE = 115200;

// LED brightness (0-255, not NeoPixel)
constexpr uint8_t LED_BRIGHTNESS_DEFAULT = 64;
constexpr uint8_t LED_BRIGHTNESS_OFF = 0;
constexpr uint8_t LED_BRIGHTNESS_MAX = 255;

// ----------------------------------------------------------------------------
// Type Aliases
// ----------------------------------------------------------------------------

// Button state type
enum class ButtonState : uint8_t {
    None = 0,
    Hold = 1,
    Clicked = 2,
    Pressed = 3,
    Released = 4,
    DecideClickCount = 5
};

// Display region for partial updates
struct DisplayRegion {
    int x;
    int y;
    int width;
    int height;
};

// Sensor reading structure
struct SensorReadings {
    int battery_level;
    bool rtc_available;
    bool imu_available;
    float accel_x, accel_y, accel_z;
    float gyro_x, gyro_y, gyro_z;
};

#endif // AMBIO_TYPES_H
