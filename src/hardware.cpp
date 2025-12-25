#include "hardware.h"
#include <M5Unified.h>
#include <M5UnitOLED.h>

// Module-level state
static m5::board_t detected_board = m5::board_t::board_unknown;
static m5::imu_t detected_imu = m5::imu_t::imu_none;

void hardware_init() {
    // Logging output for debugging
    M5_LOGE("this is error LOG");
    M5_LOGW("this is warning LOG");
    M5_LOGI("this is info LOG");
    M5_LOGD("this is debug LOG");
    M5_LOGV("this is verbose LOG");

    // Create M5 configuration
    auto cfg = M5.config();

    // Serial and basic config
    cfg.serial_baudrate = SERIAL_BAUDRATE;
    cfg.clear_display = true;
    cfg.output_power = true;

    // Internal peripherals
    cfg.internal_imu = true;
    cfg.internal_rtc = true;
    cfg.internal_spk = true;
    cfg.internal_mic = true;

    // External peripherals (disabled)
    cfg.external_imu = false;
    cfg.external_rtc = false;

    // LED configuration
    cfg.led_brightness = LED_BRIGHTNESS_DEFAULT;

    // External speaker configuration (all disabled)
    cfg.external_speaker.module_display = false;
    cfg.external_speaker.hat_spk = false;
    cfg.external_speaker.hat_spk2 = false;
    cfg.external_speaker.atomic_spk = false;
    cfg.external_speaker.atomic_echo = false;
    cfg.external_speaker.module_rca = false;

    // External display configuration
    cfg.external_display.module_display = false;
    cfg.external_display.atom_display = false;
    cfg.external_display.unit_glass = false;
    cfg.external_display.unit_glass2 = false;
    cfg.external_display.unit_oled = true;  // Unit OLED enabled
    cfg.external_display.unit_mini_oled = false;
    cfg.external_display.unit_lcd = false;
    cfg.external_display.unit_rca = false;
    cfg.external_display.module_rca = false;

    // Initialize M5Unified
    M5.begin(cfg);

    // Set primary display priority (Unit OLED first)
    M5.setPrimaryDisplayType({
        m5::board_t::board_M5UnitOLED,
    });

    // Cache detected hardware types
    detected_board = M5.getBoard();
    detected_imu = M5.Imu.getType();

    // Configure touch button area (32 pixels from bottom)
    M5.setTouchButtonHeight(32);

    M5_LOGI("Hardware initialized: board=%s, imu=%s",
            get_board_name(), get_imu_name());
}

const char* get_board_name() {
    switch (detected_board) {
#if defined(CONFIG_IDF_TARGET_ESP32S3)
    case m5::board_t::board_M5StackCoreS3:    return "StackCoreS3";
    case m5::board_t::board_M5StackCoreS3SE:  return "StackCoreS3SE";
    case m5::board_t::board_M5StampS3:        return "StampS3";
    case m5::board_t::board_M5AtomS3U:        return "ATOMS3U";
    case m5::board_t::board_M5AtomS3Lite:     return "ATOMS3Lite";
    case m5::board_t::board_M5AtomS3:         return "ATOMS3";
    case m5::board_t::board_M5AtomS3R:        return "ATOMS3R";
    case m5::board_t::board_M5AtomS3RCam:     return "ATOMS3R Camera";
    case m5::board_t::board_M5AtomS3RExt:     return "ATOMS3R Ext";
    case m5::board_t::board_M5AtomEchoS3R:    return "ATOM ECHO S3R";
    case m5::board_t::board_M5Dial:           return "Dial";
    case m5::board_t::board_M5DinMeter:       return "DinMeter";
    case m5::board_t::board_M5Capsule:        return "Capsule";
    case m5::board_t::board_M5Cardputer:      return "Cardputer";
    case m5::board_t::board_M5CardputerADV:   return "CardputerADV";
    case m5::board_t::board_M5VAMeter:        return "VAMeter";
    case m5::board_t::board_M5PaperS3:        return "PaperS3";
    case m5::board_t::board_M5PowerHub:       return "PowerHub";
#elif defined(CONFIG_IDF_TARGET_ESP32C3)
    case m5::board_t::board_M5StampC3:        return "StampC3";
    case m5::board_t::board_M5StampC3U:       return "StampC3U";
#elif defined(CONFIG_IDF_TARGET_ESP32C6)
    case m5::board_t::board_M5NanoC6:         return "NanoC6";
    case m5::board_t::board_M5UnitC6L:        return "UnitC6L";
    case m5::board_t::board_ArduinoNessoN1:   return "NessoN1";
#elif defined(CONFIG_IDF_TARGET_ESP32P4)
    case m5::board_t::board_M5Tab5:           return "Tab5";
#else
    case m5::board_t::board_M5Stack:          return "Stack";
    case m5::board_t::board_M5StackCore2:     return "StackCore2";
    case m5::board_t::board_M5StickC:         return "StickC";
    case m5::board_t::board_M5StickCPlus:     return "StickCPlus";
    case m5::board_t::board_M5StickCPlus2:    return "StickCPlus2";
    case m5::board_t::board_M5StackCoreInk:   return "CoreInk";
    case m5::board_t::board_M5Paper:          return "Paper";
    case m5::board_t::board_M5Tough:          return "Tough";
    case m5::board_t::board_M5Station:        return "Station";
    case m5::board_t::board_M5AtomLite:       return "ATOM Lite";
    case m5::board_t::board_M5AtomMatrix:     return "ATOM Matrix";
    case m5::board_t::board_M5AtomEcho:       return "ATOM ECHO";
    case m5::board_t::board_M5AtomPsram:      return "ATOM PSRAM";
    case m5::board_t::board_M5AtomU:          return "ATOM U";
    case m5::board_t::board_M5TimerCam:       return "TimerCamera";
    case m5::board_t::board_M5StampPico:      return "StampPico";
#endif
    default:
        return "Who am I ?";
    }
}

const char* get_imu_name() {
    switch (detected_imu) {
    case m5::imu_t::imu_mpu6050:
        return "MPU6050";
    case m5::imu_t::imu_mpu6886:
        return "MPU6886";
    case m5::imu_t::imu_mpu9250:
        return "MPU9250";
    case m5::imu_t::imu_bmi270:
        return "BMI270";
    case m5::imu_t::imu_sh200q:
        return "SH200Q";
    case m5::imu_t::imu_none:
        return "none";
    default:
        return "unknown";
    }
}

bool has_imu() {
    return M5.Imu.isEnabled();
}

bool has_rtc() {
    return M5.Rtc.isEnabled();
}

bool has_speaker() {
    return M5.Speaker.isEnabled();
}

m5::board_t get_board_type() {
    return detected_board;
}

m5::imu_t get_imu_type() {
    return detected_imu;
}
