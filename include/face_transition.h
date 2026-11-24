#pragma once
#include <stdint.h>
namespace faceAnimation {
constexpr uint8_t LAST_FRAME = 8;
constexpr uint16_t FRAME_MS = 40;
struct Blend { uint8_t first; uint8_t second; uint8_t amount; };
// Scenes: old open, old closed, new closed, new open.
constexpr Blend blend(uint8_t frame) {
  return frame < 3 ? Blend{0, 1, static_cast<uint8_t>(frame * 255 / 3)}
      : frame < 5 ? Blend{1, 2, static_cast<uint8_t>((frame - 3) * 255 / 2)}
      : Blend{2, 3, static_cast<uint8_t>((frame - 5) * 255 / 3)};
}
constexpr uint8_t mix(uint8_t a, uint8_t b, uint8_t amount) {
  return (uint16_t(a) * (255 - amount) + uint16_t(b) * amount) / 255;
}
class Transition {
 public:
  uint8_t current = 0;
  uint8_t target = 0;
  uint8_t frame = 0;
  bool active = false;
  uint32_t lastFrame = 0;
  constexpr void request(uint8_t mood, uint32_t now) {
    // Finish the visible transition first. The caller submits the latest mood
    // each loop, so intermediate moods cannot create an ever-growing queue.
    if (active || mood == current || mood >= 9) return;
    target = mood;
    frame = 0;
    active = true;
    lastFrame = now - FRAME_MS;
  }
  constexpr bool due(uint32_t now) const {
    return active && static_cast<uint32_t>(now - lastFrame) >= FRAME_MS;
  }
  constexpr void presented(uint32_t now) {
    if (!active) return;
    lastFrame = now;
    if (frame == LAST_FRAME) { current = target; active = false; }
    else ++frame;
  }
};
}
