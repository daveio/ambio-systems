#include <Arduino.h>

// ---- Display modules (optional) ----
// Enable the display type you're using
#include <M5UnitOLED.h>
// #include <M5AtomDisplay.h>
// #include <M5UnitGLASS.h>
// #include <M5UnitLCD.h>

// ---- M5Unified core ----
#include <M5Unified.h>

// ---- Application modules ----
#include "hardware.h"
#include "display.h"
#include "buttons.h"
#include "audio.h"
#include "sensors.h"

// ============================================================================
// Application Lifecycle
// ============================================================================

/**
 * Setup - Initialize all hardware and modules
 *
 * Initialization order is critical:
 * 1. Hardware (M5Unified)
 * 2. Display
 * 3. Buttons
 * 4. Audio
 * 5. Sensors
 *
 * This replaces the original 3,564-line monolithic setup().
 */
void setup(void) {
    // Initialize hardware and detect board/IMU
    hardware_init();

    // Initialize display system
    display_init();

    // Initialize button handlers
    buttons_init();

    // Initialize audio and play startup sound
    audio_init();

    // Initialize sensor subsystems
    sensors_init();

    // Display board information
    display_print_board_info(get_board_name(), get_imu_name());

    // Configure touchscreen button area (bottom 32 pixels)
    M5.setTouchButtonHeight(32);
    // Alternate: use ratio (25 = 10% of screen height)
    // M5.setTouchButtonHeightByRatio(25);

    M5_LOGI("Setup complete - starting main loop");
}

/**
 * Loop - Main application update cycle
 *
 * Polls all peripherals and updates displays.
 * This replaces the original loop() function.
 *
 * Note: Each subsystem manages its own display transactions (startWrite/endWrite)
 * to avoid nested transaction issues. The final display() call flushes the buffer.
 */
void loop(void) {
    M5.delay(1);  // Small delay for task scheduling

    // Update M5 internal state (buttons, power, touch, etc.)
    M5.update();

    // Update all subsystems - each handles its own display transactions
    buttons_update();   // Button events, LED, audio feedback
    sensors_update();   // Battery, RTC, IMU (includes display() call)
}

/**
 * ESP-IDF compatibility wrapper
 *
 * ESP-IDF uses app_main() instead of Arduino's setup()/loop().
 * This wrapper allows building with both frameworks.
 */
#if !defined(ARDUINO) && defined(ESP_PLATFORM)
extern "C" {
  void app_main() {
    setup();
    while (true) {
      loop();
    }
  }
}
#endif
