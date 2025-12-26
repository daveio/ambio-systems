#if defined(ARDUINO)

#include <Arduino.h>

// If you use SD card, write this.
#include <SD.h>

#if __has_include(<LittleFS.h>)
// If you use LittleFS, write this.
//  #include <LittleFS.h>
#endif

#if __has_include(<SPIFFS.h>)
// If you use SPIFFS, write this.
//  #include <SPIFFS.h>
#endif

#endif

// * The filesystem header must be included before the display library.

//----------------------------------------------------------------

// If you use ATOM Display, write this.
#include <M5AtomDisplay.h>

// If you use Module Display, write this.
#include <M5ModuleDisplay.h>

// If you use Module RCA, write this.
#include <M5ModuleRCA.h>

// If you use Unit GLASS, write this.
#include <M5UnitGLASS.h>

// If you use Unit GLASS2, write this.
#include <M5UnitGLASS2.h>

// If you use Unit OLED, write this.
#include <M5UnitOLED.h>

// If you use Unit Mini OLED, write this.
#include <M5UnitMiniOLED.h>

// If you use Unit LCD, write this.
#include <M5UnitLCD.h>

// If you use UnitRCA (for Video output), write this.
#include <M5UnitRCA.h>

// * The display header must be included before the M5Unified library.

//----------------------------------------------------------------

// Include this to enable the M5 global instance.
#include <M5Unified.h>

extern const uint8_t wav_8bit_44100[2];

void setup(void)
{
  /// You may output logs to standard output.
  M5_LOGE("this is error LOG");
  M5_LOGW("this is warning LOG");
  M5_LOGI("this is info LOG");
  M5_LOGD("this is debug LOG");
  M5_LOGV("this is verbose LOG");

  auto cfg = M5.config();

#if defined(ARDUINO)
  cfg.serial_baudrate = 115200; // default=115200. if "Serial" is not needed, set it to 0.
#endif
  cfg.clear_display = true; // default=true. clear the screen when begin.
  cfg.output_power = true;  // default=true. use external port 5V output.
  cfg.internal_imu = true;  // default=true. use internal IMU.
  cfg.internal_rtc = true;  // default=true. use internal RTC.
  cfg.internal_spk = true;  // default=true. use internal speaker.
  cfg.internal_mic = true;  // default=true. use internal microphone.
  cfg.external_imu = true;  // default=false. use Unit Accel & Gyro.
  cfg.external_rtc = true;  // default=false. use Unit RTC.
  cfg.led_brightness = 255; // default= 0. system LED brightness (0=off / 255=max) (※ not NeoPixel)

  // external speaker setting.
  cfg.external_speaker.module_display = true; // default=false. use ModuleDisplay AudioOutput
  cfg.external_speaker.hat_spk = true;        // default=false. use HAT SPK
  cfg.external_speaker.hat_spk2 = true;       // default=false. use HAT SPK2
  cfg.external_speaker.atomic_spk = true;     // default=false. use ATOMIC SPK
  cfg.external_speaker.atomic_echo = true;    // default=false. use ATOMIC ECHO BASE
  cfg.external_speaker.module_rca = false;    // default=false. use ModuleRCA AudioOutput

  // external display setting. (Pre-include required)
  cfg.external_display.module_display = true;  // default=true. use ModuleDisplay
  cfg.external_display.atom_display = true;    // default=true. use AtomDisplay
  cfg.external_display.unit_glass = false;     // default=true. use UnitGLASS
  cfg.external_display.unit_glass2 = false;    // default=true. use UnitGLASS2
  cfg.external_display.unit_oled = true;       // default=true. use UnitOLED
  cfg.external_display.unit_mini_oled = false; // default=true. use UnitMiniOLED
  cfg.external_display.unit_lcd = false;       // default=true. use UnitLCD
  cfg.external_display.unit_rca = false;       // default=true. use UnitRCA VideoOutput
  cfg.external_display.module_rca = false;     // default=true. use ModuleRCA VideoOutput
/*
※ Unit OLED, Unit Mini OLED, Unit GLASS2 cannot be distinguished at runtime and may be misidentified as each other.

※ Display with auto-detection
 - module_display
 - atom_display
 - unit_glass
 - unit_glass2
 - unit_oled
 - unit_mini_oled
 - unit_lcd

※ Displays that cannot be auto-detected
 - module_rca
 - unit_rca

※ Note that if you enable a display that cannot be auto-detected,
   it will operate as if it were connected, even if it is not actually connected.
   When RCA is enabled, it consumes a lot of memory to allocate the frame buffer.
//*/

// Set individual parameters for external displays.
// (※ Use only the items you wish to change. Basically, it can be omitted.)
#if defined(__M5GFX_M5ATOMDISPLAY__) // setting for ATOM Display.
// cfg.atom_display.logical_width  = 1280;
// cfg.atom_display.logical_height = 720;
// cfg.atom_display.refresh_rate   = 60;
#endif
#if defined(__M5GFX_M5MODULEDISPLAY__) // setting for Module Display.
// cfg.module_display.logical_width  = 1280;
// cfg.module_display.logical_height = 720;
// cfg.module_display.refresh_rate   = 60;
#endif
#if defined(__M5GFX_M5MODULERCA__) // setting for Module RCA.
// cfg.module_rca.logical_width  = 216;
// cfg.module_rca.logical_height = 144;
// cfg.module_rca.signal_type    = M5ModuleRCA::signal_type_t::PAL;
// cfg.module_rca.use_psram      = M5ModuleRCA::use_psram_t::psram_use;
#endif
#if defined(__M5GFX_M5UNITRCA__) // setting for Unit RCA.
// cfg.unit_rca.logical_width  = 216;
// cfg.unit_rca.logical_height = 144;
// cfg.unit_rca.signal_type    = M5UnitRCA::signal_type_t::PAL;
// cfg.unit_rca.use_psram      = M5UnitRCA::use_psram_t::psram_use;
#endif
#if defined(__M5GFX_M5UNITGLASS__) // setting for Unit GLASS.
// cfg.unit_glass.pin_sda  = GPIO_NUM_21;
// cfg.unit_glass.pin_scl  = GPIO_NUM_22;
// cfg.unit_glass.i2c_addr = 0x3D;
// cfg.unit_glass.i2c_freq = 400000;
// cfg.unit_glass.i2c_port = I2C_NUM_0;
#endif
#if defined(__M5GFX_M5UNITGLASS2__) // setting for Unit GLASS2.
// cfg.unit_glass2.pin_sda  = GPIO_NUM_21;
// cfg.unit_glass2.pin_scl  = GPIO_NUM_22;
// cfg.unit_glass2.i2c_addr = 0x3C;
// cfg.unit_glass2.i2c_freq = 400000;
// cfg.unit_glass2.i2c_port = I2C_NUM_0;
#endif
#if defined(__M5GFX_M5UNITOLED__) // setting for Unit OLED.
// cfg.unit_oled.pin_sda  = GPIO_NUM_21;
// cfg.unit_oled.pin_scl  = GPIO_NUM_22;
// cfg.unit_oled.i2c_addr = 0x3C;
// cfg.unit_oled.i2c_freq = 400000;
// cfg.unit_oled.i2c_port = I2C_NUM_0;
#endif
#if defined(__M5GFX_M5UNITMINIOLED__) // setting for Unit Mini OLED.
// cfg.unit_mini_oled.pin_sda  = GPIO_NUM_21;
// cfg.unit_mini_oled.pin_scl  = GPIO_NUM_22;
// cfg.unit_mini_oled.i2c_addr = 0x3C;
// cfg.unit_mini_oled.i2c_freq = 400000;
// cfg.unit_mini_oled.i2c_port = I2C_NUM_0;
#endif
#if defined(__M5GFX_M5UNITLCD__) // setting for Unit LCD.
// cfg.unit_lcd.pin_sda  = GPIO_NUM_21;
// cfg.unit_lcd.pin_scl  = GPIO_NUM_22;
// cfg.unit_lcd.i2c_addr = 0x3E;
// cfg.unit_lcd.i2c_freq = 400000;
// cfg.unit_lcd.i2c_port = I2C_NUM_0;
#endif

  // begin M5Unified.
  M5.begin(cfg);

  // If an external display is to be used as the main display, it can be listed in order of priority.
  M5.setPrimaryDisplayType({
      //    m5::board_t::board_M5ModuleDisplay,
      //    m5::board_t::board_M5AtomDisplay,
      //    m5::board_t::board_M5ModuleRCA,
      //    m5::board_t::board_M5UnitGLASS,
      //    m5::board_t::board_M5UnitGLASS2,
      //    m5::board_t::board_M5UnitMiniOLED,
      m5::board_t::board_M5UnitOLED,
      //    m5::board_t::board_M5UnitLCD,
      //    m5::board_t::board_M5UnitRCA,
  });

  if (M5.Speaker.isEnabled())
  {
    /// set master volume (0~255)
    M5.Speaker.setVolume(64);

    /// play beep sound 2000Hz 100msec (background task)
    M5.Speaker.tone(2000, 100);

    /// wait done
    while (M5.Speaker.isPlaying())
    {
      M5.delay(1);
    }

    /// play beep sound 1000Hz 100msec (background task)
    M5.Speaker.tone(1000, 100);

    /// wait play beep sound 2000Hz 100msec (background task)
    while (M5.Speaker.isPlaying())
    {
      M5.delay(1);
    }

    M5.Speaker.playRaw(wav_8bit_44100, sizeof(wav_8bit_44100), 44100, false);
  }

  if (M5.Rtc.isEnabled())
  {
    // It is recommended to set UTC for the RTC and ESP32 internal clocks.
    //  rtc direct setting.    YYYY  MM  DD      hh  mm  ss
    //  M5.Rtc.setDateTime( {{ 2021, 12, 31 }, { 12, 34, 56 }} );
  }

  /// For models with EPD : refresh control
  M5.Display.setEpdMode(epd_mode_t::epd_fastest); // fastest but very-low quality.
                                                  // M5.Display.setEpdMode(epd_mode_t::epd_fast   ); // fast but low quality.
  // M5.Display.setEpdMode(epd_mode_t::epd_text   ); // slow but high quality. (for text)
  // M5.Display.setEpdMode(epd_mode_t::epd_quality); // slow but high quality. (for image)

  /// For models with LCD : backlight control (0~255)
  M5.Display.setBrightness(128);

  if (M5.Display.width() < M5.Display.height())
  { /// Landscape mode.
    M5.Display.setRotation(M5.Display.getRotation() ^ 1);
  }

  // multi display.
  size_t display_count = M5.getDisplayCount();
  for (int i = 0; i < display_count; ++i)
  {
    M5.Displays(i).startWrite();
    for (int y = 0; y < 128; ++y)
    {
      for (int x = 0; x < 128; ++x)
      {
        M5.Displays(i).writePixel(x, y, M5.Display.color888(x * 2, x + y, y * 2));
      }
    }
    M5.Displays(i).printf("Display %d\n", i);
    M5.Displays(i).endWrite();
  }

  int textsize = M5.Display.height() / 160;
  if (textsize == 0)
  {
    textsize = 1;
  }
  M5.Display.setTextSize(textsize);

  // run-time branch : hardware model check
  const char *name;
  switch (M5.getBoard())
  {
#if defined(CONFIG_IDF_TARGET_ESP32S3)
  case m5::board_t::board_M5StackCoreS3:
    name = "StackCoreS3";
    break;
  case m5::board_t::board_M5StackCoreS3SE:
    name = "StackCoreS3SE";
    break;
  case m5::board_t::board_M5StampS3:
    name = "StampS3";
    break;
  case m5::board_t::board_M5AtomS3U:
    name = "ATOMS3U";
    break;
  case m5::board_t::board_M5AtomS3Lite:
    name = "ATOMS3Lite";
    break;
  case m5::board_t::board_M5AtomS3:
    name = "ATOMS3";
    break;
  case m5::board_t::board_M5AtomS3R:
    name = "ATOMS3R";
    break;
  case m5::board_t::board_M5AtomS3RCam:
    name = "ATOMS3R Camera";
    break;
  case m5::board_t::board_M5AtomS3RExt:
    name = "ATOMS3R Ext";
    break;
  case m5::board_t::board_M5AtomEchoS3R:
    name = "ATOM ECHO S3R";
    break;
  case m5::board_t::board_M5Dial:
    name = "Dial";
    break;
  case m5::board_t::board_M5DinMeter:
    name = "DinMeter";
    break;
  case m5::board_t::board_M5Capsule:
    name = "Capsule";
    break;
  case m5::board_t::board_M5Cardputer:
    name = "Cardputer";
    break;
  case m5::board_t::board_M5CardputerADV:
    name = "CardputerADV";
    break;
  case m5::board_t::board_M5VAMeter:
    name = "VAMeter";
    break;
  case m5::board_t::board_M5PaperS3:
    name = "PaperS3";
    break;
  case m5::board_t::board_M5PowerHub:
    name = "PowerHub";
    break;
#elif defined(CONFIG_IDF_TARGET_ESP32C3)
  case m5::board_t::board_M5StampC3:
    name = "StampC3";
    break;
  case m5::board_t::board_M5StampC3U:
    name = "StampC3U";
    break;
#elif defined(CONFIG_IDF_TARGET_ESP32C6)
  case m5::board_t::board_M5NanoC6:
    name = "NanoC6";
    break;
  case m5::board_t::board_M5UnitC6L:
    name = "UnitC6L";
    break;
  case m5::board_t::board_ArduinoNessoN1:
    name = "NessoN1";
    break;
#elif defined(CONFIG_IDF_TARGET_ESP32P4)
  case m5::board_t::board_M5Tab5:
    name = "Tab5";
    break;
#else
  case m5::board_t::board_M5Stack:
    name = "Stack";
    break;
  case m5::board_t::board_M5StackCore2:
    name = "StackCore2";
    break;
  case m5::board_t::board_M5StickC:
    name = "StickC";
    break;
  case m5::board_t::board_M5StickCPlus:
    name = "StickCPlus";
    break;
  case m5::board_t::board_M5StickCPlus2:
    name = "StickCPlus2";
    break;
  case m5::board_t::board_M5StackCoreInk:
    name = "CoreInk";
    break;
  case m5::board_t::board_M5Paper:
    name = "Paper";
    break;
  case m5::board_t::board_M5Tough:
    name = "Tough";
    break;
  case m5::board_t::board_M5Station:
    name = "Station";
    break;
  case m5::board_t::board_M5AtomLite:
    name = "ATOM Lite";
    break;
  case m5::board_t::board_M5AtomMatrix:
    name = "ATOM Matrix";
    break;
  case m5::board_t::board_M5AtomEcho:
    name = "ATOM ECHO";
    break;
  case m5::board_t::board_M5AtomPsram:
    name = "ATOM PSRAM";
    break;
  case m5::board_t::board_M5AtomU:
    name = "ATOM U";
    break;
  case m5::board_t::board_M5TimerCam:
    name = "TimerCamera";
    break;
  case m5::board_t::board_M5StampPico:
    name = "StampPico";
    break;
#endif
  default:
    name = "Who am I ?";
    break;
  }
  M5.Display.startWrite();
  M5.Display.print("Core:");
  M5.Display.println(name);
  M5_LOGI("core:%s", name);

  // run-time branch : imu model check
  switch (M5.Imu.getType())
  {
  case m5::imu_t::imu_mpu6050:
    name = "MPU6050";
    break;
  case m5::imu_t::imu_mpu6886:
    name = "MPU6886";
    break;
  case m5::imu_t::imu_mpu9250:
    name = "MPU9250";
    break;
  case m5::imu_t::imu_bmi270:
    name = "BMI270";
    break;
  case m5::imu_t::imu_sh200q:
    name = "SH200Q";
    break;
  case m5::imu_t::imu_none:
    name = "none";
    break;
  default:
    name = "unknown";
    break;
  }
  M5.Display.print("IMU:");
  M5.Display.println(name);
  M5.Display.endWrite();
  M5_LOGI("imu:%s", name);

  // You can instruct the bottom edge of the touchscreen to be treated as BtnA ~ BtnC by pixel number.
  M5.setTouchButtonHeight(32);
  // You can also specify the ratio to the screen height. (25 = 10% , 255 = 100%)
  // M5.setTouchButtonHeightByRatio(25);
}

void loop(void)
{
  M5.delay(1);
  int h = M5.Display.height() / 8;

  M5.update();

  //------------------- Button test
  /*
  /// List of available buttons:
    M5Stack BASIC/GRAY/GO/FIRE:  BtnA,BtnB,BtnC
    M5Stack Core2:               BtnA,BtnB,BtnC,BtnPWR
    M5Stick C/CPlus:             BtnA,BtnB,     BtnPWR
    M5Stick CoreInk:             BtnA,BtnB,BtnC,BtnPWR,BtnEXT
    M5Paper:                     BtnA,BtnB,BtnC
    M5Station:                   BtnA,BtnB,BtnC,BtnPWR
    M5Stack CoreS3,Tough:                       BtnPWR
    M5Atom Series:               BtnA
    M5Stamp Series:              BtnA
  */

  static constexpr const int colors[] = {TFT_WHITE, TFT_CYAN, TFT_RED, TFT_YELLOW, TFT_BLUE, TFT_GREEN};
  static constexpr const char *const names[] = {"none", "wasHold", "wasClicked", "wasPressed", "wasReleased", "wasDeciedCount"};

  M5.Display.startWrite();

  /// BtnPWR: "wasClicked"/"wasHold"  can be use.
  /// BtnPWR of CoreInk: "isPressed"/"wasPressed"/"isReleased"/"wasReleased"/"wasClicked"/"wasHold"/"isHolding"  can be use.
  int state = M5.BtnPWR.wasHold()               ? 1
              : M5.BtnPWR.wasClicked()          ? 2
              : M5.BtnPWR.wasPressed()          ? 3
              : M5.BtnPWR.wasReleased()         ? 4
              : M5.BtnPWR.wasDecideClickCount() ? 5
                                                : 0;
  if (state)
  {
    M5.Led.setAllColor(colors[state]);
    M5.Speaker.tone(783.991, 100);
    M5_LOGI("BtnPWR:%s  count:%d", names[state], M5.BtnPWR.getClickCount());
    if (!M5.Display.displayBusy())
    {
      M5.Display.fillRect(0, h * 2, h, h - 1, colors[state]);
      M5.Display.setCursor(0, h * 2);
      M5.Display.printf("%d", M5.BtnPWR.getClickCount());
    }
  }

  /// BtnA,BtnB,BtnC,BtnEXT: "isPressed"/"wasPressed"/"isReleased"/"wasReleased"/"wasClicked"/"wasHold"/"isHolding"  can be use.
  state = M5.BtnA.wasHold()               ? 1
          : M5.BtnA.wasClicked()          ? 2
          : M5.BtnA.wasPressed()          ? 3
          : M5.BtnA.wasReleased()         ? 4
          : M5.BtnA.wasDecideClickCount() ? 5
                                          : 0;
  if (state)
  {
    M5.Led.setAllColor(colors[state]);
    M5.Speaker.tone(523.251, 100);
    M5_LOGI("BtnA:%s  count:%d", names[state], M5.BtnA.getClickCount());
    if (!M5.Display.displayBusy())
    {
      M5.Display.fillRect(0, h * 3, h, h - 1, colors[state]);
      M5.Display.setCursor(0, h * 3);
      M5.Display.printf("%d", M5.BtnA.getClickCount());
    }
  }

  state = M5.BtnB.wasHold()               ? 1
          : M5.BtnB.wasClicked()          ? 2
          : M5.BtnB.wasPressed()          ? 3
          : M5.BtnB.wasReleased()         ? 4
          : M5.BtnB.wasDecideClickCount() ? 5
                                          : 0;
  if (state)
  {
    M5.Led.setAllColor(colors[state]);
    M5.Speaker.tone(587.330, 100);
    M5_LOGI("BtnB:%s  count:%d", names[state], M5.BtnB.getClickCount());
    if (!M5.Display.displayBusy())
    {
      M5.Display.fillRect(0, h * 4, h, h - 1, colors[state]);
      M5.Display.setCursor(0, h * 4);
      M5.Display.printf("%d", M5.BtnB.getClickCount());
    }
  }

  state = M5.BtnC.wasHold()               ? 1
          : M5.BtnC.wasClicked()          ? 2
          : M5.BtnC.wasPressed()          ? 3
          : M5.BtnC.wasReleased()         ? 4
          : M5.BtnC.wasDecideClickCount() ? 5
                                          : 0;
  if (state)
  {
    M5.Led.setAllColor(colors[state]);
    M5.Speaker.tone(659.255, 100);
    M5_LOGI("BtnC:%s  count:%d", names[state], M5.BtnC.getClickCount());
    if (!M5.Display.displayBusy())
    {
      M5.Display.fillRect(0, h * 5, h, h - 1, colors[state]);
      M5.Display.setCursor(0, h * 5);
      M5.Display.printf("%d", M5.BtnC.getClickCount());
    }
  }

  state = M5.BtnEXT.wasHold()               ? 1
          : M5.BtnEXT.wasClicked()          ? 2
          : M5.BtnEXT.wasPressed()          ? 3
          : M5.BtnEXT.wasReleased()         ? 4
          : M5.BtnEXT.wasDecideClickCount() ? 5
                                            : 0;
  if (state)
  {
    M5.Led.setAllColor(colors[state]);
    M5.Speaker.tone(698.456, 100);
    M5_LOGI("BtnEXT:%s  count:%d", names[state], M5.BtnEXT.getClickCount());
    if (!M5.Display.displayBusy())
    {
      M5.Display.fillRect(0, h * 6, h, h - 1, colors[state]);
      M5.Display.setCursor(0, h * 6);
      M5.Display.printf("%d", M5.BtnEXT.getClickCount());
    }
  }

  M5.Display.endWrite();

  if (!M5.Display.displayBusy())
  {
    static uint32_t prev_sec;
    uint32_t sec = m5gfx::millis() / 1000;
    if (prev_sec != sec)
    {
      prev_sec = sec;

      //------------------- Battery level
      static int prev_battery = INT_MAX;
      int battery = M5.Power.getBatteryLevel();
      if (prev_battery != battery)
      {
        prev_battery = battery;
        M5.Display.startWrite();
        M5.Display.setCursor(0, M5.Display.fontHeight() * 3);
        M5.Display.print("Bat:");
        if (battery >= 0)
        {
          M5.Display.printf("%03d", battery);
        }
        else
        {
          M5.Display.print("none");
        }
        M5.Display.endWrite();
      }
      //------------------- RTC test
      if (M5.Rtc.isEnabled())
      {
        static constexpr const char *const wd[] = {"Sun", "Mon", "Tue", "Wed", "Thr", "Fri", "Sat", "ERR"};
        char buf[32];
        //*
        /// Get the date and time from the RTC and display it.
        m5::rtc_datetime_t dt;
        if (M5.Rtc.getDateTime(&dt))
        {
          M5.Display.startWrite();
          snprintf(buf, 30, "%04d/%02d/%02d(%s)", dt.date.year, dt.date.month, dt.date.date, wd[dt.date.weekDay & 7]);
          M5.Display.drawString(buf, M5.Display.width() / 2, 0);
          snprintf(buf, 30, "%02d:%02d:%02d", dt.time.hours, dt.time.minutes, dt.time.seconds);
          M5.Display.drawString(buf, M5.Display.width() / 2, M5.Display.fontHeight());
          M5.Display.endWrite();
        }
        else
        {
          M5.Display.drawString("RTC error", M5.Display.width() / 2, M5.Display.fontHeight() >> 1);
        }
        /*/
        /// In the example above, the date and time are obtained through I2C communication with the RTC.
        /// However, since M5Unified synchronizes the ESP32's internal clock at startup,
        /// it is also possible to get the date and time, as shown in the example below.
        /// ※ Note: that there will be an error of a few seconds per day.
        ///    You may want to call M5.Rtc.setSystemTimeFromRtc() periodically to synchronize.
              auto t = time(nullptr);
              auto time = localtime(&t);
              M5.Display.startWrite();
              snprintf( buf, 30, "%04d/%02d/%02d(%s)"
                      , time->tm_year + 1900
                      , time->tm_mon + 1
                      , time->tm_mday
                      , wd[time->tm_wday & 7]
                      );
              M5.Display.drawString(buf, M5.Display.width() / 2, 0);
              snprintf( buf, 30, "%02d:%02d:%02d"
                      , time->tm_hour
                      , time->tm_min
                      , time->tm_sec
                      );
              M5.Display.drawString(buf, M5.Display.width() / 2, M5.Display.fontHeight());
              M5.Display.endWrite();
        //*/
      }
    }
  }

  //------------------- IMU test
  if (M5.Imu.isEnabled())
  {
    int ox = (M5.Display.width() + h) >> 1;
    static int prev_xpos[6];
    int xpos[6];
    float val[6];
    M5.Imu.getAccel(&val[0], &val[1], &val[2]);
    M5.Imu.getGyro(&val[3], &val[4], &val[5]);
    int color[6] = {TFT_RED, TFT_GREEN, TFT_BLUE, TFT_RED, TFT_GREEN, TFT_BLUE};

    for (int i = 0; i < 3; ++i)
    {
      xpos[i] = val[i] * 50;
      xpos[i + 3] = val[i + 3] / 2;
    }

    M5.Display.startWrite();
    M5.Display.setClipRect(h, h, M5.Display.width(), M5.Display.height());
    while (M5.Display.displayBusy())
    {
      M5.delay(1);
    }
    for (int i = 0; i < 6; ++i)
    {
      if (xpos[i] == prev_xpos[i])
        continue;

      int px = prev_xpos[i];
      if ((xpos[i] < 0) != (px < 0))
      {
        if (px)
        {
          M5.Display.fillRect(ox, h * (i + 2), px, h, M5.Display.getBaseColor());
        }
        px = 0;
      }
      if (xpos[i] != px)
      {
        if ((xpos[i] > px) != (xpos[i] < 0))
        {
          M5.Display.setColor(color[i]);
        }
        else
        {
          M5.Display.setColor(M5.Display.getBaseColor());
        }
        M5.Display.fillRect(xpos[i] + ox, h * (i + 2), px - xpos[i], h);
      }
      prev_xpos[i] = xpos[i];
    }
    M5.Display.clearClipRect();

    M5.Display.endWrite();
  }
  M5.Display.display();
}

#if !defined(ARDUINO) && defined(ESP_PLATFORM)
extern "C"
{
  void loopTask(void *)
  {
    setup();
    for (;;)
    {
      loop();
    }
    vTaskDelete(NULL);
  }

  void app_main()
  {
    xTaskCreatePinnedToCore(loopTask, "loopTask", 8192, NULL, 1, NULL, 1);
  }
}
#endif

const uint8_t wav_8bit_44100[2] = {0x00, 0x00};
