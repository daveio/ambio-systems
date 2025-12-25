#include "display.h"
#include <M5Unified.h>
#include <M5UnitOLED.h>

// Module-level state
static bool display_initialized = false;

void display_init() {
    // Set EPD mode for e-paper displays (fastest mode)
    M5.Display.setEpdMode(epd_mode_t::epd_fastest);

    // Set default brightness for LCD displays
    M5.Display.setBrightness(DISPLAY_BRIGHTNESS_MEDIUM);

    // Force landscape mode if display is in portrait
    if (M5.Display.width() < M5.Display.height()) {
        M5.Display.setRotation(M5.Display.getRotation() ^ 1);
    }

    // Multi-display initialization test
    // Draw a colorful gradient pattern on all attached displays
    size_t display_count = M5.getDisplayCount();
    for (int i = 0; i < display_count; ++i) {
        M5.Displays(i).startWrite();

        // Draw 128x128 gradient pattern
        for (int y = 0; y < 128; ++y) {
            for (int x = 0; x < 128; ++x) {
                M5.Displays(i).writePixel(x, y, M5.Display.color888(x*2, x+y, y*2));
            }
        }

        M5.Displays(i).printf("Display %d\n", i);
        M5.Displays(i).endWrite();
    }

    // Configure text size based on display height
    int textsize = M5.Display.height() / 160;
    if (textsize == 0) {
        textsize = 1;
    }
    M5.Display.setTextSize(textsize);

    display_initialized = true;
    M5_LOGI("Display initialized: %dx%d, %zu display(s)",
            M5.Display.width(), M5.Display.height(), display_count);
}

void display_set_brightness(uint8_t level) {
    M5.Display.setBrightness(level);
}

void display_clear() {
    M5.Display.clear();
}

void display_print_board_info(const char* board_name, const char* imu_name) {
    M5.Display.startWrite();
    M5.Display.print("Core:");
    M5.Display.println(board_name);
    M5.Display.print("IMU:");
    M5.Display.println(imu_name);
    M5.Display.endWrite();

    M5_LOGI("Displayed: core=%s, imu=%s", board_name, imu_name);
}

void display_begin_frame() {
    M5.Display.startWrite();
}

void display_end_frame() {
    M5.Display.endWrite();
}

int display_get_width() {
    return M5.Display.width();
}

int display_get_height() {
    return M5.Display.height();
}

size_t display_get_count() {
    return M5.getDisplayCount();
}
