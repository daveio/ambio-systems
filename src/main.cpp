#include <Arduino.h>
#include "M5Capsule.h"
#include "M5GFX.h"
#include <M5UnitOLED.h>

/**
 * @file buzzer.ino
 * @author SeanKwok (shaoxiang@m5stack.com)
 * @brief M5Capsule Buzzer Test
 * @version 0.1
 * @date 2023-10-08
 *
 *
 * @Hardwares: M5Capsule
 * @Platform Version: Arduino M5Stack Board Manager v2.0.7
 * @Dependent Library:
 * M5GFX: https://github.com/m5stack/M5GFX
 * M5Unified: https://github.com/m5stack/M5Unified
 */
static constexpr char text0[] = "one";
static constexpr char text1[] = "two";
static constexpr char text2[] = "three";
static constexpr char text3[] = "four";
static constexpr char text4[] = "five";
static constexpr char text5[] = "six";
static constexpr char text6[] = "seven";
static constexpr char text7[] = "eight";
static constexpr const char* text[] = { text0, text1, text2, text3, text4, text5, text6, text7 };


M5UnitOLED display;  // default setting
// M5UnitOLED display ; // SDA, SCL, FREQ
M5Canvas canvas(&display);

  void setup() {
  auto cfg = M5.config();
  M5Capsule.begin(cfg);
  display.init();
  display.setup();

  if (display.width() < display.height()) {
    display.setRotation(display.getRotation() ^ 1);
  }

  canvas.setColorDepth(1);  // mono color
  canvas.createSprite(display.width(), display.height());
  // canvas.setTextSize(1); // smaller than 1 is unreadable, sets to 1 without this call
  canvas.setTextScroll(true);
}

void loop() {
  // M5Capsule.Speaker.tone(4500, 100);
  delay(1000);
  static int count = 0;
  canvas.printf("%04d:%s\r\n", count, text[count & 7]);
  canvas.pushSprite(0, 0);
  ++count;
  // M5Capsule.Speaker.tone(3000, 100);
  delay(1000);
}

/*/

/// Example without canvas
void setup(void)
{
  display.begin();

  if (display.isEPD())
  {
    display.setEpdMode(epd_mode_t::epd_fastest);
    display.invertDisplay(true);
    display.clear(TFT_BLACK);
  }
  if (display.width() < display.height())
  {
    display.setRotation(display.getRotation() ^ 1);
  }

  display.setTextSize((float)display.width() / 160);
  display.setTextScroll(true);
}

void loop(void)
{
  static int count = 0;

  display.printf("%04d:%s\r\n", count, text[count & 7]);
  ++count;
}
//*/
