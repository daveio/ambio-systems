#ifndef AMBIO_AUDIO_H
#define AMBIO_AUDIO_H

#include <M5Unified.h>
#include "types.h"

// ============================================================================
// Audio Management Module
// ============================================================================

/**
 * Initialize audio subsystem and play startup sequence
 *
 * Configures speaker volume and plays startup tones.
 * Must be called after hardware_init()
 */
void audio_init();

/**
 * Set master volume level
 *
 * @param level Volume (0=silent, 255=max)
 */
void audio_set_volume(uint8_t level);

/**
 * Play a tone at specified frequency
 *
 * @param frequency Frequency in Hz (20-20000)
 * @param duration_ms Duration in milliseconds
 * @param wait If true, blocks until tone completes. If false, plays async.
 */
void audio_play_tone(uint16_t frequency, uint32_t duration_ms, bool wait = false);

/**
 * Play startup WAV sound
 *
 * Currently plays embedded WAV data.
 * Future: Will load from LittleFS filesystem.
 */
void audio_play_startup();

/**
 * Check if audio is currently playing
 *
 * @return true if speaker is playing
 */
bool audio_is_playing();

#endif // AMBIO_AUDIO_H
