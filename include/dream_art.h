#pragma once
#include <stdint.h>
namespace dreamArt {
// Five-row thought pictures leave the sleeping eyes and mouth visible.
constexpr uint8_t row(uint8_t picture, uint8_t y) {
  return picture%3==0 ? (y==0?0x66:y==1?0xff:y==2?0x7e:y==3?0x3c:0x18)
      : picture%3==1 ? (y==0?0x18:y==1?0x7e:y==2?0x3c:y==3?0x24:0x42)
      : (y==0?0x3c:y==1?0x24:y==2?0x3c:y==3?0x18:0x18);
}
constexpr bool lit(uint8_t picture,uint8_t x,uint8_t y) {
  return x<8 && y<5 && (row(picture,y)&(0x80>>x));
}
constexpr bool dreaming(uint32_t age) { return age>=12000 && (age-12000)%18000<3200; }
}
