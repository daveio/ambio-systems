#include "audio.h"
#include <M5Unified.h>
#include <LittleFS.h>

// WAV file configuration
static constexpr const char* STARTUP_WAV_PATH = "/audio/startup.wav";
static constexpr size_t WAV_BUFFER_SIZE = 48000;  // 46KB WAV + header
static uint8_t* wav_buffer = nullptr;

void audio_init() {
    if (!M5.Speaker.isEnabled()) {
        M5_LOGW("Speaker not available");
        return;
    }

    // Initialize LittleFS
    if (!LittleFS.begin(true)) {  // true = format on failure
        M5_LOGE("LittleFS mount failed");
        return;
    }
    M5_LOGI("LittleFS mounted successfully");

    // Set default volume
    M5.Speaker.setVolume(AUDIO_VOLUME_DEFAULT);

    // Play startup tone sequence
    audio_play_tone(AUDIO_TEST_TONE_HIGH, 100, true);
    audio_play_tone(AUDIO_TEST_TONE_MED, 100, true);

    // Play startup WAV from filesystem
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
    if (!M5.Speaker.isEnabled()) {
        return;
    }

    // Open WAV file from LittleFS
    File wav_file = LittleFS.open(STARTUP_WAV_PATH, "r");
    if (!wav_file) {
        M5_LOGE("Failed to open %s", STARTUP_WAV_PATH);
        return;
    }

    size_t file_size = wav_file.size();
    M5_LOGI("Loading WAV: %s (%d bytes)", STARTUP_WAV_PATH, file_size);

    // Allocate buffer if needed
    if (wav_buffer == nullptr) {
        wav_buffer = (uint8_t*)malloc(WAV_BUFFER_SIZE);
        if (wav_buffer == nullptr) {
            M5_LOGE("Failed to allocate WAV buffer");
            wav_file.close();
            return;
        }
    }

    // Read entire file into buffer
    size_t bytes_read = wav_file.read(wav_buffer, file_size);
    wav_file.close();

    if (bytes_read != file_size) {
        M5_LOGE("WAV read error: expected %d bytes, got %d", file_size, bytes_read);
        return;
    }

    // Skip WAV header (44 bytes) and play raw PCM data
    const uint8_t* pcm_data = wav_buffer + 44;
    size_t pcm_size = file_size - 44;

    M5.Speaker.playRaw(pcm_data, pcm_size, AUDIO_WAV_SAMPLE_RATE, false);
    M5_LOGI("Playing WAV: %d bytes PCM data", pcm_size);
}

bool audio_is_playing() {
    return M5.Speaker.isPlaying();
}
