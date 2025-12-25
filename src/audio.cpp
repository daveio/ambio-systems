#include "audio.h"
#include <M5Unified.h>

// External WAV data (defined in main.cpp for now, will move to filesystem)
extern const uint8_t wav_8bit_44100[46000];

void audio_init() {
    if (!M5.Speaker.isEnabled()) {
        M5_LOGW("Speaker not available");
        return;
    }

    // Set default volume
    M5.Speaker.setVolume(AUDIO_VOLUME_DEFAULT);

    // Play startup tone sequence
    audio_play_tone(AUDIO_TEST_TONE_HIGH, 100, true);
    audio_play_tone(AUDIO_TEST_TONE_MED, 100, true);

    // Play embedded startup WAV
    audio_play_startup();

    M5_LOGI("Audio initialized: volume=%d", AUDIO_VOLUME_DEFAULT);
}

void audio_set_volume(uint8_t level) {
    M5.Speaker.setVolume(level);
}

void audio_play_tone(uint16_t frequency, uint32_t duration_ms, bool wait) {
    M5.Speaker.tone(frequency, duration_ms);

    if (wait) {
        while (M5.Speaker.isPlaying()) {
            M5.delay(1);
        }
    }
}

void audio_play_startup() {
    if (M5.Speaker.isEnabled()) {
        M5.Speaker.playRaw(wav_8bit_44100, sizeof(wav_8bit_44100),
                          AUDIO_WAV_SAMPLE_RATE, false);
    }
}

bool audio_is_playing() {
    return M5.Speaker.isPlaying();
}
