#ifndef AMBIO_SENSORS_H
#define AMBIO_SENSORS_H

#include <M5Unified.h>
#include "types.h"

// ============================================================================
// Sensor Management Module (Battery, RTC, IMU)
// ============================================================================

/**
 * Initialize sensor subsystem
 *
 * Sets up state tracking for battery, RTC, and IMU sensors.
 * Must be called after hardware_init()
 */
void sensors_init();

/**
 * Update all sensor readings and display visualization
 *
 * Polls and displays:
 * - Battery level (0-100%)
 * - RTC date/time
 * - IMU accelerometer and gyroscope data (bar graph)
 *
 * Call this in the main loop
 */
void sensors_update();

/**
 * Get current battery level
 *
 * @return Battery percentage (0-100), or negative if unavailable
 */
int sensors_get_battery_level();

/**
 * Check if RTC is available and functional
 *
 * @return true if RTC is enabled
 */
bool sensors_rtc_available();

/**
 * Check if IMU is available and functional
 *
 * @return true if IMU is enabled
 */
bool sensors_imu_available();

#endif // AMBIO_SENSORS_H
