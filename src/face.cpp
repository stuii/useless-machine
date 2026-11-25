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
#include "startup_animation.h"
#include "dream_art.h"
#include "surprises.h"
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
face::Expression expressionValue=face::Expression::Normal;
uint32_t expressionSince=0, asleepSince=0;
uint8_t expressionVariant=0;
bool wasAsleep=false;
uint32_t effectKey=0xffffffffUL;
bool booting = true;
uint32_t bootSince = 0;
uint8_t bootFrame = 255;
static_assert(config::STARTUP_SETTLE_MS >= startupAnimation::DURATION_MS, "Startup must cover the intro animation");

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

constexpr uint32_t SLEEP_Z_COLOR = 0x0C131F; // Fixed dim blue, independent of breathing.

uint8_t sleepZPhase(uint32_t now) {
  return config::SLEEP_Z_STATIC_TEST ? 3
      : (now / sleepAnimation::Z_FRAME_MS) % sleepAnimation::Z_PHASES;
}

uint8_t sleepBreath(uint32_t now) {
  const uint8_t phase = (now / 500) % 8;
  return 35 + (phase < 4 ? phase : 7 - phase) * 10;
}

void drawStartup(uint8_t frame, uint32_t now) {
  // Use the live sleeping scene as the fade target, including its moving Zs.
  // The same clock and color continue after startup, without a separate reveal.
  const uint8_t zPhase = sleepZPhase(now);
  const uint8_t breath = sleepBreath(now);
  for (uint8_t y=0;y<8;++y) {
    for (uint8_t x=0;x<8;++x) {
      const auto c=startupAnimation::color(x,y,frame);
      const uint8_t mix=startupAnimation::faceMix(x,y,frame);
      const uint32_t asleep=sleepAnimation::floatingZ(x,y,zPhase)
          ? SLEEP_Z_COLOR : pixel(x,y,7,false,false,breath);
      put(x,y,pixels.Color(
          faceAnimation::mix(c.r,(asleep>>16)&255,mix),
          faceAnimation::mix(c.g,(asleep>>8)&255,mix),
          faceAnimation::mix(c.b,asleep&255,mix)));
    }
  }
}

void drawZs(uint8_t phase) {
  for (uint8_t y = 0; y < 5; ++y)
    for (uint8_t x = 0; x < 8; ++x)
      if (sleepAnimation::floatingZ(x, y, phase))
        // Fixed dim blue, below even the sleeping face's darkest breathing phase.
        put(x, y, SLEEP_Z_COLOR);
}

// Effects are drawn as complete frames; they never drive the mechanism or sound.
void drawEffect(face::Expression effect,uint32_t age,uint8_t variant) {
  if(effect==face::Expression::Sigh){
    const uint32_t exhaleStart=config::ANNOYED_IGNORE_MS-1000;
    const bool exhaling=age>=exhaleStart && age<exhaleStart+800;
    for(uint8_t y=0;y<8;++y)for(uint8_t x=0;x<8;++x){
      const bool eyes=(y==3 && (x==1 || x==2 || x==5 || x==6)) ||
          (!exhaling && y==2 && (x==1 || x==5));
      const bool mouth=exhaling?((y==5 || y==7) && (x==3 || x==4)) ||
          (y==6 && (x==2 || x==5)):(y==6 && x>=2 && x<=5);
      put(x,y,eyes?0x704010:mouth?0x402008:0);
    }
    return;
  }
  if(effect==face::Expression::Surrender){
    // Downcast eyes, warm blush, a crooked mouth and a tiny waving white flag.
    for(uint8_t y=0;y<8;++y)for(uint8_t x=0;x<8;++x){
      uint32_t color=0;
      if(y==4 && (x==1 || x==2 || x==5 || x==6))color=0x304850;
      if(y==5 && (x==0 || x==7))color=0x501820;
      if((y==6 && x>=2 && x<=4) || (y==7 && x==5))color=0x304850;
      const uint8_t flagX=4+(age/600)%2;
      if((y<2 && x>=flagX && x<flagX+3) || (y==2 && x==flagX))color=0x505048;
      put(x,y,color);
    }
    return;
  }
  if(effect==face::Expression::SystemError){
    // Colorful broken scanlines, then a deadpan recovery. Respect global brightness.
    for(uint8_t y=0;y<8;++y)for(uint8_t x=0;x<8;++x){
      const uint8_t phase=(x+2*y+age/150)%6;
      const uint32_t colors[]={0x702010,0x504010,0x106030,0x105060,0x302070,0x602040};
      put(x,y,(y+age/150)%3==0?colors[phase]:0);
    }
    if(age>=1300){draw(0,false,false,65);for(uint8_t x=2;x<6;++x)put(x,6,0x304050);}
    return;
  }
  if(effect==face::Expression::Countdown){
    const uint8_t digit=age<700?3:age<1400?2:1;
    for(uint8_t y=0;y<8;++y)for(uint8_t x=0;x<8;++x){
      const bool lit=y>=1 && y<6 && x>=2 && x<5 &&
          (surprises::digitRow(digit,y-1)&(4>>(x-2)));
      put(x,y,lit?(digit==3?0x105040:digit==2?0x605010:0x702010):0);
    }
    return;
  }
  if(effect==face::Expression::Deadpan){
    for(uint8_t y=0;y<8;++y)for(uint8_t x=0;x<8;++x)
      put(x,y,((y==3 && (x==1 || x==2 || x==5 || x==6)) ||
          (y==6 && x>=2 && x<=5))?0x304050:0);
    return;
  }
  if(effect==face::Expression::FakeSleep || effect==face::Expression::Peek){
    draw(7,false,false,55);
    if(effect==face::Expression::FakeSleep)drawZs(sleepZPhase(age));
    else {put(5,4,0x204858);put(6,4,0x204858);put(5,5,0x608898);}
  } else {
    draw(0,false,false,100);
    // A deliberate wink accompanies the knowing look and the first victory.
    if(effect==face::Expression::Knowing || variant==1){
      for(uint8_t y=1;y<5;++y)for(uint8_t x=4;x<8;++x)put(x,y,0);
      put(5,3,0x408050);put(6,3,0x408050);
      if(effect==face::Expression::Knowing){put(5,1,0x408050);put(6,0,0x408050);}
    }
    if(effect==face::Expression::Victory){
      const uint8_t frame=age/100;
      if(variant==2){ // A pulsing heart above the smile.
        for(uint8_t y=0;y<5;++y)for(uint8_t x=0;x<8;++x)
          put(x,y,dreamArt::lit(0,x,y)?(frame%4<2?0x782018:0x38100C):0);
      } else { // Confetti travels around the border, clear of the face.
        for(uint8_t i=0;i<8;++i){
          const uint8_t y=(i+frame)%8;
          put(i,0,(i+frame)%2?0x403008:0x083040);
          if(variant==3){put(0,y,0x402030);put(7,7-y,0x203840);}
        }
      }
    }
  }
}
void drawDream(uint32_t age,uint8_t picture,uint32_t now){
  draw(7,false,false,sleepBreath(now));
  const uint16_t fade=age<600?age:age>2600?3200-age:600;
  const uint8_t glow=uint32_t(fade)*24/600;
  for(uint8_t y=0;y<5;++y)for(uint8_t x=0;x<8;++x)
    if(dreamArt::lit(picture,x,y))put(x,y,pixels.Color(glow,glow/2,glow));
}

bool inServoGap() {
  // One Servo 1.2.2 channel: at most 2.4 ms of pulses per 20 ms frame.
  // Start the ~1.92 ms LED transfer at 4..14 ms, with the signal LOW.
  // Revisit before adding Servo objects or changing timer libraries.
  uint16_t ticks;
  ATOMIC_BLOCK(ATOMIC_RESTORESTATE) { ticks = TCNT1; }
  return ticks >= 8000 && ticks <= 28000
      && digitalRead(config::ARM_SERVO_PIN) == LOW;
}
}

void face::expression(Expression value,uint32_t since,uint8_t variant){
  if(value!=expressionValue || (value!=Expression::Normal && since!=expressionSince)){
    effectKey=0xffffffffUL;displayedKey=65535;
  }
  expressionValue=value;expressionSince=since;expressionVariant=variant;
}

void face::begin() {
  pixels.begin();
  pixels.setBrightness(config::FACE_BRIGHTNESS);
  transition.current = 7;
  transition.target = 7;
  bootSince = millis();
  booting = true;
  drawStartup(0, bootSince);
  bootFrame = 0;
  pixels.show(); // Before attaching Servo.
}

void face::update(uint32_t now, personality::Mood emotion, bool reacting) {
  const uint8_t mood = static_cast<uint8_t>(emotion);
  if (mood >= 9) return;
  if (booting) {
    const uint32_t age = now - bootSince;
    if (age < startupAnimation::DURATION_MS) {
      const uint8_t frame = startupAnimation::frameAt(age);
      if (frame == bootFrame || sound::toneActive()) return;
      drawStartup(frame, now);
      if (!inServoGap()) return;
      pixels.show();
      bootFrame = frame;
      return;
    }
    booting = false;
    displayedKey = 65535;
    blinkSince = now;
  }
  const bool asleep=emotion==personality::Mood::Asleep && expressionValue==Expression::Normal;
  if(asleep && !wasAsleep)asleepSince=now;
  wasAsleep=asleep;
  const uint32_t sleepAge=now-asleepSince;
  const bool dream=asleep && dreamArt::dreaming(sleepAge);
  if(expressionValue!=Expression::Normal || dream){
    const uint32_t key=now/100;
    if(key==effectKey || sound::toneActive() || !inServoGap())return;
    if(dream)drawDream((sleepAge-12000)%18000,((sleepAge-12000)/18000)%3,now);
    else drawEffect(expressionValue,now-expressionSince,expressionVariant);
    pixels.show();effectKey=key;displayedKey=65535;
    return;
  }
  effectKey=0xffffffffUL;
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
  const uint8_t breath = emotion == personality::Mood::Asleep ? sleepBreath(now) : 100;
  const uint8_t zPhase = emotion == personality::Mood::Asleep ? sleepZPhase(now) : 3;
  const uint16_t key = (uint16_t(zPhase) << 9) | mood | (blink ? 16 : 0) | (eyebrow ? 32 : 0) | (phase << 6);
  if (key == displayedKey || sound::toneActive()) return; // Avoid audio clicks from LED interrupt masking.
  draw(mood, blink, eyebrow, breath);
  if (emotion == personality::Mood::Asleep) drawZs(zPhase);
  if (!inServoGap()) return;
  pixels.show();
  displayedKey = key;
}
