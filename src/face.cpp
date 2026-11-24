#include <Arduino.h>
#include <Adafruit_NeoPixel.h>
#include <avr/pgmspace.h>
#include <util/atomic.h>
#include "config.h"
#include "face.h"
#include "sound.h"
#include "face_art.h"
#include "matrix.h"
#include "face_transition.h"
#include "sleep_animation.h"
#if !defined(__AVR_ATmega328P__) || F_CPU != 16000000UL
#error "Face/servo scheduling requires the classic 16 MHz ATmega328P Nano."
#endif
namespace {
Adafruit_NeoPixel pixels(64, config::FACE_PIN, NEO_GRB + NEO_KHZ800);
uint16_t displayedKey = 65535;
uint32_t blinkSince = 0;
bool blinking = false;
uint8_t blinkSequence = 0;
faceAnimation::Transition transition;
bool booting = true;
uint32_t bootSince = 0;
uint8_t bootFrame = 255;
static_assert(config::STARTUP_SETTLE_MS >= sleepAnimation::DURATION_MS, "Startup must cover the intro animation");

uint32_t ink(char part, uint8_t mood, uint8_t brightness) {
  uint8_t r = pgm_read_byte(&faceArt::palettes[mood][0]);
  uint8_t g = pgm_read_byte(&faceArt::palettes[mood][1]);
  uint8_t b = pgm_read_byte(&faceArt::palettes[mood][2]);
  if (part == 'h') { r = 225; g = 245; b = 255; }
  if (part == 'c') { r = 115; g = 25; b = 60; }
  if (part == 'm') { r = r * 3 / 4; g = g * 3 / 4; b = b * 3 / 4; }
  return pixels.Color(uint16_t(r) * brightness / 100,
      uint16_t(g) * brightness / 100, uint16_t(b) * brightness / 100);
}

uint32_t pixel(uint8_t x, uint8_t y, uint8_t mood, bool blink, bool reacting, uint8_t breath) {
  char part = pgm_read_byte(&faceArt::sprites[mood][y][x]);
  if (blink && (part == 'e' || part == 'h')) part = '.';
  if (blink && y == 3 && (x == 1 || x == 2 || x == 5 || x == 6)) part = 'e';
  if (reacting && mood <= 2 && y == 0 && (x == 5 || x == 6)) part = 'b';
  return part == '.' ? 0 : ink(part, mood, breath);
}

void put(uint8_t x, uint8_t y, uint32_t color) {
  pixels.setPixelColor(matrix::index(x, y, config::MATRIX_SERPENTINE,
      config::MATRIX_ROTATION, config::MATRIX_MIRROR), color);
}

void draw(uint8_t mood, bool blink, bool reacting, uint8_t breath) {
  for (uint8_t y = 0; y < 8; ++y)
    for (uint8_t x = 0; x < 8; ++x)
      put(x, y, pixel(x, y, mood, blink, reacting, breath));
}

uint32_t scene(uint8_t x, uint8_t y, uint8_t which) {
  return pixel(x, y, which < 2 ? transition.current : transition.target,
      which == 1 || which == 2, false, 100);
}

void drawTransition() {
  const auto blend = faceAnimation::blend(transition.frame);
  for (uint8_t y = 0; y < 8; ++y) {
    for (uint8_t x = 0; x < 8; ++x) {
      const uint32_t a = scene(x, y, blend.first);
      const uint32_t b = scene(x, y, blend.second);
      put(x, y, pixels.Color(
          faceAnimation::mix((a >> 16) & 255, (b >> 16) & 255, blend.amount),
          faceAnimation::mix((a >> 8) & 255, (b >> 8) & 255, blend.amount),
          faceAnimation::mix(a & 255, b & 255, blend.amount)));
    }
  }
}

void drawStartup(uint8_t frame) {
  pixels.clear();
  if (frame < 8) {
    // A small cyan light expands into a diamond, then contracts.
    const uint8_t radius = frame < 4 ? frame : 7 - frame;
    for (uint8_t y = 0; y < 8; ++y)
      for (uint8_t x = 0; x < 8; ++x) {
        const uint8_t distance = abs(int(x) - 3) + abs(int(y) - 3);
        if (distance == radius) put(x, y, pixels.Color(30, 100, 180));
      }
  } else {
    // Reveal the sleeping face softly, ending near its breathing brightness.
    draw(7, false, false, 10 + (frame - 8) * 7);
  }
}

void drawZs(uint8_t phase, uint8_t breath) {
  for (uint8_t y = 0; y < 5; ++y)
    for (uint8_t x = 0; x < 8; ++x)
      if (sleepAnimation::floatingZ(x, y, phase))
        put(x, y, pixels.Color(30, 65 + breath, 110 + breath));
}

bool inServoGap() {
  // Two Servo 1.2.2 channels use at most 2 * 2.4 = 4.8 ms per
  // 20 ms frame at 16 MHz. Wait until 6 ms; a 64-pixel transfer takes
  // ~1.92 ms and must start by 14 ms. Also covers startup's inactive slots.
  // Both signals must be LOW. Revisit for a third Servo object.
  uint16_t ticks;
  ATOMIC_BLOCK(ATOMIC_RESTORESTATE) { ticks = TCNT1; }
  return ticks >= 12000 && ticks <= 28000
      && digitalRead(config::ARM_SERVO_PIN) == LOW
      && digitalRead(config::LID_SERVO_PIN) == LOW;
}
}

void face::begin() {
  pixels.begin();
  pixels.setBrightness(config::FACE_BRIGHTNESS);
  transition.current = 7;
  transition.target = 7;
  bootSince = millis();
  booting = true;
  drawStartup(0);
  bootFrame = 0;
  pixels.show(); // Before attaching Servo.
}

void face::update(uint32_t now, personality::Mood emotion, bool reacting) {
  const uint8_t mood = static_cast<uint8_t>(emotion);
  if (mood >= 9) return;
  if (booting) {
    const uint32_t age = now - bootSince;
    if (age < sleepAnimation::DURATION_MS) {
      const uint8_t frame = sleepAnimation::startupFrame(age);
      if (frame == bootFrame || sound::toneActive()) return;
      drawStartup(frame);
      if (!inServoGap()) return;
      pixels.show();
      bootFrame = frame;
      return;
    }
    booting = false;
    displayedKey = 65535;
    blinkSince = now;
  }
  transition.request(mood, now);
  if (transition.active) {
    if (!transition.due(now) || sound::toneActive()) return;
    drawTransition();
    if (!inServoGap()) return;
    pixels.show();
    transition.presented(now); // Advance only after a frame actually reaches the LEDs.
    displayedKey = 65535;
    blinking = false;
    blinkSince = now;
    return;
  }
  // Slightly varied blink intervals, no random seed or floating point needed.
  const uint32_t interval = blinking ? (mood >= 5 ? 280UL : 130UL)
      : 2600UL + (blinkSequence % 3) * 700UL;
  if (now - blinkSince >= interval) {
    blinking = !blinking;
    blinkSince = now;
    if (!blinking) ++blinkSequence;
  }
  const bool blink = blinking && emotion != personality::Mood::Furious
      && emotion != personality::Mood::Asleep;
  const bool eyebrow = reacting && mood <= 2;
  // Slow eight-step breathing only while asleep; no full-panel flashes.
  const uint8_t phase = emotion == personality::Mood::Asleep ? (now / 500) % 8 : 0;
  const uint8_t breath = emotion == personality::Mood::Asleep
      ? 35 + (phase < 4 ? phase : 7 - phase) * 10 : 100;
  const uint8_t zPhase = emotion == personality::Mood::Asleep && !config::SLEEP_Z_STATIC_TEST ? (now / 220) % 8 : 0;
  const uint16_t key = (uint16_t(zPhase) << 9) | mood | (blink ? 16 : 0) | (eyebrow ? 32 : 0) | (phase << 6);
  if (key == displayedKey || sound::toneActive()) return; // Avoid audio clicks from LED interrupt masking.
  draw(mood, blink, eyebrow, breath);
  if (emotion == personality::Mood::Asleep) drawZs(zPhase, breath);
  if (!inServoGap()) return;
  pixels.show();
  displayedKey = key;
}
