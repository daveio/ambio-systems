# Phase 2: SD Card Interface (SPI/SDMMC)

## Learning Objectives

By the end of this phase, you will:

- Understand the difference between SPI and SDMMC modes for SD cards
- Configure the ESP-IDF filesystem drivers correctly
- Mount, read, write, and unmount SD cards without corrupting them
- Handle the various ways SD cards can fail (and they will)
- Implement proper file operations suitable for continuous data logging

---

## Step 1: SD Card Communication Fundamentals

Before touching any code, you need to understand how microcontrollers talk to SD cards. There are two main protocols, and the ESP32-S3 supports both.

### 1.1 SPI Mode

The Serial Peripheral Interface mode uses four wires:

| Signal | Function                                      |
| ------ | --------------------------------------------- |
| MOSI   | Master Out, Slave In (data to card)           |
| MISO   | Master In, Slave Out (data from card)         |
| CLK    | Clock signal                                  |
| CS     | Chip Select (tells card it's being addressed) |

**Advantages:**

- Uses standard SPI peripheral
- Flexible pin assignment
- Works with almost any SD card

**Disadvantages:**

- Slower (typically maxes out around 25 MHz)
- Uses more GPIO pins

### 1.2 SDMMC Mode (SD Bus)

The native SD protocol uses a 1-bit or 4-bit parallel bus:

| Signal | 1-bit Mode | 4-bit Mode |
| ------ | ---------- | ---------- |
| CLK    | Required   | Required   |
| CMD    | Required   | Required   |
| DAT0   | Required   | Required   |
| DAT1   | —          | Required   |
| DAT2   | —          | Required   |
| DAT3   | —          | Required   |

**Advantages:**

- Much faster (up to 40 MHz with 4-bit width)
- More efficient for continuous streaming

**Disadvantages:**

- Fixed pins on ESP32-S3 (no remapping)
- Not all cards behave well in SDMMC mode

### 1.3 Which Mode for the M5 Capsule?

The M5 Capsule uses **SPI mode** for its SD card slot. The relevant pins are:

| Function | GPIO    |
| -------- | ------- |
| MOSI     | GPIO 14 |
| MISO     | GPIO 39 |
| CLK      | GPIO 12 |
| CS       | GPIO 11 |

These pins are specific to the M5 Capsule. Other M5 devices use different pins. Always check the documentation for your specific board.

---

## Step 2: Project Setup

### 2.1 Create a New Project

Create a new PlatformIO project called `pendant-sdcard`:

```ini
; platformio.ini
[env:m5capsule]
platform = espressif32
board = esp32-s3-devkitc-1
framework = espidf

monitor_speed = 115200
build_flags =
    -DCONFIG_ESP_CONSOLE_USB_SERIAL_JTAG=y

board_build.partitions = default.csv
board_build.flash_mode = dio
board_build.f_flash = 80000000L
board_build.f_cpu = 240000000L
```

### 2.2 Required ESP-IDF Components

The SD card functionality requires these components:

```c
#include "esp_vfs.h"
#include "esp_vfs_fat.h"
#include "sdmmc_cmd.h"
#include "driver/sdspi_host.h"
#include "driver/spi_common.h"
```

---

## Step 3: Basic SD Card Mounting

### 3.1 The Mount Configuration

Create `src/main.c`:

```c
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/unistd.h>
#include "esp_log.h"
#include "esp_vfs_fat.h"
#include "sdmmc_cmd.h"
#include "driver/sdspi_host.h"
#include "driver/spi_common.h"

static const char *TAG = "sdcard";

// M5 Capsule SD card pins
#define PIN_NUM_MOSI  14
#define PIN_NUM_MISO  39
#define PIN_NUM_CLK   12
#define PIN_NUM_CS    11

// Mount point - where the SD card appears in the filesystem
#define MOUNT_POINT "/sdcard"

// Handle for the SD card - needed for unmounting
static sdmmc_card_t *card;
static const char mount_point[] = MOUNT_POINT;

esp_err_t init_sd_card(void)
{
    esp_err_t ret;

    ESP_LOGI(TAG, "Initialising SD card in SPI mode");

    // Configuration for the FATFS
    esp_vfs_fat_sdmmc_mount_config_t mount_config = {
        .format_if_mount_failed = false,  // Don't format! Data loss!
        .max_files = 5,                   // Max open files
        .allocation_unit_size = 16 * 1024 // Cluster size (affects performance)
    };

    // SPI bus configuration
    spi_bus_config_t bus_cfg = {
        .mosi_io_num = PIN_NUM_MOSI,
        .miso_io_num = PIN_NUM_MISO,
        .sclk_io_num = PIN_NUM_CLK,
        .quadwp_io_num = -1,  // Not used
        .quadhd_io_num = -1,  // Not used
        .max_transfer_sz = 4000,
    };

    // Initialise the SPI bus
    // Using SPI2_HOST (also called HSPI) - SPI3 is often used for flash
    ret = spi_bus_initialize(SPI2_HOST, &bus_cfg, SDSPI_DEFAULT_DMA);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialise SPI bus: %s", esp_err_to_name(ret));
        return ret;
    }

    // SD card specific SPI device configuration
    sdspi_device_config_t slot_config = SDSPI_DEVICE_CONFIG_DEFAULT();
    slot_config.gpio_cs = PIN_NUM_CS;
    slot_config.host_id = SPI2_HOST;

    // Mount the filesystem
    ESP_LOGI(TAG, "Mounting filesystem");
    ret = esp_vfs_fat_sdspi_mount(mount_point, &bus_cfg, &slot_config, &mount_config, &card);

    if (ret != ESP_OK) {
        if (ret == ESP_FAIL) {
            ESP_LOGE(TAG, "Failed to mount filesystem. Check card is formatted as FAT32");
        } else {
            ESP_LOGE(TAG, "Failed to initialise card: %s", esp_err_to_name(ret));
            ESP_LOGE(TAG, "Make sure SD card is inserted");
        }
        spi_bus_free(SPI2_HOST);
        return ret;
    }

    ESP_LOGI(TAG, "Filesystem mounted");

    // Print card information
    sdmmc_card_print_info(stdout, card);

    return ESP_OK;
}

void deinit_sd_card(void)
{
    // Unmount filesystem
    esp_vfs_fat_sdcard_unmount(mount_point, card);
    ESP_LOGI(TAG, "Card unmounted");

    // Free SPI bus
    spi_bus_free(SPI2_HOST);
}
```

### 3.2 Understanding the Mount Process

The mounting process involves several layers:

1. **SPI Bus** — The physical communication channel
2. **SDSPI Driver** — Speaks the SD card protocol over SPI
3. **FATFS** — The filesystem implementation
4. **VFS** — Virtual File System that maps FATFS to POSIX calls

Once mounted, you can use standard C file operations (`fopen`, `fwrite`, `fclose`) on paths starting with `/sdcard/`.

---

## Step 4: File Operations

### 4.1 Writing a File

```c
esp_err_t write_test_file(void)
{
    ESP_LOGI(TAG, "Opening file for writing");

    // Standard C file operations work!
    FILE *f = fopen(MOUNT_POINT "/test.txt", "w");
    if (f == NULL) {
        ESP_LOGE(TAG, "Failed to open file for writing");
        return ESP_FAIL;
    }

    fprintf(f, "Hello from ESP32-S3!\n");
    fprintf(f, "This file was created at runtime.\n");
    fclose(f);

    ESP_LOGI(TAG, "File written successfully");
    return ESP_OK;
}
```

### 4.2 Reading a File

```c
esp_err_t read_test_file(void)
{
    ESP_LOGI(TAG, "Reading file");

    FILE *f = fopen(MOUNT_POINT "/test.txt", "r");
    if (f == NULL) {
        ESP_LOGE(TAG, "Failed to open file for reading");
        return ESP_FAIL;
    }

    char line[128];
    while (fgets(line, sizeof(line), f)) {
        // Remove newline for clean logging
        line[strcspn(line, "\n")] = 0;
        ESP_LOGI(TAG, "Read: %s", line);
    }
    fclose(f);

    return ESP_OK;
}
```

### 4.3 Appending to a File

For audio recording, you'll be appending data continuously:

```c
esp_err_t append_to_file(const char *filename, const uint8_t *data, size_t len)
{
    FILE *f = fopen(filename, "ab");  // "ab" = append, binary
    if (f == NULL) {
        ESP_LOGE(TAG, "Failed to open %s for appending", filename);
        return ESP_FAIL;
    }

    size_t written = fwrite(data, 1, len, f);
    fclose(f);

    if (written != len) {
        ESP_LOGE(TAG, "Write incomplete: %d of %d bytes", written, len);
        return ESP_FAIL;
    }

    return ESP_OK;
}
```

### 4.4 Checking File/Directory Existence

```c
bool file_exists(const char *path)
{
    struct stat st;
    return (stat(path, &st) == 0);
}

bool is_directory(const char *path)
{
    struct stat st;
    if (stat(path, &st) != 0) {
        return false;
    }
    return S_ISDIR(st.st_mode);
}
```

### 4.5 Getting File Size

```c
long get_file_size(const char *path)
{
    struct stat st;
    if (stat(path, &st) != 0) {
        return -1;
    }
    return st.st_size;
}
```

---

## Step 5: Listing Directory Contents

```c
#include <dirent.h>

esp_err_t list_directory(const char *path)
{
    DIR *dir = opendir(path);
    if (dir == NULL) {
        ESP_LOGE(TAG, "Failed to open directory: %s", path);
        return ESP_FAIL;
    }

    ESP_LOGI(TAG, "Contents of %s:", path);

    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        // d_type: DT_REG = file, DT_DIR = directory
        const char *type = (entry->d_type == DT_DIR) ? "DIR " : "FILE";
        ESP_LOGI(TAG, "  %s: %s", type, entry->d_name);
    }

    closedir(dir);
    return ESP_OK;
}
```

---

## Step 6: Complete Test Program

Here's a complete program that exercises all the SD card functionality:

```c
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <dirent.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_vfs_fat.h"
#include "sdmmc_cmd.h"
#include "driver/sdspi_host.h"
#include "driver/spi_common.h"

static const char *TAG = "sdcard-test";

#define PIN_NUM_MOSI  14
#define PIN_NUM_MISO  39
#define PIN_NUM_CLK   12
#define PIN_NUM_CS    11
#define MOUNT_POINT   "/sdcard"

static sdmmc_card_t *card;
static const char mount_point[] = MOUNT_POINT;

esp_err_t init_sd_card(void)
{
    esp_err_t ret;

    ESP_LOGI(TAG, "Initialising SD card");

    esp_vfs_fat_sdmmc_mount_config_t mount_config = {
        .format_if_mount_failed = false,
        .max_files = 5,
        .allocation_unit_size = 16 * 1024
    };

    spi_bus_config_t bus_cfg = {
        .mosi_io_num = PIN_NUM_MOSI,
        .miso_io_num = PIN_NUM_MISO,
        .sclk_io_num = PIN_NUM_CLK,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = 4000,
    };

    ret = spi_bus_initialize(SPI2_HOST, &bus_cfg, SDSPI_DEFAULT_DMA);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "SPI bus init failed: %s", esp_err_to_name(ret));
        return ret;
    }

    sdspi_device_config_t slot_config = SDSPI_DEVICE_CONFIG_DEFAULT();
    slot_config.gpio_cs = PIN_NUM_CS;
    slot_config.host_id = SPI2_HOST;

    ret = esp_vfs_fat_sdspi_mount(mount_point, &bus_cfg, &slot_config,
                                   &mount_config, &card);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Mount failed: %s", esp_err_to_name(ret));
        spi_bus_free(SPI2_HOST);
        return ret;
    }

    ESP_LOGI(TAG, "SD card mounted successfully");
    sdmmc_card_print_info(stdout, card);

    return ESP_OK;
}

void deinit_sd_card(void)
{
    esp_vfs_fat_sdcard_unmount(mount_point, card);
    spi_bus_free(SPI2_HOST);
    ESP_LOGI(TAG, "SD card unmounted");
}

void test_write_read(void)
{
    const char *test_file = MOUNT_POINT "/test.txt";

    // Write
    ESP_LOGI(TAG, "Writing to %s", test_file);
    FILE *f = fopen(test_file, "w");
    if (f == NULL) {
        ESP_LOGE(TAG, "Failed to open file for writing");
        return;
    }
    fprintf(f, "Test line 1\n");
    fprintf(f, "Test line 2\n");
    fprintf(f, "Test line 3\n");
    fclose(f);
    ESP_LOGI(TAG, "Write complete");

    // Read back
    ESP_LOGI(TAG, "Reading from %s", test_file);
    f = fopen(test_file, "r");
    if (f == NULL) {
        ESP_LOGE(TAG, "Failed to open file for reading");
        return;
    }
    char line[64];
    while (fgets(line, sizeof(line), f)) {
        line[strcspn(line, "\n")] = 0;  // Remove newline
        ESP_LOGI(TAG, "  > %s", line);
    }
    fclose(f);
}

void test_binary_write(void)
{
    const char *bin_file = MOUNT_POINT "/binary.dat";

    // Create test data
    uint8_t test_data[256];
    for (int i = 0; i < 256; i++) {
        test_data[i] = i;
    }

    // Write binary data
    ESP_LOGI(TAG, "Writing binary data to %s", bin_file);
    FILE *f = fopen(bin_file, "wb");
    if (f == NULL) {
        ESP_LOGE(TAG, "Failed to open file for binary writing");
        return;
    }
    size_t written = fwrite(test_data, 1, sizeof(test_data), f);
    fclose(f);
    ESP_LOGI(TAG, "Wrote %d bytes", written);

    // Read back and verify
    ESP_LOGI(TAG, "Reading back binary data");
    f = fopen(bin_file, "rb");
    if (f == NULL) {
        ESP_LOGE(TAG, "Failed to open file for binary reading");
        return;
    }
    uint8_t read_data[256];
    size_t read = fread(read_data, 1, sizeof(read_data), f);
    fclose(f);

    // Verify
    if (read != sizeof(test_data)) {
        ESP_LOGE(TAG, "Size mismatch: read %d, expected %d", read, sizeof(test_data));
        return;
    }

    bool match = (memcmp(test_data, read_data, sizeof(test_data)) == 0);
    ESP_LOGI(TAG, "Data verification: %s", match ? "PASSED" : "FAILED");
}

void test_append(void)
{
    const char *append_file = MOUNT_POINT "/append.txt";

    // Delete if exists
    remove(append_file);

    // Append multiple times
    for (int i = 0; i < 5; i++) {
        FILE *f = fopen(append_file, "a");
        if (f == NULL) {
            ESP_LOGE(TAG, "Failed to open file for appending");
            return;
        }
        fprintf(f, "Append iteration %d\n", i);
        fclose(f);
        ESP_LOGI(TAG, "Appended iteration %d", i);
        vTaskDelay(pdMS_TO_TICKS(100));
    }

    // Read final result
    ESP_LOGI(TAG, "Final file contents:");
    FILE *f = fopen(append_file, "r");
    if (f) {
        char line[64];
        while (fgets(line, sizeof(line), f)) {
            line[strcspn(line, "\n")] = 0;
            ESP_LOGI(TAG, "  > %s", line);
        }
        fclose(f);
    }
}

void list_root(void)
{
    DIR *dir = opendir(MOUNT_POINT);
    if (dir == NULL) {
        ESP_LOGE(TAG, "Failed to open root directory");
        return;
    }

    ESP_LOGI(TAG, "SD card contents:");
    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        char full_path[128];
        snprintf(full_path, sizeof(full_path), "%s/%s", MOUNT_POINT, entry->d_name);

        struct stat st;
        stat(full_path, &st);

        if (entry->d_type == DT_DIR) {
            ESP_LOGI(TAG, "  [DIR]  %s", entry->d_name);
        } else {
            ESP_LOGI(TAG, "  [FILE] %s (%ld bytes)", entry->d_name, st.st_size);
        }
    }
    closedir(dir);
}

void app_main(void)
{
    ESP_LOGI(TAG, "SD Card Test Starting");

    if (init_sd_card() != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialise SD card. Halting.");
        return;
    }

    // Run tests
    list_root();
    test_write_read();
    test_binary_write();
    test_append();
    list_root();

    // Clean up
    deinit_sd_card();

    ESP_LOGI(TAG, "All tests complete");
}
```

---

## Step 7: Performance Considerations

For audio recording, you'll be writing continuously. Performance matters.

### 7.1 Buffer Sizes

Larger writes are more efficient than many small writes:

```c
// Bad: Many small writes
for (int i = 0; i < 1000; i++) {
    fwrite(&byte, 1, 1, f);  // 1000 writes of 1 byte
}

// Good: One large write
fwrite(buffer, 1, 1000, f);  // 1 write of 1000 bytes
```

### 7.2 SPI Clock Speed

You can increase the SPI clock for better performance:

```c
sdspi_device_config_t slot_config = SDSPI_DEVICE_CONFIG_DEFAULT();
slot_config.gpio_cs = PIN_NUM_CS;
slot_config.host_id = SPI2_HOST;
// Default is 20 MHz, can try higher with good cards
// Note: Some cards won't work above 25 MHz in SPI mode
```

### 7.3 Allocation Unit Size

The `allocation_unit_size` in the mount config affects write performance:

```c
esp_vfs_fat_sdmmc_mount_config_t mount_config = {
    // ...
    .allocation_unit_size = 64 * 1024  // Larger = better sequential write
};
```

Larger allocation units improve sequential write performance but waste space for small files.

### 7.4 fflush Frequency

Every `fflush()` or `fclose()` forces data to the card. For continuous recording:

```c
// Write data in large chunks
// Only flush periodically (e.g., every second or every N kilobytes)
// This balances data safety with performance
```

---

## Step 8: Error Handling

SD cards fail in creative ways. Handle them gracefully.

### 8.1 Card Removal Detection

```c
// Check if card is still present
bool card_present(void)
{
    // Try a simple file operation
    FILE *f = fopen(MOUNT_POINT "/.probe", "w");
    if (f == NULL) {
        return false;
    }
    fclose(f);
    remove(MOUNT_POINT "/.probe");
    return true;
}
```

### 8.2 Disk Full Handling

```c
#include "esp_vfs_fat.h"

void check_disk_space(void)
{
    FATFS *fs;
    DWORD free_clusters;

    if (f_getfree(MOUNT_POINT, &free_clusters, &fs) == FR_OK) {
        uint64_t total_sectors = (fs->n_fatent - 2) * fs->csize;
        uint64_t free_sectors = free_clusters * fs->csize;

        uint64_t total_kb = total_sectors * 512 / 1024;
        uint64_t free_kb = free_sectors * 512 / 1024;

        ESP_LOGI(TAG, "Disk space: %llu KB free of %llu KB", free_kb, total_kb);
    }
}
```

### 8.3 Graceful Degradation

```c
esp_err_t safe_write(const char *path, const uint8_t *data, size_t len)
{
    FILE *f = fopen(path, "ab");
    if (f == NULL) {
        ESP_LOGE(TAG, "Cannot open file - card may be removed or full");
        return ESP_FAIL;
    }

    size_t written = fwrite(data, 1, len, f);
    int flush_result = fflush(f);
    fclose(f);

    if (written != len) {
        ESP_LOGE(TAG, "Partial write: %d of %d bytes (disk full?)", written, len);
        return ESP_ERR_NO_MEM;
    }

    if (flush_result != 0) {
        ESP_LOGE(TAG, "Flush failed - write may be lost");
        return ESP_FAIL;
    }

    return ESP_OK;
}
```

---

## Checklist Before Proceeding

- [ ] SD card mounts successfully
- [ ] Can write and read text files
- [ ] Can write and read binary files
- [ ] Append operations work correctly
- [ ] Can list directory contents
- [ ] Understand performance implications
- [ ] Card unmounts cleanly

---

## Common Problems and Solutions

| Problem                            | Solution                                                             |
| ---------------------------------- | -------------------------------------------------------------------- |
| Mount fails with ESP_FAIL          | Card not formatted as FAT32, or not inserted properly                |
| Mount fails with timeout           | Wrong pins, bad wiring, or card not making contact                   |
| Writes succeed but data is garbled | Clock too fast for card - reduce SPI speed                           |
| "No more file handles"             | Increase `max_files` in mount config, or close files properly        |
| Corruption after power loss        | Always `fflush()` after critical writes, unmount before power off    |
| Very slow writes                   | Use larger buffers, increase allocation unit size, use Class 10 card |

---

## Recommended SD Cards

For audio recording, use cards that can sustain continuous writes:

- **Minimum:** Class 10 / UHS-I
- **Recommended:** UHS-I U1 or better
- **Avoid:** Ancient cards from 2008, "Class 4" or unmarked cards

A 16 kHz, 16-bit mono audio stream is about 32 KB/s. Any modern card handles this easily, but cheap cards have high latency spikes that cause buffer overruns.

---

## Next Phase

With SD card operations mastered, proceed to **Phase 3: I2S Audio Capture**, where we convince a microphone to talk to us.
