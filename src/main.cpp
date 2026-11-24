#include <Arduino.h>
#include <Servo.h>
#include "config.h"
#include "face.h"
#include "sound.h"
#include "personality.h"
#include "interaction.h"

namespace {
Servo arm;
Servo lid;
enum class State { Startup, StartupArm, Idle, OpeningLid, LidReady, ClosingLid, ClosedReady, Reacting, Approaching, Peeking, Extending, Holding, Retracting, Settling, WaitOff, MischiefWait };
enum class Feint { None, Peek, Stow, Wait, Slam };
Feint feint = Feint::None;
State state = State::Startup;
uint32_t stateSince = 0;
uint32_t rawSince = 0;
uint32_t lastStep = 0;
bool rawOn = false;
bool stableOn = false;
int angle = config::HOME_DEG;
int lidAngle = config::LID_OPEN_DEG;
bool startupComplete = false;
bool lidReady = false;
bool cycleWanted = false;
bool quickCycle = false;
personality::Model mind;
interaction::Gate switchGate;
personality::Behavior cycleBehavior = mind.behavior();
uint32_t lastRestTick = 0;
uint32_t lastReport = 0;
personality::Mood reportedMood = personality::Mood::Happy;

void reportMood() {
  Serial.print(F("Mood="));
  switch (mind.mood()) {
    case personality::Mood::Happy: Serial.print(F("happy")); break;
    case personality::Mood::Suspicious: Serial.print(F("suspicious")); break;
    case personality::Mood::Curious: Serial.print(F("curious")); break;
    case personality::Mood::Playful: Serial.print(F("playful")); break;
    case personality::Mood::Annoyed: Serial.print(F("annoyed")); break;
    case personality::Mood::Furious: Serial.print(F("furious")); break;
    case personality::Mood::Tired: Serial.print(F("tired")); break;
    case personality::Mood::Sleepy: Serial.print(F("sleepy")); break;
    case personality::Mood::Asleep: Serial.print(F("asleep")); break;
  }
  Serial.print(F(" energy=")); Serial.print(mind.values.energy);
  Serial.print(F(" annoyance=")); Serial.print(mind.values.annoyance);
  Serial.print(F(" excitement=")); Serial.print(mind.values.excitement);
  Serial.print(F(" curiosity=")); Serial.print(mind.values.curiosity);
  Serial.print(F(" patience=")); Serial.print(mind.values.patience);
  Serial.print(F(" mischief=")); Serial.println(mind.values.mischief);
  reportedMood = mind.mood();
}

bool elapsed(uint32_t now, uint32_t since, uint32_t duration) {
  return static_cast<uint32_t>(now - since) >= duration;
}

void enter(State next, uint32_t now, const __FlashStringHelper* message) {
  if (next == State::OpeningLid && !lid.attached()) {
    // Restore the last command before pulses resume; avoid an attach-time jump.
    lid.write(lidAngle);
    lid.attach(config::LID_SERVO_PIN);
  }
  if (next == State::OpeningLid || next == State::ClosingLid) lidReady = false;
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

bool moveAccessory(Servo& servo, int& position, int target, uint32_t now, uint16_t stepMs) {
  if (position != target && elapsed(now, lastStep, stepMs)) {
    lastStep = now;
    position += target > position ? 1 : -1;
    servo.write(position);
  }
  return position == target;
}

void startResponse(uint32_t now) {
  if (!cycleWanted || !stableOn) {
    enter(State::Retracting, now, F("Cancelled; stowing"));
  } else if (lidReady) {
    enter(quickCycle || feint == Feint::Slam ? State::Extending : State::Reacting, now, F("Arm ready"));
    if (quickCycle) lastStep = now - cycleBehavior.stepMs;
  } else if (state == State::Idle || state == State::WaitOff || state == State::MischiefWait) {
    // Already retracted and settled: don't repeat a home wait on every poke.
    enter(State::OpeningLid, now, F("Opening lid"));
  } else {
    // Retract and settle before moving the lid, even on rapid repeats.
    enter(State::Retracting, now, F("Preparing mechanism"));
  }
}

bool moveTo(int target, uint32_t now, uint16_t stepMs) {
  if (stepMs == 0) {
    // Command full available servo speed; physical arrival still takes time.
    if (angle != target) arm.write(target);
    angle = target;
    return true;
  }
  if (angle != target && elapsed(now, lastStep, stepMs)) {
    lastStep = now;
    angle += target > angle ? 1 : -1;
    arm.write(angle);
  }
  return angle == target;
}
}

void setup() {
  digitalWrite(LED_BUILTIN, LOW);
  pinMode(LED_BUILTIN, OUTPUT);
  Serial.begin(115200);
  pinMode(config::SWITCH_PIN, INPUT_PULLUP);
  mind.startAsleep();
  face::begin();
  sound::begin();
  // Open the lid first; enable the arm only after startup clearance time.
  pinMode(config::ARM_SERVO_PIN, OUTPUT);
  digitalWrite(config::ARM_SERVO_PIN, LOW);
  arm.write(config::HOME_DEG);
  lid.write(config::LID_OPEN_DEG);
  lid.attach(config::LID_SERVO_PIN);
  rawOn = digitalRead(config::SWITCH_PIN) == LOW;
  rawSince = millis();
  enter(State::Startup, millis(), F("Useless machine: D4 switch; D9 arm, D11 lid. Opening lid."));
}

void loop() {
  const uint32_t now = millis();
  updateSwitch(now);
  if (state != State::Idle || stableOn || rawOn) {
    if (startupComplete && state != State::Idle) mind.disturbed();
    lastRestTick = now;
  } else {
    // Unsigned subtraction remains correct across millis() rollover.
    while (elapsed(now, lastRestTick, 1000)) {
      mind.restSecond();
      lastRestTick += 1000;
    }
  }
  // A fresh debounced OFF->ON may preempt retraction or home settling.
  // A continuously held ON never retriggers, even after a missed push.
  if (switchGate.accept(stableOn, now, !startupComplete)) {
    quickCycle = switchGate.quick(now);
    cycleWanted = true;
    mind.activate();
    cycleBehavior = mind.behavior();
    // A fresh poke replaces any old fake-out; quick repeats retain immediate response.
    feint = mind.mood() == personality::Mood::Playful && !quickCycle ? Feint::Peek : Feint::None;
    sound::play(mind.mood(), now);
    reportMood();
    if (quickCycle) {
      cycleBehavior.reactionMs = 0;
      cycleBehavior.pauseMs = 0;
    }
    startResponse(now);
  }
  if (!stableOn) {
    cycleWanted = false;
    feint = Feint::None;
  }
  switch (state) {
    case State::Startup:
      if (elapsed(now, stateSince, config::STARTUP_SETTLE_MS)) {
        lidReady = true;
        arm.attach(config::ARM_SERVO_PIN);
        enter(State::StartupArm, now, F("Homing arm"));
      }
      break;
    case State::StartupArm:
      if (elapsed(now, stateSince, config::STARTUP_ARM_SETTLE_MS))
        enter(State::ClosingLid, now, F("Closing lid"));
      break;
    case State::OpeningLid:
      if (moveAccessory(lid, lidAngle, config::LID_OPEN_DEG, now, config::LID_STEP_MS))
        enter(State::LidReady, now, F("Lid settling"));
      break;
    case State::LidReady:
      if (elapsed(now, stateSince, config::LID_SETTLE_MS)) {
        lidReady = true;
        startResponse(now);
      }
      break;
    case State::ClosingLid:
      if (moveAccessory(lid, lidAngle, config::LID_CLOSED_DEG, now, config::LID_STEP_MS))
        enter(State::ClosedReady, now, F("Lid settling closed"));
      break;
    case State::ClosedReady:
      if (elapsed(now, stateSince, config::LID_SETTLE_MS)) {
        // Never fight the closed lid's stop throughout idle/sleep or WaitOff.
        // Keep torque while open: releasing there could drop the lid on the arm.
        lid.detach();
        digitalWrite(config::LID_SERVO_PIN, LOW);
        startupComplete = true;
        if (feint == Feint::Stow && cycleWanted && stableOn) {
          feint = Feint::Wait;
          enter(State::MischiefWait, now, F("Just kidding..."));
        } else {
          feint = Feint::None;
          enter(stableOn ? State::WaitOff : State::Idle, now, F("Stowed"));
        }
      }
      break;
    case State::Idle:
      break;
    case State::Reacting:
      if (!stableOn) enter(State::Retracting, now, F("Cancelled; retracting"));
      else if (elapsed(now, stateSince, cycleBehavior.reactionMs))
        enter(cycleBehavior.pauseMs ? State::Approaching : State::Extending, now, F("Coming out"));
      break;
    case State::Approaching:
      if (!stableOn) enter(State::Retracting, now, F("Switch off; retracting"));
      else if (moveTo(cycleBehavior.peekDeg, now, cycleBehavior.approachMs))
        enter(State::Peeking, now, F("Watching the switch..."));
      break;
    case State::Peeking:
      if (!stableOn) enter(State::Retracting, now, F("Switch off; retracting"));
      else if (elapsed(now, stateSince, cycleBehavior.pauseMs)) {
        if (feint == Feint::Peek) {
          feint = Feint::Stow;
          enter(State::Retracting, now, F("Pretending to give up"));
        } else enter(State::Extending, now, F("Pushing"));
      }
      break;
    case State::Extending:
      if (!stableOn) enter(State::Retracting, now, F("Switch off; retracting"));
      else if (moveTo(config::PUSH_DEG, now, cycleBehavior.stepMs))
        enter(State::Holding, now, F("At push position"));
      break;
    case State::Holding:
      if (!stableOn || elapsed(now, stateSince, config::PUSH_HOLD_MS)) {
        cycleWanted = false;
        feint = Feint::None;
        enter(State::Retracting, now, F("Retracting"));
      }
      break;
    case State::Retracting:
      if (moveTo(config::HOME_DEG, now, cycleBehavior.returnMs))
        enter(State::Settling, now, F("Home"));
      break;
    case State::Settling:
      if (elapsed(now, stateSince, config::HOME_SETTLE_MS))
        enter(cycleWanted && feint != Feint::Stow ? State::OpeningLid : State::ClosingLid, now, F("Arm clear"));
      break;
    case State::MischiefWait:
      if (!stableOn || !cycleWanted) {
        feint = Feint::None;
        enter(State::Idle, now, F("Fake-out cancelled"));
      } else if (elapsed(now, stateSince, cycleBehavior.closedWaitMs)) {
        feint = Feint::Slam;
        enter(State::OpeningLid, now, F("Surprise push"));
      }
      break;
    case State::WaitOff:
      // A missed switch gets one attempt, never endless retries.
      if (!stableOn) enter(State::Idle, now, F("Ready"));
      break;
  }
  if (mind.mood() != reportedMood || elapsed(now, lastReport, 5000)) {
    reportMood();
    lastReport = now;
  }
  // Sound belongs only to an active switch interaction, never idle recovery.
  if (!stableOn || state == State::Idle || state == State::WaitOff) sound::stop();
  sound::update(now);
  face::update(now, mind.mood(), state == State::Reacting || state == State::Peeking);
}
