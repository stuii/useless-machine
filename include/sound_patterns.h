#pragma once
#include <stdint.h>
#include <avr/pgmspace.h>
#include "personality.h"
namespace sound {
struct Note { uint16_t fromHz; uint16_t toHz; uint16_t ms; uint8_t warbleHz; };
constexpr uint8_t NOTES_PER_PHRASE = 9;
constexpr uint8_t PITCH_TICK_MS = 4; // 250 Hz pitch-control updates, not audio sample rate.
// Five tiny syllables separated by LED-safe rests. All sound is interaction-triggered.
constexpr Note phrases[9][NOTES_PER_PHRASE] PROGMEM = {
 {{1100,1800,48,0},{0,0,24,0},{1800,2500,40,0},{0,0,24,0},{1500,2200,36,15},{0,0,24,0},{2400,3000,48,0},{0,0,24,0},{3000,2200,56,20}},
 {{650,1300,56,0},{0,0,24,0},{1800,1100,40,20},{0,0,24,0},{900,1600,44,0},{0,0,24,0},{1300,2100,40,15},{0,0,24,0},{2100,2900,60,25}},
 {{1400,2400,32,20},{0,0,24,0},{2600,1700,32,0},{0,0,24,0},{1700,3000,36,25},{0,0,24,0},{2400,1400,32,20},{0,0,24,0},{2100,3200,52,30}},
 {{750,420,44,35},{0,0,24,0},{680,380,40,45},{0,0,24,0},{500,800,32,30},{0,0,24,0},{600,300,52,40},{0,0,24,0},{440,220,72,25}},
 {{360,180,32,50},{0,0,24,0},{520,240,28,65},{0,0,24,0},{300,650,32,60},{0,0,24,0},{600,200,40,50},{0,0,24,0},{280,140,80,30}},
 {{1100,850,72,10},{0,0,24,0},{850,620,64,10},{0,0,24,0},{700,500,60,15},{0,0,24,0},{500,350,68,10},{0,0,24,0},{350,190,96,10}},
 {{1200,950,64,0},{0,0,24,0},{950,750,56,0},{0,0,24,0},{750,600,52,10},{0,0,24,0},{600,450,56,0},{0,0,24,0},{450,300,80,0}},
 {{900,700,64,0},{0,0,24,0},{700,550,56,0},{0,0,24,0},{550,420,52,0},{0,0,24,0},{420,320,56,0},{0,0,24,0},{320,220,72,0}},
 {{700,1100,64,15},{0,0,24,0},{1200,800,48,20},{0,0,24,0},{850,1400,56,15},{0,0,24,0},{1500,1000,48,20},{0,0,24,0},{1000,1900,64,25}}
};
constexpr bool noteFinished(uint32_t now, uint32_t since, uint16_t duration) {
  return static_cast<uint32_t>(now - since) >= duration;
}
constexpr uint16_t pitchAt(Note note, uint16_t elapsed) {
  if (!note.fromHz) return 0;
  if (elapsed >= note.ms) return note.toHz;
  const int32_t sweep = int32_t(note.fromHz) +
      (int32_t(note.toHz) - note.fromHz) * elapsed / note.ms;
  const int16_t wobble = (elapsed / 8) % 2 ? note.warbleHz : -int16_t(note.warbleHz);
  return static_cast<uint16_t>(sweep + wobble);
}
}
