#pragma once
#include <stdint.h>

namespace config {
constexpr uint8_t SWITCH_PIN = 4; // Switch closes to GND when ON.
constexpr uint8_t ARM_SERVO_PIN = 9;
// Start with the horn removed. Adjust these to match your mechanism.
constexpr int HOME_DEG = 60;
constexpr int PUSH_DEG = 110;
constexpr uint32_t DEBOUNCE_MS = 30;
constexpr uint32_t REACTION_MS = 400;
constexpr uint32_t STEP_MS = 20; // One degree per step; larger is slower.
constexpr uint32_t PUSH_HOLD_MS = 500;
constexpr uint32_t HOME_SETTLE_MS = 500;
constexpr uint32_t STARTUP_SETTLE_MS = 1000;
static_assert(HOME_DEG >= 0 && HOME_DEG <= 180, "Invalid home angle");
static_assert(PUSH_DEG >= 0 && PUSH_DEG <= 180, "Invalid push angle");
static_assert(HOME_DEG != PUSH_DEG, "Arm needs two different positions");
static_assert(STEP_MS > 0, "Step interval must be positive");
}
