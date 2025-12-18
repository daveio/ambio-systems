# Phase 5: M5 Capsule Specifics

## Learning Objectives

By the end of this phase, you will:

- Know the M5 Capsule's complete pinout and hardware layout
- Handle the button, LED, and power management correctly
- Implement battery monitoring and low-power modes
- Create a production-ready recording application
- Understand the quirks specific to this hardware

---

## Step 1: M5 Capsule Hardware Overview

The M5 Capsule is essentially an ESP32-S3 crammed into a tube with some peripherals attached. Before writing code that interacts with the hardware, you need to understand what's actually in there.

### 1.1 Core Specifications

| Component    | Details                           |
| ------------ | --------------------------------- |
| MCU          | ESP32-S3FN8 (8MB Flash, no PSRAM) |
| Battery      | 120mAh Li-Po (built-in)           |
| Microphone   | SPM1423 PDM MEMS                  |
| Storage      | microSD slot                      |
| Connectivity | USB-C, WiFi, Bluetooth            |
| Button       | Single physical button            |
| LED          | RGB LED (WS2812-compatible)       |

> **Note:** Some documentation suggests PSRAM, but the Capsule variant typically ships without it. Check your specific unit. This affects buffer allocation strategies.

### 1.2 Complete Pinout

Here's the definitive pin mapping for the M5 Capsule:

| Function             | GPIO | Notes                        |
| -------------------- | ---- | ---------------------------- |
| **Microphone**       |      |                              |
| PDM Clock            | 43   | Output to mic                |
| PDM Data             | 44   | Input from mic               |
| **SD Card**          |      |                              |
| SPI MOSI             | 14   |                              |
| SPI MISO             | 39   |                              |
| SPI CLK              | 12   |                              |
| SPI CS               | 11   |                              |
| **User Interface**   |      |                              |
| Button               | 42   | Active LOW, internal pull-up |
| RGB LED              | 21   | WS2812 data line             |
| **Power Management** |      |                              |
| Battery Voltage      | 38   | ADC input                    |
| Power Hold           | 46   | Keep HIGH to stay on         |
| **USB**              |      |                              |
| USB D+               | 20   | USB data                     |
| USB D-               | 19   | USB data                     |
| **I2C (Grove)**      |      |                              |
| SDA                  | 13   | Grove connector              |
| SCL                  | 15   | Grove connector              |

### 1.3 Hardware Block Diagram

```
                                    ┌─────────────────┐
                                    │   USB-C Port    │
                                    └────────┬────────┘
                                             │
┌────────────────────────────────────────────┼────────────────────────────────────────────┐
│ M5 Capsule                                 │                                            │
│                                            │                                            │
│  ┌──────────┐   ┌──────────────────────────┼───────────────────────────┐               │
│  │ SPM1423  │   │                    ESP32-S3                          │               │
│  │   Mic    │◄──┤ GPIO 43 (CLK)                                        │               │
│  │          ├──►│ GPIO 44 (DATA)                                       │               │
│  └──────────┘   │                                                      │               │
│                 │ GPIO 14 ─────►┌───────────┐                          │               │
│                 │ GPIO 39 ◄─────│  SD Card  │                          │               │
│                 │ GPIO 12 ─────►│   Slot    │                          │               │
│                 │ GPIO 11 ─────►└───────────┘                          │               │
│                 │                                                      │               │
│  ┌──────────┐   │ GPIO 42 ◄─────┬────────────                          │               │
│  │  Button  ├───┤               │                                      │               │
│  └──────────┘   │               GND                                    │               │
│                 │                                                      │               │
│  ┌──────────┐   │ GPIO 21 ─────►                                       │               │
│  │ RGB LED  │◄──┤                                                      │               │
│  └──────────┘   │                                                      │               │
│                 │ GPIO 38 ◄───── Battery ADC                           │               │
│  ┌──────────┐   │ GPIO 46 ─────► Power Hold                            │               │
│  │ 120mAh   │   │                                                      │               │
│  │ Battery  │   └──────────────────────────────────────────────────────┘               │
│  └──────────┘                                                                          │
└────────────────────────────────────────────────────────────────────────────────────────┘
```

---

## Step 2: RGB LED Control

The M5 Capsule's LED is a WS2812-compatible RGB LED, which uses a timing-sensitive single-wire protocol. Fortunately, the ESP32-S3's RMT peripheral handles this without bit-banging.

### 2.1 LED Driver Setup

```c
#include "driver/rmt_tx.h"
#include "led_strip.h"
#include "led_strip_types.h"

#define LED_GPIO        21
#define LED_RMT_CHANNEL 0

static led_strip_handle_t led_strip = NULL;

esp_err_t init_led(void)
{
    led_strip_config_t strip_config = {
        .strip_gpio_num = LED_GPIO,
        .max_leds = 1,  // Single LED
        .led_pixel_format = LED_PIXEL_FORMAT_GRB,
        .led_model = LED_MODEL_WS2812,
        .flags.invert_out = false,
    };

    led_strip_rmt_config_t rmt_config = {
        .clk_src = RMT_CLK_SRC_DEFAULT,
        .resolution_hz = 10 * 1000 * 1000,  // 10MHz
        .flags.with_dma = false,
    };

    return led_strip_new_rmt_device(&strip_config, &rmt_config, &led_strip);
}

esp_err_t set_led_color(uint8_t red, uint8_t green, uint8_t blue)
{
    if (led_strip == NULL) return ESP_FAIL;

    ESP_ERROR_CHECK(led_strip_set_pixel(led_strip, 0, red, green, blue));
    return led_strip_refresh(led_strip);
}

esp_err_t led_off(void)
{
    if (led_strip == NULL) return ESP_FAIL;
    return led_strip_clear(led_strip);
}
```

### 2.2 Adding to platformio.ini

You'll need the LED strip component:

```ini
lib_deps =
    espressif/led_strip @ ^2.5.0
```

### 2.3 Status Indication Patterns

```c
typedef enum {
    LED_STATUS_IDLE,        // Off or dim white
    LED_STATUS_RECORDING,   // Red, pulsing
    LED_STATUS_ERROR,       // Fast red blink
    LED_STATUS_LOW_BATTERY, // Orange
    LED_STATUS_READY,       // Green
} led_status_t;

static led_status_t current_status = LED_STATUS_IDLE;

void led_status_task(void *pvParameters)
{
    uint8_t brightness = 0;
    int8_t direction = 1;

    while (1) {
        switch (current_status) {
            case LED_STATUS_IDLE:
                set_led_color(5, 5, 5);  // Dim white
                vTaskDelay(pdMS_TO_TICKS(500));
                break;

            case LED_STATUS_RECORDING:
                // Pulsing red
                brightness += direction * 5;
                if (brightness >= 100 || brightness <= 10) {
                    direction = -direction;
                }
                set_led_color(brightness, 0, 0);
                vTaskDelay(pdMS_TO_TICKS(30));
                break;

            case LED_STATUS_ERROR:
                set_led_color(100, 0, 0);
                vTaskDelay(pdMS_TO_TICKS(100));
                led_off();
                vTaskDelay(pdMS_TO_TICKS(100));
                break;

            case LED_STATUS_LOW_BATTERY:
                set_led_color(100, 50, 0);  // Orange
                vTaskDelay(pdMS_TO_TICKS(500));
                break;

            case LED_STATUS_READY:
                set_led_color(0, 50, 0);  // Green
                vTaskDelay(pdMS_TO_TICKS(500));
                break;
        }
    }
}

void set_led_status(led_status_t status)
{
    current_status = status;
}
```

---

## Step 3: Button Handling

The M5 Capsule has a single button. To get more functionality from one button, we need to distinguish between short presses, long presses, and potentially double-clicks.

### 3.1 Debounced Button with Press Types

```c
#define BUTTON_GPIO         42
#define DEBOUNCE_MS         50
#define LONG_PRESS_MS       1000
#define DOUBLE_CLICK_MS     300

typedef enum {
    BUTTON_EVENT_NONE,
    BUTTON_EVENT_SHORT_PRESS,
    BUTTON_EVENT_LONG_PRESS,
    BUTTON_EVENT_DOUBLE_CLICK,
} button_event_t;

static QueueHandle_t button_event_queue = NULL;

void button_task(void *pvParameters)
{
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << BUTTON_GPIO),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    gpio_config(&io_conf);

    bool last_state = true;
    bool current_state;
    TickType_t press_start = 0;
    TickType_t last_click_time = 0;
    int click_count = 0;

    while (1) {
        current_state = gpio_get_level(BUTTON_GPIO);

        // Button pressed (active low)
        if (!current_state && last_state) {
            vTaskDelay(pdMS_TO_TICKS(DEBOUNCE_MS));
            current_state = gpio_get_level(BUTTON_GPIO);

            if (!current_state) {
                press_start = xTaskGetTickCount();
            }
        }

        // Button released
        if (current_state && !last_state) {
            vTaskDelay(pdMS_TO_TICKS(DEBOUNCE_MS));
            current_state = gpio_get_level(BUTTON_GPIO);

            if (current_state && press_start != 0) {
                TickType_t press_duration = xTaskGetTickCount() - press_start;

                if (press_duration >= pdMS_TO_TICKS(LONG_PRESS_MS)) {
                    // Long press
                    button_event_t event = BUTTON_EVENT_LONG_PRESS;
                    xQueueSend(button_event_queue, &event, 0);
                    click_count = 0;
                } else {
                    // Short press - check for double click
                    TickType_t now = xTaskGetTickCount();

                    if ((now - last_click_time) < pdMS_TO_TICKS(DOUBLE_CLICK_MS)) {
                        click_count++;
                        if (click_count >= 2) {
                            button_event_t event = BUTTON_EVENT_DOUBLE_CLICK;
                            xQueueSend(button_event_queue, &event, 0);
                            click_count = 0;
                        }
                    } else {
                        // Single click (send after timeout if no second click)
                        click_count = 1;
                    }
                    last_click_time = now;
                }
                press_start = 0;
            }
        }

        // Send single click after double-click timeout
        if (click_count == 1) {
            if ((xTaskGetTickCount() - last_click_time) >= pdMS_TO_TICKS(DOUBLE_CLICK_MS)) {
                button_event_t event = BUTTON_EVENT_SHORT_PRESS;
                xQueueSend(button_event_queue, &event, 0);
                click_count = 0;
            }
        }

        last_state = current_state;
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}
```

### 3.2 Using Button Events

```c
void handle_button_events(void *pvParameters)
{
    button_event_t event;

    while (1) {
        if (xQueueReceive(button_event_queue, &event, portMAX_DELAY) == pdTRUE) {
            switch (event) {
                case BUTTON_EVENT_SHORT_PRESS:
                    ESP_LOGI(TAG, "Short press - toggle recording");
                    if (is_recording) {
                        stop_recording();
                    } else {
                        start_recording();
                    }
                    break;

                case BUTTON_EVENT_LONG_PRESS:
                    ESP_LOGI(TAG, "Long press - power off");
                    power_off();
                    break;

                case BUTTON_EVENT_DOUBLE_CLICK:
                    ESP_LOGI(TAG, "Double click - mark timestamp");
                    mark_timestamp();
                    break;

                default:
                    break;
            }
        }
    }
}
```

---

## Step 4: Battery Monitoring

The M5 Capsule has an ADC input connected to the battery through a voltage divider. Monitoring battery level prevents unpleasant surprises like recordings cutting off mid-sentence.

### 4.1 ADC Configuration

```c
#include "esp_adc/adc_oneshot.h"
#include "esp_adc/adc_cali.h"
#include "esp_adc/adc_cali_scheme.h"

#define BATTERY_ADC_GPIO    38
#define BATTERY_ADC_CHANNEL ADC_CHANNEL_2  // GPIO 38 is ADC1_CH2

static adc_oneshot_unit_handle_t adc_handle = NULL;
static adc_cali_handle_t cali_handle = NULL;

esp_err_t init_battery_monitor(void)
{
    // Configure ADC unit
    adc_oneshot_unit_init_cfg_t init_config = {
        .unit_id = ADC_UNIT_1,
    };
    ESP_ERROR_CHECK(adc_oneshot_new_unit(&init_config, &adc_handle));

    // Configure ADC channel
    adc_oneshot_chan_cfg_t chan_config = {
        .bitwidth = ADC_BITWIDTH_12,
        .atten = ADC_ATTEN_DB_12,  // Full range for 3.3V measurement
    };
    ESP_ERROR_CHECK(adc_oneshot_config_channel(adc_handle, BATTERY_ADC_CHANNEL, &chan_config));

    // Calibration
    adc_cali_line_fitting_config_t cali_config = {
        .unit_id = ADC_UNIT_1,
        .atten = ADC_ATTEN_DB_12,
        .bitwidth = ADC_BITWIDTH_12,
    };

    esp_err_t ret = adc_cali_create_scheme_line_fitting(&cali_config, &cali_handle);
    if (ret != ESP_OK) {
        ESP_LOGW(TAG, "ADC calibration not available, using raw values");
    }

    return ESP_OK;
}

// Read battery voltage in millivolts
int read_battery_mv(void)
{
    int raw_value;
    ESP_ERROR_CHECK(adc_oneshot_read(adc_handle, BATTERY_ADC_CHANNEL, &raw_value));

    int voltage_mv;
    if (cali_handle != NULL) {
        adc_cali_raw_to_voltage(cali_handle, raw_value, &voltage_mv);
    } else {
        // Approximate conversion without calibration
        voltage_mv = (raw_value * 3300) / 4095;
    }

    // The voltage divider halves the battery voltage
    // Actual battery voltage = measured * 2
    return voltage_mv * 2;
}

// Convert voltage to percentage (rough approximation for Li-Po)
int battery_percentage(int voltage_mv)
{
    // Li-Po typical range: 3.0V (0%) to 4.2V (100%)
    // These values are approximate and depend on discharge curve

    if (voltage_mv >= 4200) return 100;
    if (voltage_mv <= 3000) return 0;

    // Linear approximation (good enough for user feedback)
    return (voltage_mv - 3000) * 100 / 1200;
}
```

### 4.2 Battery Monitoring Task

```c
#define LOW_BATTERY_THRESHOLD   20  // Percent
#define CRITICAL_BATTERY_THRESHOLD 5

void battery_monitor_task(void *pvParameters)
{
    while (1) {
        int mv = read_battery_mv();
        int percent = battery_percentage(mv);

        ESP_LOGI(TAG, "Battery: %d mV (%d%%)", mv, percent);

        if (percent <= CRITICAL_BATTERY_THRESHOLD) {
            ESP_LOGW(TAG, "Critical battery - shutting down");
            stop_recording();  // Save current file
            set_led_status(LED_STATUS_ERROR);
            vTaskDelay(pdMS_TO_TICKS(2000));
            power_off();
        } else if (percent <= LOW_BATTERY_THRESHOLD) {
            set_led_status(LED_STATUS_LOW_BATTERY);
        }

        vTaskDelay(pdMS_TO_TICKS(30000));  // Check every 30 seconds
    }
}
```

---

## Step 5: Power Management

The M5 Capsule has a power hold pin that must be kept HIGH to maintain power. This allows software-controlled shutdown.

### 5.1 Power Control

```c
#define POWER_HOLD_GPIO 46

void init_power_control(void)
{
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << POWER_HOLD_GPIO),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
    };
    gpio_config(&io_conf);

    // Keep power on
    gpio_set_level(POWER_HOLD_GPIO, 1);
}

void power_off(void)
{
    ESP_LOGI(TAG, "Powering off...");

    // Give other tasks time to clean up
    vTaskDelay(pdMS_TO_TICKS(100));

    // Turn off LED
    led_off();

    // Release power hold
    gpio_set_level(POWER_HOLD_GPIO, 0);

    // If we're still running (USB power?), enter deep sleep
    vTaskDelay(pdMS_TO_TICKS(100));
    esp_deep_sleep_start();
}
```

### 5.2 Light Sleep Between Operations

For battery life, the ESP32-S3 can enter light sleep when not actively recording:

```c
#include "esp_sleep.h"
#include "esp_pm.h"

void configure_power_management(void)
{
    // Configure automatic light sleep
    esp_pm_config_t pm_config = {
        .max_freq_mhz = 240,
        .min_freq_mhz = 80,
        .light_sleep_enable = true,
    };

    ESP_ERROR_CHECK(esp_pm_configure(&pm_config));

    // Configure button to wake from light sleep
    ESP_ERROR_CHECK(gpio_wakeup_enable(BUTTON_GPIO, GPIO_INTR_LOW_LEVEL));
    ESP_ERROR_CHECK(esp_sleep_enable_gpio_wakeup());
}
```

---

## Step 6: Complete Production Application

Putting it all together into a cohesive application:

### 6.1 Project Structure

```
pendant-recorder/
├── include/
│   ├── audio.h
│   ├── storage.h
│   ├── ui.h
│   └── power.h
├── src/
│   ├── main.c
│   ├── audio.c
│   ├── storage.c
│   ├── ui.c
│   └── power.c
├── platformio.ini
└── sdkconfig.defaults
```

### 6.2 Main Application (src/main.c)

```c
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "esp_log.h"

#include "audio.h"
#include "storage.h"
#include "ui.h"
#include "power.h"

static const char *TAG = "pendant";

void app_main(void)
{
    ESP_LOGI(TAG, "M5 Capsule Audio Pendant Starting");
    ESP_LOGI(TAG, "Firmware version: 1.0.0");

    // Initialise power control first (keeps device on)
    power_init();

    // Initialise UI (LED and button)
    if (ui_init() != ESP_OK) {
        ESP_LOGE(TAG, "UI init failed");
        power_off();
        return;
    }
    ui_set_status(UI_STATUS_STARTING);

    // Initialise storage (SD card)
    if (storage_init() != ESP_OK) {
        ESP_LOGE(TAG, "Storage init failed - check SD card");
        ui_set_status(UI_STATUS_ERROR);
        vTaskDelay(pdMS_TO_TICKS(5000));
        power_off();
        return;
    }

    // Initialise audio (microphone)
    if (audio_init() != ESP_OK) {
        ESP_LOGE(TAG, "Audio init failed");
        ui_set_status(UI_STATUS_ERROR);
        vTaskDelay(pdMS_TO_TICKS(5000));
        power_off();
        return;
    }

    // Start battery monitor
    power_start_battery_monitor();

    // Ready!
    ui_set_status(UI_STATUS_READY);
    ESP_LOGI(TAG, "System ready");
    ESP_LOGI(TAG, "Short press: Start/Stop recording");
    ESP_LOGI(TAG, "Long press: Power off");
    ESP_LOGI(TAG, "Double click: Mark timestamp");

    // Main event loop
    while (1) {
        ui_event_t event = ui_get_event(pdMS_TO_TICKS(1000));

        switch (event) {
            case UI_EVENT_BUTTON_SHORT:
                if (audio_is_recording()) {
                    audio_stop_recording();
                    ui_set_status(UI_STATUS_READY);
                } else {
                    if (audio_start_recording() == ESP_OK) {
                        ui_set_status(UI_STATUS_RECORDING);
                    } else {
                        ui_set_status(UI_STATUS_ERROR);
                        vTaskDelay(pdMS_TO_TICKS(1000));
                        ui_set_status(UI_STATUS_READY);
                    }
                }
                break;

            case UI_EVENT_BUTTON_LONG:
                ESP_LOGI(TAG, "User requested power off");
                audio_stop_recording();
                ui_set_status(UI_STATUS_IDLE);
                vTaskDelay(pdMS_TO_TICKS(500));
                power_off();
                break;

            case UI_EVENT_BUTTON_DOUBLE:
                ESP_LOGI(TAG, "Timestamp marker");
                audio_mark_timestamp();
                break;

            case UI_EVENT_NONE:
            default:
                break;
        }
    }
}
```

### 6.3 sdkconfig.defaults

Create this file to set ESP-IDF options consistently:

```
# USB Console
CONFIG_ESP_CONSOLE_USB_SERIAL_JTAG=y

# FreeRTOS
CONFIG_FREERTOS_HZ=1000
CONFIG_FREERTOS_TIMER_TASK_STACK_DEPTH=3072

# Power Management
CONFIG_PM_ENABLE=y
CONFIG_PM_DFS_INIT_AUTO=y

# Logging
CONFIG_LOG_DEFAULT_LEVEL_INFO=y
CONFIG_LOG_MAXIMUM_LEVEL_DEBUG=y

# FAT Filesystem
CONFIG_FATFS_LFN_HEAP=y
CONFIG_FATFS_MAX_LFN=255

# I2S
CONFIG_SOC_I2S_SUPPORTS_PDM_RX=y
```

---

## Step 7: Testing Checklist

Before declaring victory, verify:

### 7.1 Hardware Tests

- [ ] LED lights up in all colours (red, green, blue, white)
- [ ] LED brightness control works
- [ ] Button short press detected
- [ ] Button long press detected (1+ second)
- [ ] Button double click detected
- [ ] Battery voltage reading is sensible (3.0-4.2V range)
- [ ] Power off works when not USB-connected
- [ ] SD card mounts reliably
- [ ] Microphone captures audio

### 7.2 Recording Tests

- [ ] Short recording (10 seconds) plays correctly
- [ ] Long recording (10 minutes) plays correctly
- [ ] Recording survives abrupt stop (button press)
- [ ] Multiple recordings create separate files
- [ ] File chunking works for long sessions
- [ ] No audio dropouts or glitches

### 7.3 Edge Cases

- [ ] Behaviour when SD card is full
- [ ] Behaviour when SD card is removed during recording
- [ ] Behaviour when battery is low
- [ ] Recovery after crash/reset
- [ ] Concurrent USB connection during recording

---

## Step 8: Debugging Tips

### 8.1 Serial Monitor

Always monitor serial output during development:

```fish
pio device monitor --filter colorize
```

### 8.2 Common Issues

| Symptom                | Likely Cause         | Solution                         |
| ---------------------- | -------------------- | -------------------------------- |
| Device won't turn on   | Power hold not set   | Check GPIO 46 init               |
| LED doesn't work       | Wrong pin or timing  | Verify GPIO 21, check RMT config |
| Button unresponsive    | Wrong pull resistor  | Use internal pull-up             |
| SD card fails randomly | Power brownout       | Add capacitor, check USB power   |
| Audio distorted        | Sample rate mismatch | Verify PDM clock configuration   |
| Recordings corrupt     | File not closed      | Always finalise header           |
| Battery reads 0        | ADC not calibrated   | Check voltage divider ratio      |

### 8.3 Memory Debugging

The ESP32-S3 without PSRAM is tight on RAM. Monitor usage:

```c
void print_memory_stats(void)
{
    ESP_LOGI(TAG, "Free heap: %d bytes", esp_get_free_heap_size());
    ESP_LOGI(TAG, "Min free heap: %d bytes", esp_get_minimum_free_heap_size());
    ESP_LOGI(TAG, "Free internal: %d bytes",
             heap_caps_get_free_size(MALLOC_CAP_INTERNAL));
}
```

Call this periodically to catch memory leaks.

---

## Checklist: Ready for Real Use

- [ ] All hardware functions work correctly
- [ ] Recording quality is acceptable
- [ ] Battery life is acceptable
- [ ] Device is reliable over extended use
- [ ] Error states are handled gracefully
- [ ] User can operate without referring to documentation
- [ ] Device survives being dropped (test with care)

---

## Next Steps

With the pendant segment complete, you might consider:

1. **Bluetooth transfer** — Sync recordings to a phone app
2. **WiFi upload** — Direct upload to your processing server
3. **VAD (Voice Activity Detection)** — Only record when speech detected
4. **Ultrasonic shutdown** — The feature from your ticket for TV/music detection
5. **Better battery life** — Dynamic frequency scaling, aggressive sleep

Each of these would be another phase of development, building on the foundation you now have.

---

## Resources

- [M5 Capsule Documentation](https://docs.m5stack.com/en/core/M5Capsule)
- [M5 Capsule Schematic](https://m5stack.oss-cn-shenzhen.aliyuncs.com/resource/docs/schematic/Core/K135%20V0.1.pdf)
- [ESP32-S3 Technical Reference](https://www.espressif.com/sites/default/files/documentation/esp32-s3_technical_reference_manual_en.pdf)
- [SPM1423 Microphone Datasheet](https://m5stack.oss-cn-shenzhen.aliyuncs.com/resource/docs/datasheet/core/SPM1423HM4H-B_datasheet_en.pdf)

---

Congratulations. You now have sufficient knowledge to make an M5 Capsule record audio to an SD card without bursting into flames. The pendant segment of your project is achievable. The rest of the components—Bluetooth proxy, ultrasonic shutdown, cloud processing—await their own learning journeys, but you've conquered the foundation.
