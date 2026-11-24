#pragma once
#include <stdint.h>
#include "personality.h"
#ifdef __AVR__
#include <avr/pgmspace.h>
#define CHOREO_FLASH PROGMEM
#else
#define CHOREO_FLASH
#endif

namespace choreography {
// Percent of home-to-push travel; all gestures stop short of switch actuation.
// Wait begins after the command reaches its target (not measured shaft arrival).
struct Step { uint8_t percent; uint8_t stepMs; uint16_t waitMs; };
constexpr uint8_t ROUTINES=7, VARIANTS=2, MAX_STEPS=6;
constexpr uint8_t lengths[ROUTINES][VARIANTS] CHOREO_FLASH = {
  {4,4},{5,5},{4,5},{6,6},{4,3},{4,3},{4,4}
};
constexpr Step scripts[ROUTINES][VARIANTS][MAX_STEPS] CHOREO_FLASH = {
  // Happy: double greeting / wind-up bow and release.
  {{{17,0,220},{0,0,220},{28,0,250},{0,0,250}},
   {{20,5,180},{40,5,250},{10,0,250},{0,0,300}}},
  // Curious: two double takes / approach in three steps, think, back up.
  {{{45,4,300},{20,0,250},{65,4,350},{40,0,250},{75,6,350}},
   {{25,5,180},{50,5,250},{75,6,450},{60,0,350},{75,8,250}}},
  // Suspicious: startle, hide, cautiously reappear / two aborted approaches.
  {{{60,8,300},{0,0,850},{30,12,250},{65,8,500}},
   {{40,8,250},{15,0,300},{65,8,300},{0,0,650},{50,10,450}}},
  // Playful: two full fake-outs / almost give up, tiny peek, surprise.
  // The controller resolves 83% in FakeOut to the calibrated MISCHIEF_PEEK_DEG.
  {{{83,0,700},{0,0,900},{83,0,450},{0,0,700},{20,0,250},{0,0,500}},
   {{83,0,700},{0,0,1400},{17,0,350},{0,0,650},{35,0,250},{0,0,500}}},
  // Annoyed: two impatient air-knocks / freeze, pull back, then swat.
  {{{35,0,180},{15,0,180},{35,0,180},{0,0,250}},
   {{60,0,600},{25,0,250},{0,0,200}}},
  // Furious: short wind-up shake / charge, brake, recoil, attack.
  // Existing two-jab limit still applies after these non-contact gestures.
  {{{25,0,160},{0,0,160},{35,0,180},{0,0,220}},
   {{65,0,280},{35,0,220},{0,0,220}}},
  // Tired: nod off and restart / two droopy false starts, then normal slow creep.
  {{{17,18,650},{0,18,400},{25,18,350},{0,18,300}},
   {{30,18,350},{12,18,450},{35,18,350},{15,18,350}}}
};
inline uint8_t count(personality::Routine routine,uint8_t variant) {
  const uint8_t r=static_cast<uint8_t>(routine);
  if(r>=ROUTINES || variant<1 || variant>VARIANTS)return 0;
#ifdef __AVR__
  return pgm_read_byte(&lengths[r][variant-1]);
#else
  return lengths[r][variant-1];
#endif
}
inline Step read(personality::Routine routine,uint8_t variant,uint8_t index) {
  Step result={};
  if(index>=count(routine,variant))return result;
#ifdef __AVR__
  memcpy_P(&result,&scripts[static_cast<uint8_t>(routine)][variant-1][index],sizeof(result));
#else
  result=scripts[static_cast<uint8_t>(routine)][variant-1][index];
#endif
  return result;
}
constexpr bool bounded() {
  for(uint8_t r=0;r<ROUTINES;++r)for(uint8_t v=0;v<VARIANTS;++v){
    if(lengths[r][v]>MAX_STEPS)return false;
    for(uint8_t s=0;s<lengths[r][v];++s){
      const auto a=scripts[r][v][s];
      if(a.percent>83 || a.waitMs<150 || a.waitMs>1500 || a.stepMs>18)return false;
    }
  }
  return true;
}
static_assert(bounded(),"Theatre must stay inside travel and have bounded settling pauses");
}
#undef CHOREO_FLASH
