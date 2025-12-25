#ifndef AMBIO_HARDWARE_H
#define AMBIO_HARDWARE_H

#include <M5Unified.h>
#include "types.h"

// ============================================================================
// Hardware Initialization and Device Detection
// ============================================================================

/**
 * Initialize M5Stack hardware with standard configuration
 *
 * Configures:
 * - Serial communication (115200 baud)
 * - Display clearing and power output
 * - Internal peripherals (IMU, RTC, Speaker, Mic)
 * - External display priority (Unit OLED)
 * - LED brightness
 *
 * This must be called before any other module init functions.
 */
void hardware_init();

/**
 * Get the detected board name as a human-readable string
 *
 * @return Board name (e.g., "StampS3", "ATOMS3", "Capsule")
 *         Returns "Who am I ?" if board type is unknown
 */
const char* get_board_name();

/**
 * Get the detected IMU name as a human-readable string
 *
 * @return IMU chip name (e.g., "MPU6886", "BMI270", "SH200Q")
 *         Returns "none" if no IMU detected
 *         Returns "unknown" for unrecognized IMU types
 */
const char* get_imu_name();

/**
 * Check if an IMU sensor is available
 *
 * @return true if IMU is detected and initialized
 *         false if no IMU is present
 */
bool has_imu();

/**
 * Check if RTC is available
 *
 * @return true if RTC is detected and initialized
 *         false if no RTC is present
 */
bool has_rtc();

/**
 * Check if speaker is available
 *
 * @return true if speaker is detected and initialized
 *         false if no speaker is present
 */
bool has_speaker();

/**
 * Get the M5Stack board type enum
 *
 * @return m5::board_t enum value for current board
 */
m5::board_t get_board_type();

/**
 * Get the IMU type enum
 *
 * @return m5::imu_t enum value for current IMU
 */
m5::imu_t get_imu_type();

#endif // AMBIO_HARDWARE_H
