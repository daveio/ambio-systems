# Phase 1: Environment & ESP32-S3 Fundamentals

## Learning Objectives

By the end of this phase, you will:

- Have a working PlatformIO + ESP-IDF development environment
- Understand the ESP32-S3's architecture and how it differs from its ancestors
- Grasp FreeRTOS fundamentals sufficient to not shoot yourself in the foot
- Successfully upload code to the M5 Capsule and prove it works

---

## Step 1: Install PlatformIO

PlatformIO is essentially a civilised wrapper around the various embedded toolchains that would otherwise require you to sacrifice a goat during a full moon to configure correctly.

### 1.1 VS Code Extension (Recommended)

1. Install [Visual Studio Code](https://code.visualstudio.com/) if you haven't already succumbed
2. Open the Extensions panel (`Ctrl+Shift+X` / `Cmd+Shift+X`)
3. Search for "PlatformIO IDE"
4. Click Install
5. Wait approximately forever while it downloads half the internet
6. Restart VS Code when prompted

### 1.2 Verify Installation

Open a terminal in VS Code and run:

```fish
pio --version
```

You should see something like `PlatformIO Core, version 6.x.x`. If you see an error, close VS Code, make a cup of tea, reopen it, and try again. This solves approximately 60% of all software problems.

---

## Step 2: Create Your First Project

### 2.1 Project Initialisation

1. Click the PlatformIO icon in the VS Code sidebar (it looks like an alien head, or possibly a house—the designers were unclear)
2. Click "Create New Project"
3. Configure as follows:
   - **Name:** `pendant-blink` (we're starting simple)
   - **Board:** `M5Stack-StampS3` (the Capsule uses the StampS3 module)
   - **Framework:** `ESP-IDF`
4. Click "Finish" and wait while PlatformIO downloads the ESP-IDF framework

> **Note:** If `M5Stack-StampS3` isn't available, use `esp32-s3-devkitc-1` and we'll adjust the configuration manually.

### 2.2 Understanding the Project Structure

PlatformIO creates this structure:

```
pendant-blink/
├── include/          # Header files go here
├── lib/              # Project-specific libraries
├── src/              # Your source code
│   └── main.c        # Entry point
├── test/             # Unit tests (optimistic, I know)
├── platformio.ini    # Project configuration
└── sdkconfig.defaults # ESP-IDF configuration overrides
```

### 2.3 Configure platformio.ini

Open `platformio.ini` and replace its contents with:

```ini
[env:m5capsule]
platform = espressif32
board = esp32-s3-devkitc-1
framework = espidf

; USB CDC for serial output (the Capsule uses USB, not UART)
monitor_speed = 115200
build_flags =
    -DCONFIG_ESP_CONSOLE_USB_SERIAL_JTAG=y

; Upload via USB
upload_protocol = esptool
upload_port = /dev/ttyACM0  ; Adjust for your system

; ESP-IDF specific
board_build.partitions = default.csv
board_build.flash_mode = dio
board_build.f_flash = 80000000L
board_build.f_cpu = 240000000L
```

> **Platform-specific upload ports:**
>
> - Linux: `/dev/ttyACM0` or `/dev/ttyUSB0`
> - macOS: `/dev/cu.usbmodem*`
> - Windows: `COM3` or similar (check Device Manager)

---

## Step 3: The Obligatory Blink

Every embedded journey begins with blinking an LED. It's tradition. The M5 Capsule has an RGB LED on GPIO 21.

### 3.1 Create the Main File

Replace the contents of `src/main.c` with:

```c
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_log.h"

// M5 Capsule RGB LED is on GPIO 21
// It's active LOW (because of course it is)
#define LED_GPIO 21

static const char *TAG = "blink";

void app_main(void)
{
    ESP_LOGI(TAG, "Initialising LED on GPIO %d", LED_GPIO);

    // Configure GPIO
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << LED_GPIO),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    gpio_config(&io_conf);

    ESP_LOGI(TAG, "Starting blink loop");

    int level = 0;
    while (1) {
        gpio_set_level(LED_GPIO, level);
        level = !level;

        // FreeRTOS delay - this is NOT the same as a busy wait
        // It yields the CPU to other tasks
        vTaskDelay(pdMS_TO_TICKS(500));
    }
}
```

### 3.2 Build and Upload

1. Connect the M5 Capsule via USB-C
2. Put it in download mode:
   - Hold the side button
   - Press and release the reset button (or reconnect USB)
   - Release the side button after a second
3. Click the PlatformIO "Upload" button (arrow icon) or run:

```fish
pio run -t upload
```

### 3.3 Monitor Serial Output

```fish
pio device monitor
```

You should see log output including "Starting blink loop", and the LED should blink. If it doesn't, welcome to embedded development. Check your connections, upload port, and whether Mercury is in retrograde.

---

## Step 4: Understanding the ESP32-S3

The ESP32-S3 is the third generation of Espressif's ESP32 line, and it has some characteristics worth understanding before you start doing anything clever.

### 4.1 Key Specifications

| Feature   | ESP32-S3                              |
| --------- | ------------------------------------- |
| CPU       | Dual-core Xtensa LX7 @ 240 MHz        |
| RAM       | 512 KB SRAM + 8 MB PSRAM (on Capsule) |
| Flash     | 8 MB (on Capsule)                     |
| WiFi      | 802.11 b/g/n                          |
| Bluetooth | BLE 5.0                               |
| USB       | Native USB-OTG                        |
| I2S       | 2x I2S controllers                    |

### 4.2 What Makes the S3 Different

1. **Native USB** — Unlike earlier ESP32s that needed a USB-to-UART chip, the S3 has built-in USB. This is why programming and serial monitoring work differently.

2. **Vector instructions** — The S3 has vector extensions for AI/ML workloads. Probably overkill for audio capture, but nice to have.

3. **More I2S flexibility** — The S3's I2S peripheral supports more configurations, including better PDM microphone support.

4. **Different GPIO mapping** — Pin functions aren't in the same places as ESP32 or ESP32-S2. Always check the S3-specific documentation.

### 4.3 Memory Architecture

The ESP32-S3 has a somewhat baroque memory layout:

- **IRAM** (Instruction RAM): Fast, for interrupt handlers and performance-critical code
- **DRAM** (Data RAM): General-purpose, where your variables live
- **PSRAM** (if equipped): External SPI RAM, slower but much larger
- **Flash**: Where your code and read-only data live

For audio work, you'll likely use PSRAM for large buffers. The internal SRAM is too precious.

---

## Step 5: FreeRTOS Fundamentals

ESP-IDF is built on FreeRTOS. You cannot escape it. Fortunately, you only need to understand a few concepts.

### 5.1 Tasks

A task is FreeRTOS's unit of concurrent execution. Each task has its own stack and runs (from its perspective) independently.

```c
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

// Task function signature
void my_task(void *pvParameters)
{
    while (1) {
        // Do something
        vTaskDelay(pdMS_TO_TICKS(100));  // ALWAYS delay or yield!
    }
}

// Create the task (typically in app_main)
void app_main(void)
{
    xTaskCreate(
        my_task,        // Function to run
        "my_task",      // Name (for debugging)
        4096,           // Stack size in bytes
        NULL,           // Parameter to pass
        5,              // Priority (higher = more important)
        NULL            // Handle (optional, for controlling the task)
    );
}
```

> **Critical:** Tasks MUST yield control periodically via `vTaskDelay()`, `vTaskYield()`, or blocking on a synchronisation primitive. A task that never yields will starve everything else, including the watchdog timer, which will then reset your device.

### 5.2 Queues

Queues are how tasks communicate safely. They're thread-safe FIFO buffers.

```c
#include "freertos/queue.h"

// Create a queue that holds 10 integers
QueueHandle_t my_queue = xQueueCreate(10, sizeof(int));

// Send to queue (from producer task)
int data = 42;
xQueueSend(my_queue, &data, portMAX_DELAY);

// Receive from queue (from consumer task)
int received;
if (xQueueReceive(my_queue, &received, portMAX_DELAY) == pdTRUE) {
    // Got data
}
```

For audio, you'll use queues to pass buffer pointers between the capture task and the SD-writing task.

### 5.3 Semaphores and Mutexes

When multiple tasks access shared resources (like a file handle), you need synchronisation.

```c
#include "freertos/semphr.h"

// Create a mutex
SemaphoreHandle_t my_mutex = xSemaphoreCreateMutex();

// Use the mutex
if (xSemaphoreTake(my_mutex, portMAX_DELAY) == pdTRUE) {
    // Critical section - only one task at a time here
    // ... do things with shared resource ...
    xSemaphoreGive(my_mutex);
}
```

### 5.4 Practical Exercise: Dual-Task Blink

Modify your blink program to use two tasks—one that controls the LED, another that logs messages. This proves you understand task creation:

```c
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_log.h"

#define LED_GPIO 21
static const char *TAG = "dual-task";

// Shared state (in practice, protect with mutex)
static volatile int blink_count = 0;

void blink_task(void *pvParameters)
{
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << LED_GPIO),
        .mode = GPIO_MODE_OUTPUT,
    };
    gpio_config(&io_conf);

    int level = 0;
    while (1) {
        gpio_set_level(LED_GPIO, level);
        level = !level;
        blink_count++;
        vTaskDelay(pdMS_TO_TICKS(500));
    }
}

void logger_task(void *pvParameters)
{
    while (1) {
        ESP_LOGI(TAG, "Blink count: %d", blink_count);
        vTaskDelay(pdMS_TO_TICKS(2000));
    }
}

void app_main(void)
{
    ESP_LOGI(TAG, "Starting dual-task demo");

    // Create tasks on different cores for extra smugness
    xTaskCreatePinnedToCore(blink_task, "blink", 2048, NULL, 5, NULL, 0);
    xTaskCreatePinnedToCore(logger_task, "logger", 4096, NULL, 4, NULL, 1);
}
```

---

## Step 6: ESP-IDF Logging

The ESP-IDF logging system is your primary debugging tool. Learn to love it.

### 6.1 Log Levels

```c
#include "esp_log.h"

static const char *TAG = "my_component";

ESP_LOGE(TAG, "Error: %s", "something broke");     // Red, always shown
ESP_LOGW(TAG, "Warning: %d", some_value);          // Yellow
ESP_LOGI(TAG, "Info: starting up");                // Green, default level
ESP_LOGD(TAG, "Debug: buffer at %p", ptr);         // Not shown by default
ESP_LOGV(TAG, "Verbose: byte 0x%02X", byte);       // Definitely not shown
```

### 6.2 Changing Log Levels

In `platformio.ini`:

```ini
build_flags =
    -DLOG_LOCAL_LEVEL=ESP_LOG_DEBUG
```

Or at runtime:

```c
esp_log_level_set("my_component", ESP_LOG_DEBUG);
esp_log_level_set("*", ESP_LOG_WARN);  // Everything else quieter
```

---

## Checklist Before Proceeding

Before moving to Phase 2, ensure you can tick all these boxes:

- [ ] PlatformIO installed and working
- [ ] Successfully built and uploaded code to M5 Capsule
- [ ] LED blinks
- [ ] Serial monitor shows log output
- [ ] Understand what a FreeRTOS task is
- [ ] Understand what a queue is for
- [ ] Can create and run multiple tasks

---

## Common Problems and Solutions

| Problem                            | Solution                                                                                                  |
| ---------------------------------- | --------------------------------------------------------------------------------------------------------- |
| Upload fails with "No serial port" | Check USB connection, try different cable (data cables vs charge-only), ensure device is in download mode |
| Upload fails with timeout          | Hold button longer when entering download mode                                                            |
| No serial output                   | Ensure `CONFIG_ESP_CONSOLE_USB_SERIAL_JTAG=y` is set, or try USB_CDC settings                             |
| Device resets repeatedly           | Watchdog timeout—make sure tasks yield via `vTaskDelay()`                                                 |
| "Brown out" resets                 | Poor USB power—try a different port or powered hub                                                        |

---

## Resources

- [ESP-IDF Programming Guide (ESP32-S3)](https://docs.espressif.com/projects/esp-idf/en/latest/esp32s3/)
- [FreeRTOS Documentation](https://www.freertos.org/Documentation/RTOS_book.html)
- [M5 Capsule Documentation](https://docs.m5stack.com/en/core/M5Capsule)
- [PlatformIO ESP-IDF Documentation](https://docs.platformio.org/en/latest/frameworks/espidf.html)

---

## Next Phase

Once you're comfortable with the environment and can make tasks do your bidding, proceed to **Phase 2: SD Card Interface**, where we'll learn to write files without corrupting everything.
