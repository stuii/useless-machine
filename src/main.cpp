#include <Arduino.h>
#include <Servo.h>
#include "config.h"

namespace {
Servo arm;
enum class State { Startup, Idle, Reacting, Extending, Holding, Retracting, Settling, WaitOff };
State state = State::Startup;
uint32_t stateSince = 0;
uint32_t rawSince = 0;
uint32_t lastStep = 0;
bool rawOn = false;
bool stableOn = false;
int angle = config::HOME_DEG;

bool elapsed(uint32_t now, uint32_t since, uint32_t duration) {
  return static_cast<uint32_t>(now - since) >= duration;
}

void enter(State next, uint32_t now, const __FlashStringHelper* message) {
  state = next;
  stateSince = now;
  lastStep = now;
  Serial.println(message);
}

void updateSwitch(uint32_t now) {
  const bool sample = digitalRead(config::SWITCH_PIN) == LOW;
  if (sample != rawOn) {
    rawOn = sample;
    rawSince = now;
  }
  if (stableOn != rawOn && elapsed(now, rawSince, config::DEBOUNCE_MS)) {
    stableOn = rawOn;
    Serial.println(stableOn ? F("Switch ON") : F("Switch OFF"));
  }
}

bool moveTo(int target, uint32_t now) {
  if (angle != target && elapsed(now, lastStep, config::STEP_MS)) {
    lastStep = now;
    angle += target > angle ? 1 : -1;
    arm.write(angle);
  }
  return angle == target;
}
}

void setup() {
  Serial.begin(115200);
  pinMode(config::SWITCH_PIN, INPUT_PULLUP);
  // Queue home before enabling pulses to avoid a default center command.
  arm.write(config::HOME_DEG);
  arm.attach(config::ARM_SERVO_PIN);
  rawOn = digitalRead(config::SWITCH_PIN) == LOW;
  rawSince = millis();
  enter(State::Startup, millis(), F("Useless machine: D4 switch, D9 arm. Homing."));
}

void loop() {
  const uint32_t now = millis();
  updateSwitch(now);
  switch (state) {
    case State::Startup:
      if (elapsed(now, stateSince, config::STARTUP_SETTLE_MS))
        enter(State::Idle, now, F("Ready"));
      break;
    case State::Idle:
      if (stableOn) enter(State::Reacting, now, F("Thinking..."));
      break;
    case State::Reacting:
      if (!stableOn) enter(State::Idle, now, F("Cancelled"));
      else if (elapsed(now, stateSince, config::REACTION_MS))
        enter(State::Extending, now, F("Pushing"));
      break;
    case State::Extending:
      if (!stableOn) enter(State::Retracting, now, F("Switch off; retracting"));
      else if (moveTo(config::PUSH_DEG, now))
        enter(State::Holding, now, F("At push position"));
      break;
    case State::Holding:
      if (!stableOn || elapsed(now, stateSince, config::PUSH_HOLD_MS))
        enter(State::Retracting, now, F("Retracting"));
      break;
    case State::Retracting:
      if (moveTo(config::HOME_DEG, now))
        enter(State::Settling, now, F("Home"));
      break;
    case State::Settling:
      if (elapsed(now, stateSince, config::HOME_SETTLE_MS))
        enter(State::WaitOff, now, F("Waiting for switch OFF to rearm"));
      break;
    case State::WaitOff:
      // A missed switch gets one attempt, never endless retries.
      if (!stableOn) enter(State::Idle, now, F("Ready"));
      break;
  }
}
