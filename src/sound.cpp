#include <Arduino.h>
#include <avr/pgmspace.h>
#include "config.h"
#include "sound.h"
#include "sound_patterns.h"
namespace {
uint8_t phrase = 0;
uint8_t position = 0;
sound::Note note = {};
uint32_t since = 0;
uint32_t lastPitch = 0;
uint16_t previousHz = 0;
bool playing = false;
bool audible = false;
void setPitch(uint16_t hz) {
  if (hz == previousHz) return;
  if (hz) tone(config::PIEZO_PIN, hz);
  else noTone(config::PIEZO_PIN);
  previousHz = hz;
  audible = hz != 0;
}
void startNote(uint32_t now) {
  memcpy_P(&note, &sound::phrases[phrase][position], sizeof(note));
  since = now;
  lastPitch = now;
  setPitch(sound::pitchAt(note, 0));
}
}
void sound::begin() {
  pinMode(config::PIEZO_PIN, OUTPUT);
  digitalWrite(config::PIEZO_PIN, LOW);
}
void sound::stop() {
  if (!playing && !audible) return;
  noTone(config::PIEZO_PIN);
  digitalWrite(config::PIEZO_PIN, LOW);
  previousHz = 0;
  audible = false;
  playing = false;
}
void sound::play(personality::Mood mood, uint32_t now) {
  stop();
  if (!config::SOUND_ENABLED || static_cast<uint8_t>(mood) >= 9) return;
  phrase = static_cast<uint8_t>(mood);
  position = 0;
  playing = true;
  startNote(now);
}
void sound::update(uint32_t now) {
  if (!playing) return;
  if (noteFinished(now, since, note.ms)) {
    if (++position >= NOTES_PER_PHRASE) stop();
    else startNote(now);
    return;
  }
  if (noteFinished(now, lastPitch, PITCH_TICK_MS)) {
    lastPitch = now;
    setPitch(pitchAt(note, static_cast<uint16_t>(now - since)));
  }
}
bool sound::toneActive() { return audible; }
