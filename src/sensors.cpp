#include "sensors.h"
#include "display.h"
#include <M5Unified.h>
#include <time.h>

// Module-level state
static int prev_battery = INT_MAX;
static int prev_xpos[IMU_GRAPH_CHANNELS] = {0};

void sensors_init() {
    M5_LOGI("Sensors initialized");
}

// Battery level monitoring and display
static void update_battery() {
    int battery = M5.Power.getBatteryLevel();
    if (prev_battery != battery) {
        prev_battery = battery;

        M5.Display.startWrite();
        M5.Display.setCursor(0, M5.Display.fontHeight() * 3);
        M5.Display.print("Bat:");
        if (battery >= 0) {
            M5.Display.printf("%03d", battery);
        } else {
            M5.Display.print("none");
        }
        M5.Display.endWrite();
    }
}

// RTC date/time display
static void update_rtc() {
    if (!M5.Rtc.isEnabled()) {
        return;
    }

    static constexpr const char* const wd[] = {
        "Sun", "Mon", "Tue", "Wed", "Thr", "Fri", "Sat", "ERR"
    };
    char buf[32];

    // Get date/time from RTC hardware
    m5::rtc_datetime_t dt;
    if (M5.Rtc.getDateTime(&dt)) {
        M5.Display.startWrite();

        // Display date: YYYY/MM/DD(Day)
        snprintf(buf, 30, "%04d/%02d/%02d(%s)",
                dt.date.year,
                dt.date.month,
                dt.date.date,
                wd[dt.date.weekDay & 7]);
        M5.Display.drawString(buf, M5.Display.width() / 2, 0);

        // Display time: HH:MM:SS
        snprintf(buf, 30, "%02d:%02d:%02d",
                dt.time.hours,
                dt.time.minutes,
                dt.time.seconds);
        M5.Display.drawString(buf, M5.Display.width() / 2, M5.Display.fontHeight());

        M5.Display.endWrite();
    } else {
        // Fallback: Use ESP32 system time (less accurate, ~few seconds drift/day)
        auto t = time(nullptr);
        auto time_info = localtime(&t);

        M5.Display.startWrite();

        snprintf(buf, 30, "%04d/%02d/%02d(%s)",
                time_info->tm_year + 1900,
                time_info->tm_mon + 1,
                time_info->tm_mday,
                wd[time_info->tm_wday & 7]);
        M5.Display.drawString(buf, M5.Display.width() / 2, 0);

        snprintf(buf, 30, "%02d:%02d:%02d",
                time_info->tm_hour,
                time_info->tm_min,
                time_info->tm_sec);
        M5.Display.drawString(buf, M5.Display.width() / 2, M5.Display.fontHeight());

        M5.Display.endWrite();
    }
}

// IMU accelerometer/gyroscope visualization
static void update_imu() {
    if (!M5.Imu.isEnabled()) {
        return;
    }

    // Calculate center offset for bar graphs
    int h = display_get_height() / 8;
    int ox = (display_get_width() + h) >> 1;

    // Current and previous positions for each axis
    int xpos[IMU_GRAPH_CHANNELS];
    float val[IMU_GRAPH_CHANNELS];

    // Read sensor data: accel (X, Y, Z) and gyro (X, Y, Z)
    M5.Imu.getAccel(&val[0], &val[1], &val[2]);
    M5.Imu.getGyro(&val[3], &val[4], &val[5]);

    // Color scheme: RGB for both accel and gyro
    int color[IMU_GRAPH_CHANNELS] = {
        TFT_RED, TFT_GREEN, TFT_BLUE,  // Accel X, Y, Z
        TFT_RED, TFT_GREEN, TFT_BLUE   // Gyro X, Y, Z
    };

    // Scale values for display
    for (int i = 0; i < 3; ++i) {
        xpos[i] = val[i] * 50;        // Accel: ±2g → ±100px
        xpos[i + 3] = val[i + 3] / 2;  // Gyro: scale down for visibility
    }

    // Render bar graphs with clipping region
    M5.Display.startWrite();
    M5.Display.setClipRect(h, h, display_get_width(), display_get_height());

    // Wait for display if busy (e-paper displays)
    while (M5.Display.displayBusy()) {
        M5.delay(1);
    }

    // Draw each axis bar graph
    for (int i = 0; i < IMU_GRAPH_CHANNELS; ++i) {
        if (xpos[i] == prev_xpos[i]) {
            continue;  // No change, skip update
        }

        int px = prev_xpos[i];

        // Handle sign change (crossing zero)
        if ((xpos[i] < 0) != (px < 0)) {
            if (px) {
                M5.Display.fillRect(ox, h * (i + 2), px, h,
                                  M5.Display.getBaseColor());
            }
            px = 0;
        }

        // Draw bar (either growing or shrinking)
        if (xpos[i] != px) {
            if ((xpos[i] > px) != (xpos[i] < 0)) {
                M5.Display.setColor(color[i]);  // Growing: use axis color
            } else {
                M5.Display.setColor(M5.Display.getBaseColor());  // Shrinking: erase
            }
            M5.Display.fillRect(xpos[i] + ox, h * (i + 2), px - xpos[i], h);
        }

        prev_xpos[i] = xpos[i];
    }

    M5.Display.clearClipRect();
    M5.Display.endWrite();
}

// Main sensor update function
void sensors_update() {
    update_battery();
    update_rtc();
    update_imu();
    M5.Display.display();  // Flush all display updates
}
