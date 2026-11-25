#pragma once
#include <stdint.h>

namespace config {
constexpr uint8_t SWITCH_PIN = 4; // Switch closes to GND when ON.
constexpr uint8_t ARM_SERVO_PIN = 9;
constexpr uint8_t FACE_PIN = 7;
constexpr uint8_t PIEZO_PIN = 8;
constexpr bool SOUND_ENABLED = true; // Set false to mute.
static_assert(PIEZO_PIN != SWITCH_PIN && PIEZO_PIN != ARM_SERVO_PIN && PIEZO_PIN != FACE_PIN,
    "Piezo must have its own pin");
constexpr uint8_t FACE_BRIGHTNESS = 10; // 0..255; deliberately dim for a breadboard.
constexpr bool MATRIX_SERPENTINE = false; // Video confirms straight rows; alternating reversal broke the Z.
constexpr uint8_t MATRIX_ROTATION = 0; // Clockwise quarter-turns: 0, 1, 2, 3.
constexpr bool MATRIX_MIRROR = false; // Mirror horizontally before rotation.
constexpr bool SLEEP_Z_STATIC_TEST = false; // true freezes Z motion to diagnose panel orientation.
constexpr uint32_t RECOVERY_DELAY_MS = 2000; // Quiet time before recovery starts.
constexpr uint32_t RAPID_POKE_WINDOW_MS = 8000; // Keep repeated-poking sensitivity separate.
constexpr uint8_t ANNOYED_THRESHOLD = 55;
constexpr uint8_t FURIOUS_THRESHOLD = 75;
constexpr uint8_t ENERGY_RECOVERY_PER_SECOND = 4;
constexpr uint8_t ANNOYANCE_RECOVERY_PER_SECOND = 20;
constexpr uint32_t ANNOYANCE_RECOVERY_DELAY_MS = 2000;
constexpr uint8_t PLAYFUL_EXCITEMENT_THRESHOLD = 60;
constexpr uint8_t PLAYFUL_MIN_POKES = 3;
constexpr uint8_t ENERGY_COST_SPACED = 4;
constexpr uint8_t ENERGY_COST_REPEATED = 8;
constexpr uint8_t PATIENCE_RECOVERY_PER_SECOND = 5;
constexpr uint8_t EXCITEMENT_DECAY_PER_SECOND = 4;
constexpr uint8_t CURIOSITY_RECOVERY_PER_SECOND = 2;
constexpr uint8_t MISCHIEF_RECOVERY_PER_SECOND = 3;
constexpr uint32_t SLEEPY_AFTER_MS = 30000;
constexpr uint32_t SLEEP_AFTER_MS = 60000;
constexpr uint32_t ANGRY_REACTION_MS = 0;
constexpr uint32_t ANGRY_STEP_MS = 3;
static_assert(MATRIX_ROTATION < 4, "Rotation must be 0..3");
// Start with the horn removed. Adjust these to match your mechanism.
constexpr int HOME_DEG = 180; // Resting/sleep position: matches the working Arduino sketch.
constexpr int PUSH_DEG = 60; // Switch actuation position for the assembled box.
constexpr int TIRED_PEEK_DEG = 100;
constexpr uint16_t TIRED_CREEP_MS = 18; // One degree per step; ~1.4 seconds from rest to peek.
constexpr uint16_t TIRED_PAUSE_MS = 900;
constexpr int MISCHIEF_PEEK_DEG = 80;
constexpr uint16_t MISCHIEF_PEEK_PAUSE_MS = 700; // Includes time to reach the fast peek target.
constexpr uint16_t MISCHIEF_HIDE_WAIT_MS = 650;
static_assert((HOME_DEG > PUSH_DEG && MISCHIEF_PEEK_DEG < HOME_DEG && MISCHIEF_PEEK_DEG > PUSH_DEG)
    || (HOME_DEG < PUSH_DEG && MISCHIEF_PEEK_DEG > HOME_DEG && MISCHIEF_PEEK_DEG < PUSH_DEG),
    "Mischief peek must be between rest and push");
static_assert((HOME_DEG > PUSH_DEG && TIRED_PEEK_DEG < HOME_DEG && TIRED_PEEK_DEG > PUSH_DEG)
    || (HOME_DEG < PUSH_DEG && TIRED_PEEK_DEG > HOME_DEG && TIRED_PEEK_DEG < PUSH_DEG),
    "Tired peek must be between rest and push");
constexpr uint16_t ARM_STEP_PERCENT = 10; // Scale the remaining ramped movements.
static_assert(ARM_STEP_PERCENT > 0 && ARM_STEP_PERCENT <= 100, "Arm speed scale must be 1..100 percent");
constexpr uint32_t DEBOUNCE_MS = 30;
constexpr uint16_t ANNOYED_IGNORE_MS = 5000;
constexpr uint8_t PLAIN_START_ACTIVATIONS = 2; // Simple push/return before personality performances.
constexpr uint16_t RESPONSE_PAUSE_MS = 400; // Let the interaction sound lead every arm response.
constexpr uint32_t REACTION_MS = 250; // Happy reaction pause.
constexpr uint32_t STEP_MS = 20; // One degree per step; larger is slower.
constexpr uint32_t PUSH_HOLD_MS = 500;
constexpr uint16_t GESTURE_SETTLE_MS = 250;
constexpr uint16_t FURIOUS_RECOIL_MS = 300;
constexpr uint32_t HOME_SETTLE_MS = 500;
constexpr uint32_t STARTUP_SETTLE_MS = 2400;
static_assert(HOME_DEG >= 0 && HOME_DEG <= 180, "Invalid home angle");
static_assert(PUSH_DEG >= 0 && PUSH_DEG <= 180, "Invalid push angle");
static_assert(HOME_DEG != PUSH_DEG, "Arm needs two different positions");
constexpr bool pinsUnique() {
  const uint8_t pins[] = {SWITCH_PIN, ARM_SERVO_PIN, FACE_PIN, PIEZO_PIN};
  for (uint8_t i=0;i<4;++i) for (uint8_t j=i+1;j<4;++j) if (pins[i] == pins[j]) return false;
  return true;
}
static_assert(pinsUnique(), "Each connected device needs a unique pin");
}
