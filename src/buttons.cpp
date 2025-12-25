#include "buttons.h"
#include "display.h"
#include <M5Unified.h>

// Module-level state
static bool buttons_initialized = false;
static bool any_button_event = false;

// Button tone frequencies (musical notes)
static constexpr uint16_t TONE_BTN_PWR = 784;  // G5
static constexpr uint16_t TONE_BTN_A = 523;    // C5
static constexpr uint16_t TONE_BTN_B = 587;    // D5
static constexpr uint16_t TONE_BTN_C = 659;    // E5
static constexpr uint16_t TONE_BTN_EXT = 698;  // F5

void buttons_init() {
    buttons_initialized = true;
    M5_LOGI("Buttons initialized");
}

/**
 * Helper function to process a single button
 * Returns the button state (0-5) or 0 if no event
 */
template<typename ButtonType>
static int process_button(
    ButtonType& button,
    const char* name,
    uint16_t tone_freq,
    int display_row
) {
    int state = button.wasHold() ? 1
              : button.wasClicked() ? 2
              : button.wasPressed() ? 3
              : button.wasReleased() ? 4
              : button.wasDecideClickCount() ? 5
              : 0;

    if (state) {
        // Set LED color based on button state
        M5.Led.setAllColor(BUTTON_STATE_COLORS[state]);

        // Play audio feedback
        M5.Speaker.tone(tone_freq, BUTTON_TONE_DURATION_MS);

        // Log event
        M5_LOGI("%s:%s  count:%d", name, BUTTON_STATE_NAMES[state],
                button.getClickCount());

        // Display visualization (if display is not busy)
        if (!M5.Display.displayBusy()) {
            int h = display_get_height() / 8;
            M5.Display.fillRect(0, h * display_row, h, h - 1,
                              BUTTON_STATE_COLORS[state]);
            M5.Display.setCursor(0, h * display_row);
            M5.Display.printf("%d", button.getClickCount());
        }

        any_button_event = true;
    }

    return state;
}

void buttons_update() {
    any_button_event = false;

    display_begin_frame();

    // Process each button with its specific tone and display row
    process_button(M5.BtnPWR, "BtnPWR", TONE_BTN_PWR, 2);
    process_button(M5.BtnA, "BtnA", TONE_BTN_A, 3);
    process_button(M5.BtnB, "BtnB", TONE_BTN_B, 4);
    process_button(M5.BtnC, "BtnC", TONE_BTN_C, 5);
    process_button(M5.BtnEXT, "BtnEXT", TONE_BTN_EXT, 6);

    display_end_frame();
}

bool buttons_any_pressed() {
    return any_button_event;
}
