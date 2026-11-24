#pragma once
#include <stdint.h>
namespace surprises {
enum class Kind : uint8_t { None, SystemError, Countdown, Deadpan };
constexpr uint16_t duration(Kind k) {
  return k==Kind::SystemError?1800:k==Kind::Countdown?2100:1100;
}
struct Selector {
  uint32_t randomState=0x6d2b79f5UL, lastAt=0;
  uint8_t pokes=0;
  Kind previous=Kind::None;
  Kind choose(uint32_t now,bool eligible) {
    if(pokes<255)++pokes;
    // Human interaction timing perturbs a private PRNG; no floating analog pin needed.
    randomState^=now;
    randomState^=randomState<<13;randomState^=randomState>>17;randomState^=randomState<<5;
    if(!randomState)randomState=0x6d2b79f5UL;
    if(!eligible || pokes<5 || (previous!=Kind::None &&
        (pokes<9 || uint32_t(now-lastAt)<45000UL)) || randomState%12!=0)return Kind::None;
    Kind result=Kind(1+(randomState/12)%3);
    if(result==previous)result=Kind(1+uint8_t(result)%3);
    previous=result;lastAt=now;pokes=0;
    return result;
  }
};
// Five rows, three columns; countdown reads 3, 2, 1.
constexpr uint8_t digitRow(uint8_t digit,uint8_t y) {
  return digit==3 ? (y%2==0?7:1) : digit==2 ? (y%2==0?7:y==1?1:4)
      : (y==0?2:y==1?6:y==4?7:2);
}
}
