// Compile-only simulation tests using the same model as the firmware.
#include "personality.h"
#include "matrix.h"
using personality::Model;
using personality::Mood;
constexpr bool progression() {
  Model m;
  if (m.mood() != Mood::Happy) return false;
  m.activate(); if (m.mood() != Mood::Curious) return false;
  m.activate(); if (m.mood() == Mood::Playful) return false;
  m.activate(); if (m.mood() != Mood::Playful) return false;
  m.activate(); m.activate(); // Fourth/fifth pokes are still below anger threshold.
  if (m.mood()==Mood::Annoyed || m.mood()==Mood::Furious || m.mood()==Mood::Tired) return false;
  m.activate(); if (m.mood()!=Mood::Annoyed) return false; // Sixth.
  m.activate(); if (m.mood()!=Mood::Furious) return false; // Seventh.
  m.activate(); if (m.mood()!=Mood::Furious) return false; // Eighth.
  m.activate();
  return m.mood() == Mood::Tired;
}
static_assert(progression(), "Playful needs three pokes; sustained poking tires the machine by nine");
constexpr bool recovery() {
  Model m;
  for (int i = 0; i < 20; ++i) m.activate();
  const auto before = m.values;
  for (uint32_t ms = 1000; ms < config::RECOVERY_DELAY_MS; ms += 1000) m.restSecond();
  if (m.values.annoyance != before.annoyance || m.values.energy != before.energy) return false;
  m.restSecond();
  if (m.values.annoyance >= before.annoyance || m.values.energy <= before.energy) return false;
  for (int i = 0; i < 120; ++i) m.restSecond();
  if (m.values.annoyance != 0 || m.values.energy != 100) return false;
  if (m.mood() != Mood::Asleep) return false;
  m.activate();
  return m.mood() == Mood::Curious && m.values.annoyance == 3 && m.quietMs == 0;
}
static_assert(recovery(), "Quiet cooldown should restore all parameters and allow a gentle wake");
constexpr bool sleepAndDisturbance() {
  Model m;
  for (int i = 0; i < 30; ++i) m.restSecond();
  if (m.mood() != Mood::Sleepy) return false;
  m.disturbed();
  if (m.quietMs != 0 || m.mood() != Mood::Happy) return false;
  for (int i = 0; i < 60; ++i) m.restSecond();
  return m.mood() == Mood::Asleep;
}
static_assert(sleepAndDisturbance(), "Sleep depends on uninterrupted quiet time");
constexpr bool bounded() {
  Model m;
  for (int i = 0; i < 1000; ++i) m.activate();
  if (m.values.annoyance != 100 || m.values.energy != 0) return false;
  for (int i = 0; i < 1000; ++i) m.restSecond();
  return m.values.energy == 100 && m.values.annoyance == 0 && m.quietMs == 300000;
}
static_assert(bounded(), "Parameters and quiet timer must saturate");
constexpr bool mixedEmotions() {
  Model angry; angry.values.annoyance = 90;
  const auto fast = angry.behavior();
  angry.values.energy = 10;
  if (angry.mood() != Mood::Tired || angry.behavior().approachMs <= fast.approachMs) return false;
  Model curious; curious.activate();
  return fast.pauseMs < curious.behavior().pauseMs;
}
static_assert(mixedEmotions(), "Energy and fatigue must influence mood and motion, not only annoyance");
constexpr bool allLayoutsAreBijective() {
  for (int serpentine = 0; serpentine < 2; ++serpentine)
    for (int mirror = 0; mirror < 2; ++mirror)
      for (int rotation = 0; rotation < 4; ++rotation) {
        bool seen[64] = {};
        for (int y = 0; y < 8; ++y)
          for (int x = 0; x < 8; ++x) {
            const uint8_t pixel = matrix::index(x, y, serpentine, rotation, mirror);
            if (pixel >= 64 || seen[pixel]) return false;
            seen[pixel] = true;
          }
      }
  return true;
}
static_assert(allLayoutsAreBijective(), "Every layout must use every LED exactly once");
static_assert(matrix::index(0, 0, true, 0, false) == 0, "Origin");
static_assert(matrix::index(0, 1, true, 0, false) == 15, "Serpentine odd row");
static_assert(matrix::index(0, 1, false, 0, false) == 8, "Progressive odd row");
static_assert(matrix::index(0, 0, false, 1, false) == 7, "Clockwise rotation");
static_assert(matrix::index(0, 0, false, 0, true) == 7, "Horizontal mirror");

constexpr bool spacingMatters() {
  Model rapid; rapid.activate(); rapid.activate();
  Model spaced; spaced.activate();
  for (int i = 0; i < 20; ++i) spaced.restSecond();
  spaced.activate();
  if (rapid.values.annoyance <= spaced.values.annoyance) return false;
  spaced.activations = 65535; spaced.activate();
  return spaced.activations == 65535;
}
static_assert(spacingMatters(), "Quiet spacing reduces annoyance; lifetime count saturates");

#include "sound_patterns.h"
constexpr bool soundPatternsAreBounded() {
  for (uint8_t mood = 0; mood < 9; ++mood) {
    uint16_t total = 0;
    for (uint8_t i = 0; i < sound::NOTES_PER_PHRASE; ++i) {
      const auto n = sound::phrases[mood][i];
      if (n.ms < 20 || n.ms > 100) return false;
      if (i % 2 == 1 && n.fromHz != 0) return false; // LED update windows.
      if (i % 2 == 0 && (n.fromHz < 100 || n.fromHz > 4000)) return false;
      for (uint16_t t = 0; t <= n.ms; t += sound::PITCH_TICK_MS) {
        const uint16_t hz = sound::pitchAt(n, t);
        if (n.fromHz == 0 ? hz != 0 : (hz < 100 || hz > 4000)) return false;
      }
      total += n.ms;
    }
    if (total > 1000) return false;
  }
  return true;
}
static_assert(soundPatternsAreBounded(), "All moods need finite phrases with LED update gaps");
static_assert(!sound::noteFinished(0x10UL, 0xfffffff0UL, 33), "Note must not end early over rollover");
static_assert(sound::noteFinished(0x11UL, 0xfffffff0UL, 33), "Note must end across rollover");

static_assert(sound::pitchAt({1000,2000,40,0},20) == 1500, "Rising sweep midpoint");
static_assert(sound::pitchAt({2000,1000,40,0},20) == 1500, "Falling sweep midpoint must not underflow");
static_assert(sound::pitchAt({1000,2000,40,0},40) == 2000, "Sweep endpoint");
static_assert(sound::pitchAt({1000,1000,40,40},0) == 960, "Warble low");
static_assert(sound::pitchAt({1000,1000,40,40},8) == 1040, "Warble high");

constexpr bool newTraitsMatter() {
  Model patient; patient.activations = 1; patient.values.patience = 100;
  Model impatient = patient; impatient.values.patience = 0;
  patient.activate(); impatient.activate();
  if (impatient.values.annoyance <= patient.values.annoyance) return false;
  Model curious; curious.values.excitement = 20; curious.values.curiosity = 80;
  Model bored = curious; bored.values.curiosity = 10;
  if (curious.mood() != Mood::Curious || bored.mood() == Mood::Curious) return false;
  Model imp; imp.playStreak = 3; imp.values.excitement = 80; imp.values.mischief = 80; imp.activations = 2;
  Model serious = imp; serious.values.mischief = 0;
  if (imp.mood() != Mood::Playful || serious.mood() == Mood::Playful) return false;
  const auto teasing = imp.behavior();
  return teasing.peekDeg == config::MISCHIEF_PEEK_DEG && teasing.approachMs == 0
      && teasing.pauseMs >= config::MISCHIEF_PEEK_PAUSE_MS && teasing.stepMs == 0
      && teasing.returnMs == 0 && serious.behavior().pauseMs < teasing.pauseMs;
}
static_assert(newTraitsMatter(), "Patience, curiosity and mischief must each affect behavior");
constexpr bool traitsRecover() {
  Model m; for (int i=0; i<1000; ++i) m.activate();
  if (m.values.patience != 0 || m.values.curiosity != 0 || m.values.mischief != 0) return false;
  for (int i=0; i<1000; ++i) m.restSecond();
  return m.values.patience == 100 && m.values.curiosity == 70 && m.values.mischief == 25;
}
static_assert(traitsRecover(), "New traits must stay bounded and recover during quiet time");

#include "face_transition.h"
constexpr bool transitionPlayback() {
  faceAnimation::Transition t;
  t.request(3, 0xfffffff0UL);
  if (!t.due(0xfffffff0UL)) return false;
  t.presented(0xfffffff0UL);
  if (t.due(0x10UL) || !t.due(0x18UL)) return false;
  t.request(4, 0x18UL); // Do not restart the visible transition.
  if (t.target != 3 || t.frame != 1) return false;
  for (int i = 1; i <= faceAnimation::LAST_FRAME; ++i) t.presented(i * 40);
  if (t.active || t.current != 3) return false;
  t.request(4, 400);
  return t.active && t.current == 3 && t.target == 4 && t.frame == 0;
}
static_assert(transitionPlayback(), "Transitions finish, respect rollover, and then accept the latest mood");
static_assert(faceAnimation::blend(0).amount == 0, "Begin on old face");
static_assert(faceAnimation::blend(8).amount == 255 && faceAnimation::blend(8).second == 3, "End on new face");
static_assert(faceAnimation::mix(255,255,128) == 255, "Blend must not overflow on AVR");
static_assert(faceAnimation::mix(20,200,0) == 20 && faceAnimation::mix(20,200,255) == 200, "Exact color endpoints");

#include "interaction.h"
constexpr bool retriggerRules() {
  interaction::Gate g;
  if (g.accept(true, 0, true)) return false; // Startup cannot move.
  if (!g.accept(true, 1000, false)) return false;
  if (g.accept(true, 2000, false)) return false; // Stuck ON never loops.
  g.accept(false, 2100, false);
  if (!g.accept(true, 2130, false) || !g.quick(2130)) return false;
  if (g.accept(true, 2500, false)) return false;
  g.accept(false, 3000, false);
  if (!g.accept(true, 5000, false) || g.quick(5000)) return false;
  g.accept(false, 0xfffffff0UL, false);
  return g.accept(true, 0x20UL, false) && g.quick(0x20UL);
}
static_assert(retriggerRules(), "Fresh flips preempt motion; held ON is ignored; timestamps wrap safely");
static_assert(interaction::approachAngle(60,110) == 90, "Peek before contact");
static_assert(interaction::approachAngle(110,60) == 80, "Reverse-mounted arm peek");
constexpr bool distinctRoutines() {
  Model tired; tired.values.energy = 10;
  Model suspicious; suspicious.values.annoyance = 20; suspicious.values.curiosity = 60; suspicious.values.excitement = 20;
  Model angry; angry.values.annoyance = 60;
  const auto slow = tired.behavior(), peek = suspicious.behavior(), fast = angry.behavior();
  return suspicious.mood() == Mood::Suspicious && peek.pauseMs == 250 && peek.approachMs > peek.stepMs
      && peek.stepMs == 0 && peek.returnMs == 0
      && slow.stepMs == 0 && slow.returnMs > peek.returnMs
      && slow.approachMs > peek.approachMs && slow.pauseMs > peek.pauseMs
      && slow.peekDeg == config::TIRED_PEEK_DEG
      && fast.stepMs == 0 && fast.returnMs == 0
      && fast.reactionMs == 0 && fast.pauseMs == 0;
}
static_assert(distinctRoutines(), "Tired, suspicious and annoyed must use different motion routines");

constexpr bool fasterRecovery() {
  Model m; m.values.energy = 0; m.values.annoyance = 100;
  m.values.patience = 0; m.values.excitement = 100; m.values.curiosity = 0;
  m.values.mischief = 100;
  m.restSecond();
  if (m.values.energy != 0 || m.values.annoyance != 100) return false;
  m.restSecond();
  if (m.values.energy != 4 || m.values.annoyance != 80) return false;
  for (int i = 2; i < 26; ++i) m.restSecond();
  if (m.values.energy != 100 || m.values.annoyance != 0 || m.values.patience != 100 || m.values.mischief != 25 || m.mood() != Mood::Happy) return false;
  for (int i = 26; i < 36; ++i) m.restSecond();
  return m.values.curiosity == 70;
}
static_assert(fasterRecovery(), "Recovery starts after 2 seconds; stamina fully returns by 26 seconds");

#include "sleep_animation.h"
constexpr bool startSleepingAndWake() {
  Model m; m.startAsleep();
  if (m.mood() != Mood::Asleep) return false;
  m.restSecond();
  if (m.mood() != Mood::Asleep) return false;
  m.activate();
  return m.quietMs == 0 && m.mood() != Mood::Asleep && m.activations == 1;
}
static_assert(startSleepingAndWake(), "Boot asleep; switch activation wakes the personality");
constexpr bool sleepGlyphs() {
  bool bothVisible=false, complete[2]={false,false}, exited[2]={false,false};
  for(uint8_t phase=0;phase<sleepAnimation::Z_PHASES;++phase){
    uint8_t counts[2]={0,0}; bool moves=false;
    for(uint8_t y=0;y<8;++y)for(uint8_t x=0;x<8;++x){
      const bool a=sleepAnimation::streamZ(x,y,phase,0), b=sleepAnimation::streamZ(x,y,phase,1);
      if((y>=5 && (a||b)) || (a&&b))return false;
      counts[0]+=a;counts[1]+=b;
      if((a||b)!=sleepAnimation::floatingZ(x,y,(phase+1)%sleepAnimation::Z_PHASES))moves=true;
    }
    if(!moves || counts[0]+counts[1]==0)return false;
    if(counts[0] && counts[1])bothVisible=true;
    for(uint8_t i=0;i<2;++i){
      if(counts[i]==10)complete[i]=true;
      if(counts[i]==0)exited[i]=true;
    }
  }
  return bothVisible && complete[0] && complete[1] && exited[0] && exited[1];
}
static_assert(sleepGlyphs(), "Two readable Zs stream continuously offscreen without overlapping the face or each other");
#include "startup_animation.h"
static_assert(startupAnimation::frameAt(0)==0 && startupAnimation::frameAt(2399)==59 && startupAnimation::frameAt(2400)==59,"Startup frame bounds");
static_assert(config::STARTUP_SETTLE_MS>=startupAnimation::DURATION_MS,"Homing gate must cover animation");
constexpr bool colorfulStartup(){
  bool red=false,green=false,blue=false;
  for(uint8_t y=0;y<8;++y)for(uint8_t x=0;x<8;++x){
    const auto c=startupAnimation::color(x,y,25);
    if(c.r==0 && c.g==0 && c.b==0)return false;
    if(c.r>c.g && c.r>c.b)red=true;
    if(c.g>c.r && c.g>c.b)green=true;
    if(c.b>c.r && c.b>c.g)blue=true;
    if(startupAnimation::faceMix(x,y,39)!=0 || startupAnimation::faceMix(x,y,59)!=255)return false;
  }
  return red && green && blue;
}
static_assert(colorfulStartup(),"All 64 pixels light up with RGB variety, then fully dissolve to sleep");

static_assert(sleepAnimation::glyph(2,1) && sleepAnimation::glyph(1,2), "Z needs a descending diagonal");
static_assert(!sleepAnimation::glyph(1,1) && !sleepAnimation::glyph(2,2), "Diagonal must not resemble an S or I");

constexpr bool forgivingAndVaried() {
  Model m; m.values.annoyance=100;
  for(int i=0;i<5;++i) m.restSecond();
  if(m.mood()==Mood::Annoyed || m.mood()==Mood::Furious) return false;
  for(int i=5;i<10;++i) m.restSecond();
  if(m.values.annoyance!=0) return false;
  for(int i=0;i<12;++i){m.activate();for(int t=0;t<4;++t)m.restSecond();if(m.values.annoyance>=config::ANNOYED_THRESHOLD)return false;}
  Model curious; curious.values.curiosity=60;curious.values.excitement=20;
  curious.activations=1;const auto a=curious.behavior();
  curious.activations=2;const auto b=curious.behavior();
  curious.activations=3;const auto c=curious.behavior();
  if(a.approachMs==b.approachMs || b.approachMs==c.approachMs || a.pauseMs==c.pauseMs)return false;
  Model playful; playful.playStreak=3; playful.values.excitement=60;playful.values.mischief=60;
  playful.activations=1;const auto p=playful.behavior();
  playful.activations=3;const auto q=playful.behavior();
  return p.peekDeg==q.peekDeg && p.pauseMs<q.pauseMs && p.hideWaitMs<q.hideWaitMs && q.stepMs==0;
}
static_assert(forgivingAndVaried(), "Spaced play cools anger; expressive variants keep strong pushes");

#include "choreography.h"
static_assert(choreography::bounded(), "Every gesture stays short of the push point");

constexpr bool slowerPlayFasterFatigue() {
  Model m;
  m.values.curiosity=0; m.values.excitement=100; m.values.mischief=100;
  m.activate(); if(m.mood()==Mood::Playful)return false;
  m.activate(); if(m.mood()==Mood::Playful)return false;
  m.activate(); if(m.mood()!=Mood::Playful)return false;
  for(int i=0;i<8;++i)m.restSecond();
  m.values.curiosity=0;m.values.excitement=100;m.values.mischief=100;
  m.activate();if(m.playStreak!=1 || m.mood()==Mood::Playful)return false;
  Model angry;angry.values.annoyance=100;
  angry.restSecond();if(angry.values.annoyance!=100)return false;
  angry.restSecond();angry.restSecond();angry.restSecond();if(angry.mood()==Mood::Annoyed || angry.mood()==Mood::Furious)return false;
  angry.restSecond();angry.restSecond();if(angry.values.annoyance!=0)return false;
  Model spaced;for(int i=0;i<9;++i){spaced.activate();for(int t=0;t<8;++t)spaced.restSecond();}
  if(spaced.mood()==Mood::Tired)return false;
  Model furious;furious.values.annoyance=90;
  return furious.mood()==Mood::Furious; // Still available with enough energy.
}
static_assert(slowerPlayFasterFatigue(),"Warm up before playful; cool anger quickly; spaced play is less tiring");

constexpr bool angerBeforeExhaustionWithGaps() {
  Model m;
  for(int poke=1;poke<=9;++poke){
    m.activate();
    if(poke==6 && m.mood()!=Mood::Annoyed)return false;
    if((poke==7 || poke==8) && m.mood()!=Mood::Furious)return false;
    if(poke==9 && m.mood()!=Mood::Tired)return false;
    m.restSecond(); // A short gap must not erase the whole irritation increase.
  }
  return true;
}
static_assert(angerBeforeExhaustionWithGaps(),"One-second gaps still allow anger before fatigue");
