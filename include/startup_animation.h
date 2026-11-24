#pragma once
#include <stdint.h>
namespace startupAnimation {
constexpr uint16_t FRAME_MS=40;
constexpr uint8_t FRAMES=60;
constexpr uint16_t DURATION_MS=FRAME_MS*FRAMES;
struct RGB { uint8_t r,g,b; };
constexpr uint8_t frameAt(uint32_t ms){return ms>=DURATION_MS?FRAMES-1:ms/FRAME_MS;}
constexpr uint8_t clamp(int value){return value<0?0:value>255?255:value;}
constexpr uint8_t triangle(uint8_t phase){return phase<128?phase*2:(255-phase)*2;}
constexpr RGB wheel(uint8_t hue){
  if(hue<85)return {uint8_t(255-hue*3),uint8_t(hue*3),0};
  if(hue<170){hue-=85;return {0,uint8_t(255-hue*3),uint8_t(hue*3)};}
  hue-=170;return {uint8_t(hue*3),0,uint8_t(255-hue*3)};
}
constexpr RGB color(uint8_t x,uint8_t y,uint8_t frame){
  const int dx=int(x)*2-7,dy=int(y)*2-7;
  const uint8_t radius=dx*dx+dy*dy;
  // Diagonal rainbow ignition, followed by a moving field of concentric color ripples.
  const uint8_t hue=uint8_t(x*19+y*29+frame*7+radius*2);
  const uint8_t wave=triangle(uint8_t(radius*5-frame*13+x*9));
  const uint8_t level=80+uint16_t(wave)*120/255;
  const uint8_t reveal=frame<20?clamp(int(frame)*22-int(x+y)*16+32):255;
  const uint16_t value=uint16_t(level)*reveal/255;
  const RGB c=wheel(hue);
  return {uint8_t(uint16_t(c.r)*value/255),uint8_t(uint16_t(c.g)*value/255),uint8_t(uint16_t(c.b)*value/255)};
}
constexpr uint8_t faceMix(uint8_t x,uint8_t y,uint8_t frame){
  // Stagger the dissolve across the grid; every pixel is fully asleep by the end.
  return frame<40?0:clamp(int(frame-40)*20-int(x+y)*5);
}
}
