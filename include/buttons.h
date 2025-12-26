#ifndef AMBIO_BUTTONS_H
#define AMBIO_BUTTONS_H

#include "types.h"  // Provides M5Unified types transitively

// ============================================================================
// Button Input Management Module
// ============================================================================

/**
 * Initialize button subsystem
 *
 * Sets up button state tracking for all available buttons.
 * Must be called after hardware_init()
 */
void buttons_init();

/**
 * Update button states and handle events
 *
 * Polls all buttons (BtnPWR, BtnA, BtnB, BtnC, BtnEXT) and:
 * - Updates LED colors based on button state
 * - Plays audio feedback tones
 * - Displays button state visualization
 * - Logs button events
 *
 * Call this in the main loop after M5.update()
 */
void buttons_update();

/**
 * Check if any button was pressed
 *
 * @return true if any button had an event this frame
 */
bool buttons_any_pressed();

#endif // AMBIO_BUTTONS_H
