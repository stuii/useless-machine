#pragma once
#include <stdint.h>
#include "config.h"

namespace personality {
enum class Routine : uint8_t { Greeting, Inspect, Distrust, FakeOut, Swat, DoubleJab, Creep };
enum class Mood : uint8_t { Happy, Curious, Playful, Annoyed, Furious, Tired, Sleepy, Asleep, Suspicious };
constexpr uint8_t add(uint8_t value, uint8_t amount) { return value + amount > 100 ? 100 : value + amount; }
constexpr uint8_t sub(uint8_t value, uint8_t amount) { return value > amount ? value - amount : 0; }
constexpr uint16_t armInterval(uint16_t ms) {
  const uint16_t scaled = uint32_t(ms) * config::ARM_STEP_PERCENT / 100;
  return scaled < 3 ? 3 : scaled;
}
struct Parameters {
  uint8_t energy = 90;
  uint8_t annoyance = 0;
  uint8_t excitement = 0;
  uint8_t curiosity = 40;
  uint8_t patience = 80;
  uint8_t mischief = 25;
};
// A zero movement interval requests the target immediately (no software ramp).
struct Behavior {
  uint16_t reactionMs; uint16_t stepMs; uint16_t approachMs; uint16_t pauseMs; uint16_t returnMs;
  uint16_t hideWaitMs = config::MISCHIEF_HIDE_WAIT_MS;
  Routine routine = Routine::Swat;
  int peekDeg = config::HOME_DEG + (config::PUSH_DEG - config::HOME_DEG) * 60 / 100;
};

class Model {
 public:
  Parameters values;
  uint32_t quietMs = 0;
  uint16_t activations = 0;
  uint8_t playStreak = 0; // Warm up again after a quiet break, not just once per boot.

  constexpr void startAsleep() { quietMs = config::SLEEP_AFTER_MS; }

  constexpr void activate() {
    const bool rapid = activations > 0 && quietMs < config::RAPID_POKE_WINDOW_MS;
    playStreak = rapid ? (playStreak < config::PLAYFUL_MIN_POKES ? playStreak + 1 : playStreak) : 1;
    // Patience is tolerance for the NEXT poke, not the inverse of existing anger.
    const uint8_t irritation = rapid ? 10 + (100 - values.patience) / 20 : 3;
    values.annoyance = add(values.annoyance, irritation);
    values.patience = sub(values.patience, rapid ? 10 : 3);
    // Novelty piques curiosity; repetitive poking becomes boring.
    values.curiosity = rapid ? sub(values.curiosity, 12) : add(values.curiosity, 22);
    // Playfulness grows while interaction is welcome, then fades under stress.
    values.mischief = values.annoyance < config::ANNOYED_THRESHOLD
        ? add(values.mischief, rapid ? 8 : 16) : sub(values.mischief, 12);
    values.energy = sub(values.energy, rapid ? config::ENERGY_COST_REPEATED : config::ENERGY_COST_SPACED);
    values.excitement = add(values.excitement, rapid ? 24 : 20);
    if (activations < 65535) ++activations;
    quietMs = 0;
  }

  // One second of quiet time. A blocked ON switch is not quiet.
  constexpr void restSecond() {
    if (quietMs < 300000UL) quietMs += 1000;
    values.excitement = sub(values.excitement, config::EXCITEMENT_DECAY_PER_SECOND);
    if (quietMs >= config::ANNOYANCE_RECOVERY_DELAY_MS)
      values.annoyance = sub(values.annoyance, config::ANNOYANCE_RECOVERY_PER_SECOND);
    if (quietMs >= config::RECOVERY_DELAY_MS) {
      values.energy = add(values.energy, config::ENERGY_RECOVERY_PER_SECOND);
      values.patience = add(values.patience, config::PATIENCE_RECOVERY_PER_SECOND);
      if (values.curiosity < 70) {
        values.curiosity = add(values.curiosity, config::CURIOSITY_RECOVERY_PER_SECOND);
        if (values.curiosity > 70) values.curiosity = 70;
      }
      if (values.mischief > 25) {
        values.mischief = sub(values.mischief, config::MISCHIEF_RECOVERY_PER_SECOND);
        if (values.mischief < 25) values.mischief = 25;
      } else if (values.mischief < 25) {
        values.mischief = add(values.mischief, config::MISCHIEF_RECOVERY_PER_SECOND);
        if (values.mischief > 25) values.mischief = 25;
      }
    }
  }
  constexpr void disturbed() { quietMs = 0; }

  constexpr Mood mood() const {
    // Rest can lead to sleep only after irritation has subsided.
    if (quietMs >= config::SLEEP_AFTER_MS && values.annoyance < 20) return Mood::Asleep;
    // Fatigue takes priority over anger: exhausted, not endlessly frantic.
    if (values.energy <= 25) return Mood::Tired;
    if (values.annoyance >= config::FURIOUS_THRESHOLD) return Mood::Furious;
    if (values.annoyance >= config::ANNOYED_THRESHOLD) return Mood::Annoyed;
    if (quietMs >= config::SLEEPY_AFTER_MS && values.excitement < 20) return Mood::Sleepy;
    if (values.annoyance >= 15 && values.curiosity >= 45 && values.excitement >= 10) return Mood::Suspicious;
    if (playStreak >= config::PLAYFUL_MIN_POKES
        && values.excitement >= config::PLAYFUL_EXCITEMENT_THRESHOLD && values.mischief >= 35) return Mood::Playful;
    if (values.curiosity >= 45 && values.excitement >= 10) return Mood::Curious;
    return Mood::Happy;
  }

  constexpr Behavior behavior() const {
    // Snapshot once per activation, so a stroke never changes speed halfway.
    const Mood current = mood();
    Behavior b = {config::REACTION_MS, config::STEP_MS, 20, 0, 18};
    if (current == Mood::Curious) b = {180, 20, 32, 250, 20};
    if (current == Mood::Suspicious) b = {0, 25, 65, 850, 32};
    if (current == Mood::Annoyed) b = {0, 5, 5, 0, 7};
    if (current == Mood::Furious) b = {config::ANGRY_REACTION_MS, config::ANGRY_STEP_MS, 3, 0, 5};
    if (current == Mood::Tired) b = {300, 65, 65, 0, 75};
    if (current == Mood::Sleepy || current == Mood::Asleep) b = {600, 45, 45, 350, 55};
    // Keep angry responses brisk; stamina still selects the tired routine.
    if (current != Mood::Annoyed && current != Mood::Furious) {
      b.stepMs += (100 - values.energy) / 20;
      b.returnMs += (100 - values.energy) / 20;
    }
    b.stepMs = armInterval(b.stepMs);
    b.approachMs = armInterval(b.approachMs);
    b.returnMs = armInterval(b.returnMs);
    b.reactionMs /= 3;
    b.pauseMs /= 3;
    // Approach with character, then slam the switch and snap back.
    // Direct commands request available servo speed, not instantaneous arrival.
    if (current != Mood::Tired && current != Mood::Sleepy && current != Mood::Asleep) {
      b.stepMs = b.returnMs = 0;
    }
    if (current == Mood::Curious) {
      b.reactionMs = 0;
      b.approachMs = 6;
      b.pauseMs = 120;
    }
    if (current == Mood::Suspicious) {
      b.approachMs = 8;
      b.pauseMs = 250;
    }
    if (current == Mood::Playful) {
      b.reactionMs = 0;
      b.approachMs = 0;
      b.peekDeg = config::MISCHIEF_PEEK_DEG;
      b.pauseMs = config::MISCHIEF_PEEK_PAUSE_MS;
    }
    if (current == Mood::Tired) {
      b.approachMs = config::TIRED_CREEP_MS;
      b.pauseMs = config::TIRED_PAUSE_MS;
      b.peekDeg = config::TIRED_PEEK_DEG;
      b.stepMs = 0; // Fatigue slows the approach, never the actual switch push.
    }
    if (current == Mood::Happy || current == Mood::Annoyed || current == Mood::Furious) {
      b.reactionMs = 0;
    }
    // Rotate expressive variants per activation; keep every actual push decisive.
    const uint8_t variant = activations ? (activations - 1) % 3 : 0;
    if (current == Mood::Happy && variant) {
      b.approachMs = 3;
      b.pauseMs = variant * 100;
    }
    if (current == Mood::Curious && variant) {
      b.approachMs = variant == 1 ? 3 : 0;
      b.pauseMs = variant == 1 ? 240 : 350;
    }
    if (current == Mood::Suspicious && variant) {
      b.approachMs = variant == 1 ? 5 : 10;
      b.pauseMs = variant == 1 ? 400 : 550;
    }
    if (current == Mood::Playful) {
      b.pauseMs += variant * 200;
      b.hideWaitMs += variant * 200;
    }
    if (current == Mood::Happy) { b.routine = Routine::Greeting; b.pauseMs = 250; }
    if (current == Mood::Curious) b.routine = Routine::Inspect;
    if (current == Mood::Suspicious) b.routine = Routine::Distrust;
    if (current == Mood::Playful) b.routine = Routine::FakeOut;
    if (current == Mood::Furious) b.routine = Routine::DoubleJab;
    if (current == Mood::Tired || current == Mood::Sleepy || current == Mood::Asleep) b.routine = Routine::Creep;
    return b;
  }
};
static_assert(config::ANGRY_STEP_MS > 0, "Motion interval must be positive");
}
