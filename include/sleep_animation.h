#pragma once
#include <stdint.h>
namespace sleepAnimation {
constexpr uint16_t FRAME_MS = 100;
constexpr uint8_t FRAMES = 16;
constexpr uint16_t DURATION_MS = FRAME_MS * FRAMES;
constexpr uint8_t startupFrame(uint32_t elapsed) {
  return elapsed >= DURATION_MS ? FRAMES - 1 : elapsed / FRAME_MS;
}
// Four rows make the diagonal readable: #### / ..#. / .#.. / ####.
constexpr bool glyph(int x, int y) {
  return x >= 0 && x < 4 && y >= 0 && y < 4
      && (y == 0 || y == 3 || x == 3 - y);
}
constexpr bool floatingZ(uint8_t x, uint8_t y, uint8_t phase) {
  // One complete letter; never overlap glyphs or crop the horizontal strokes.
  // Drift right and up, leaving eyes on row 5 and mouth on row 7 untouched.
  if (y >= 5) return false;
  phase %= 8;
  return glyph(int(x) - (1 + phase / 3), int(y) - (phase < 4 ? 1 : 0));
}
}
