#include <Arduino.h>
#include <Servo.h>
#include "config.h"
#include "face.h"
#include "sound.h"
#include "personality.h"
#include "interaction.h"
#include "choreography.h"

namespace {
Servo arm;
using personality::Routine;
enum class State { Startup, Idle, ResponsePause, Preparing, Prepared, Reacting, Approaching, Peeking,
  GreetingOut, GreetingPause, GreetingBack, GreetingBackPause,
  InspectCloser, InspectPause, SuspiciousBack, SuspiciousPause,
  Extending, Holding, Recoil, RecoilPause, Retracting, Settling, MischiefWait, WaitOff, ScriptMove, ScriptPause, Anticipate, AnticipatePause, FakeSleep, FakePeek };
State state = State::Startup;
uint32_t stateSince=0, rawSince=0, lastStep=0, lastRestTick=0, lastReport=0;
bool rawOn=false, stableOn=false, startupComplete=false;
bool feintPending=false;
uint8_t jabs=0;
uint8_t quickStreak=0, playfulEncounters=0, victories=0;
bool anticipate=false, pretendSleep=false, victoryPending=false, celebrating=false;
uint32_t victorySince=0;
uint8_t nextVariant[choreography::ROUTINES]={1,1,1,1,1,1,1};
uint8_t motionVariant=0, scriptIndex=0;
choreography::Step scriptStep={};
int angle=config::HOME_DEG;
personality::Model mind;
interaction::Gate switchGate;
personality::Behavior cycleBehavior=mind.behavior();
personality::Mood reportedMood=personality::Mood::Happy;
// All gestures stay inside the calibrated travel, away from the push endpoint.
constexpr int gestureAngle(uint8_t percent) {
  return config::HOME_DEG + (config::PUSH_DEG-config::HOME_DEG)*percent/100;
}
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

bool elapsed(uint32_t now,uint32_t since,uint32_t duration) {
  return static_cast<uint32_t>(now-since)>=duration;
}
void enter(State next,uint32_t now,const __FlashStringHelper* message) {
  state=next; stateSince=now; lastStep=now; Serial.println(message);
}
void updateSwitch(uint32_t now) {
  const bool sample=digitalRead(config::SWITCH_PIN)==LOW;
  if(sample!=rawOn){rawOn=sample;rawSince=now;}
  if(stableOn!=rawOn && elapsed(now,rawSince,config::DEBOUNCE_MS)){
    stableOn=rawOn; Serial.println(stableOn?F("Switch ON"):F("Switch OFF"));
  }
}
bool moveTo(int target,uint32_t now,uint16_t stepMs) {
  if(stepMs==0){if(angle!=target)arm.write(target);angle=target;return true;}
  if(angle!=target && elapsed(now,lastStep,stepMs)){
    lastStep=now;angle+=target>angle?1:-1;arm.write(angle);
  }
  return angle==target;
}
void beginScriptStep(uint32_t now) {
  scriptStep=choreography::read(cycleBehavior.routine,motionVariant,scriptIndex);
  enter(State::ScriptMove,now,F("Gesture"));
}
void startResponse(uint32_t now) {
  feintPending=false; jabs=0;
  if(anticipate){enter(State::Anticipate,now,F("I saw that coming"));return;}
  if(pretendSleep){enter(State::FakeSleep,now,F("Definitely asleep..."));return;}
  // Ordinary activations rotate through the selected variants.
  if(cycleBehavior.routine==Routine::Creep){
    const bool beyondPeek=config::HOME_DEG>config::PUSH_DEG
        ? angle<=cycleBehavior.peekDeg : angle>=cycleBehavior.peekDeg;
    enter(beyondPeek?State::Preparing:State::Reacting,now,F("Sleepy approach"));
  } else enter(State::Reacting,now,F("New response"));
}
}
void setup(){
  digitalWrite(LED_BUILTIN,LOW);pinMode(LED_BUILTIN,OUTPUT);
  Serial.begin(115200);pinMode(config::SWITCH_PIN,INPUT_PULLUP);
  mind.startAsleep();face::begin();sound::begin();
  pinMode(config::ARM_SERVO_PIN,OUTPUT);digitalWrite(config::ARM_SERVO_PIN,LOW);
  arm.write(config::HOME_DEG);arm.attach(config::ARM_SERVO_PIN);
  rawOn=digitalRead(config::SWITCH_PIN)==LOW;rawSince=millis();
  enter(State::Startup,millis(),F("D4 switch; D9 arm. Rest 180, push 60. Homing."));
}
void loop(){
  const uint32_t now=millis();
  const bool wasOn=stableOn;
  updateSwitch(now);
  // Only an OFF edge while actually pushing qualifies as a win.
  if(wasOn && !stableOn && state==State::Holding && victories<3)victoryPending=true;
  if(state!=State::Idle || stableOn || rawOn){
    if(startupComplete && state!=State::Idle)mind.disturbed();
    lastRestTick=now;
  } else while(elapsed(now,lastRestTick,1000)){mind.restSecond();lastRestTick+=1000;}
  if(switchGate.accept(stableOn,now,!startupComplete)){
    quickStreak=switchGate.quick(now) ? (quickStreak%3+1) : 0;
    mind.activate();cycleBehavior=mind.behavior();
    anticipate=quickStreak>=3 && quickStreak%3==0 && cycleBehavior.routine!=Routine::Creep;
    pretendSleep=false;
    if(cycleBehavior.routine==Routine::FakeOut){
      playfulEncounters=(playfulEncounters+1)%2;
      pretendSleep=playfulEncounters==0 && !anticipate;
    }
    victoryPending=false;celebrating=false;
    // Rotate per mood, not globally: returning to a mood gives its next distinct routine.
    const uint8_t routine=static_cast<uint8_t>(cycleBehavior.routine);
    motionVariant=nextVariant[routine];
    nextVariant[routine]=(motionVariant+1)%3;
    sound::play(mind.mood(),now);reportMood();
    Serial.print(F("Motion variant="));Serial.println(motionVariant+1);
    feintPending=false; jabs=0;
    enter(State::ResponsePause,now,F("Listening..."));
  }
  // One cancellation path covers every gesture, pause and pending second jab.
  if(startupComplete && !stableOn && state!=State::Idle && state!=State::WaitOff
      && state!=State::Retracting && state!=State::Settling){
    feintPending=false;enter(State::Retracting,now,F("Switch off; returning"));
  }
  switch(state){
    case State::Startup:
      if(elapsed(now,stateSince,config::STARTUP_SETTLE_MS)){
        startupComplete=true;enter(stableOn?State::WaitOff:State::Idle,now,F("Ready"));
      }
      break;
    case State::Idle:break;
    case State::Anticipate:
      if(moveTo(80,now,0))enter(State::AnticipatePause,now,F("Already waiting for you"));
      break;
    case State::AnticipatePause:
      if(elapsed(now,stateSince,550))enter(State::Extending,now,F("Called it"));
      break;
    case State::FakeSleep:
      if(moveTo(config::HOME_DEG,now,0) && elapsed(now,stateSince,1300))
        enter(State::FakePeek,now,F("One eye open"));
      break;
    case State::FakePeek:
      if(elapsed(now,stateSince,650))enter(State::Extending,now,F("Fooled you"));
      break;
    case State::ResponsePause:
      if(elapsed(now,stateSince,config::RESPONSE_PAUSE_MS))startResponse(now);
      break;
    case State::Preparing:
      if(moveTo(config::HOME_DEG,now,cycleBehavior.returnMs))enter(State::Prepared,now,F("Resetting tired approach"));
      break;
    case State::Prepared:
      if(elapsed(now,stateSince,config::HOME_SETTLE_MS))enter(State::Reacting,now,F("Ready to creep"));
      break;
    case State::Reacting:
      if(elapsed(now,stateSince,cycleBehavior.reactionMs)){
        if(motionVariant) {scriptIndex=0;beginScriptStep(now);}
        else if(cycleBehavior.routine==Routine::Greeting)enter(State::GreetingOut,now,F("Hello!"));
        else if(cycleBehavior.routine==Routine::Swat || cycleBehavior.routine==Routine::DoubleJab)
          enter(State::Extending,now,F("Swat"));
        else enter(State::Approaching,now,F("Taking a look"));
      }
      break;
    case State::ScriptMove:
      if(moveTo(cycleBehavior.routine==Routine::FakeOut && scriptStep.percent==83
          ? config::MISCHIEF_PEEK_DEG : gestureAngle(scriptStep.percent),now,scriptStep.stepMs))
        enter(State::ScriptPause,now,F("Gesture pause"));
      break;
    case State::ScriptPause:
      if(elapsed(now,stateSince,scriptStep.waitMs)) {
        if(++scriptIndex<choreography::count(cycleBehavior.routine,motionVariant))beginScriptStep(now);
        else enter(cycleBehavior.routine==Routine::Creep?State::Approaching:State::Extending,now,F("Enough theatre"));
      }
      break;
    case State::GreetingOut:
      if(moveTo(gestureAngle(17),now,0))enter(State::GreetingPause,now,F("Greeting bob"));
      break;
    case State::GreetingPause:
      if(elapsed(now,stateSince,config::GESTURE_SETTLE_MS))enter(State::GreetingBack,now,F("Bob back"));
      break;
    case State::GreetingBack:
      if(moveTo(config::HOME_DEG,now,0))enter(State::GreetingBackPause,now,F("Ready..."));
      break;
    case State::GreetingBackPause:
      if(elapsed(now,stateSince,config::GESTURE_SETTLE_MS))enter(State::Extending,now,F("Happy push"));
      break;
    case State::Approaching:
      if(moveTo(cycleBehavior.peekDeg,now,cycleBehavior.approachMs))enter(State::Peeking,now,F("Watching..."));
      break;
    case State::Peeking:
      if(elapsed(now,stateSince,cycleBehavior.pauseMs)){
        if(cycleBehavior.routine==Routine::FakeOut){feintPending=true;enter(State::Retracting,now,F("Pretending to give up"));}
        else if(cycleBehavior.routine==Routine::Inspect)enter(State::InspectCloser,now,F("A closer look"));
        else if(cycleBehavior.routine==Routine::Distrust)enter(State::SuspiciousBack,now,F("Backing away"));
        else enter(State::Extending,now,F("Final slam"));
      }
      break;
    case State::InspectCloser:
      if(moveTo(gestureAngle(75),now,6))enter(State::InspectPause,now,F("Inspecting closely"));
      break;
    case State::InspectPause:
      if(elapsed(now,stateSince,300))enter(State::Extending,now,F("Decision made"));
      break;
    case State::SuspiciousBack:
      if(moveTo(gestureAngle(25),now,6))enter(State::SuspiciousPause,now,F("Not sure..."));
      break;
    case State::SuspiciousPause:
      if(elapsed(now,stateSince,400))enter(State::Extending,now,F("Surprise lunge"));
      break;
    case State::Extending:
      if(moveTo(config::PUSH_DEG,now,cycleBehavior.stepMs)){++jabs;enter(State::Holding,now,F("Push"));}
      break;
    case State::Holding:
      if(elapsed(now,stateSince,config::PUSH_HOLD_MS)){
        if(cycleBehavior.routine==Routine::DoubleJab && jabs<2)enter(State::Recoil,now,F("One more jab"));
        else {feintPending=false;enter(State::Retracting,now,F("Returning"));}
      }
      break;
    case State::Recoil:
      if(moveTo(gestureAngle(60),now,0))enter(State::RecoilPause,now,F("Recoil"));
      break;
    case State::RecoilPause:
      if(elapsed(now,stateSince,config::FURIOUS_RECOIL_MS))enter(State::Extending,now,F("Second jab"));
      break;
    case State::Retracting:
      if(moveTo(config::HOME_DEG,now,cycleBehavior.returnMs))enter(State::Settling,now,F("Home"));
      break;
    case State::Settling:
      if(elapsed(now,stateSince,config::HOME_SETTLE_MS)){
        if(feintPending && stableOn)enter(State::MischiefWait,now,F("Hiding..."));
        else {
          feintPending=false;
          if(!stableOn && victoryPending && victories<3){
            ++victories;celebrating=true;victorySince=now;
          }
          victoryPending=false;
          enter(stableOn?State::WaitOff:State::Idle,now,F("Stowed"));
        }
      }
      break;
    case State::MischiefWait:
      if(elapsed(now,stateSince,cycleBehavior.hideWaitMs)){
        feintPending=false;enter(State::Extending,now,F("Surprise slam"));
      }
      break;
    case State::WaitOff:
      if(!stableOn)enter(State::Idle,now,F("Ready"));
      break;
  }
  if(mind.mood()!=reportedMood || elapsed(now,lastReport,5000)){reportMood();lastReport=now;}
  if(!stableOn || state==State::Idle || state==State::WaitOff)sound::stop();
  sound::update(now);
  if(celebrating && elapsed(now,victorySince,1400))celebrating=false;
  face::Expression expression=face::Expression::Normal;
  if(state==State::Anticipate || state==State::AnticipatePause)expression=face::Expression::Knowing;
  else if(state==State::FakeSleep)expression=face::Expression::FakeSleep;
  else if(state==State::FakePeek)expression=face::Expression::Peek;
  else if(celebrating && state==State::Idle)expression=face::Expression::Victory;
  face::expression(expression,expression==face::Expression::Victory?victorySince:stateSince,victories);
  face::update(now,mind.mood(),state==State::Peeking || state==State::InspectPause || state==State::SuspiciousPause || state==State::ScriptPause);
}
