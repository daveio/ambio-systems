#ifndef AMBIO_DISPLAY_H
#define AMBIO_DISPLAY_H

#include <M5Unified.h>
#include "types.h"

// ============================================================================
// Display Management Module
// ============================================================================

/**
 * Initialize display subsystem
 *
 * Performs:
 * - EPD mode configuration (for e-paper displays)
 * - Brightness setup (0-255)
 * - Rotation adjustment (landscape mode)
 * - Multi-display initialization test
 * - Text size configuration based on display height
 *
 * Must be called after hardware_init()
 */
void display_init();

/**
 * Set display brightness level
 *
 * @param level Brightness (0=off, 255=max)
 *
 * Note: Only affects LCD displays, not OLED/EPD
 */
void display_set_brightness(uint8_t level);

/**
 * Clear the entire display to black
 */
void display_clear();

/**
 * Print board and IMU information to display
 *
 * @param board_name Board type string (e.g., "StampS3")
 * @param imu_name IMU type string (e.g., "MPU6886")
 *
 * Displays:
 * - "Core: <board_name>"
 * - "IMU: <imu_name>"
 */
void display_print_board_info(const char* board_name, const char* imu_name);

/**
 * Begin a display frame (start batched write)
 *
 * Use this before multiple display operations to improve performance.
 * Must be paired with display_end_frame()
 */
void display_begin_frame();

/**
 * End a display frame (finish batched write)
 *
 * Flushes all pending display operations to the screen.
 * Must be called after display_begin_frame()
 */
void display_end_frame();

/**
 * Get display width in pixels
 *
 * @return Width in pixels
 */
int display_get_width();

/**
 * Get display height in pixels
 *
 * @return Height in pixels
 */
int display_get_height();

/**
 * Get the number of attached displays
 *
 * @return Number of displays (usually 1, can be more for multi-display setups)
 */
size_t display_get_count();

#endif // AMBIO_DISPLAY_H
