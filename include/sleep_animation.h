#pragma once
#include <stdint.h>
namespace sleepAnimation {
constexpr uint16_t Z_FRAME_MS = 160;
constexpr uint8_t Z_PHASES = 10;
// Four rows make the diagonal readable: #### / ..#. / .#.. / ####.
constexpr bool glyph(int x, int y) {
  return x >= 0 && x < 4 && y >= 0 && y < 4
      && (y == 0 || y == 3 || x == 3 - y);
}
constexpr bool streamZ(uint8_t x, uint8_t y, uint8_t phase, uint8_t letter) {
  // Two lanes, half a cycle apart. Letters emerge above the face and travel
  // completely past the top edge before restarting; clipping is intentional.
  if (y >= 5) return false;
  const uint8_t age = (phase + letter * (Z_PHASES / 2)) % Z_PHASES;
  return glyph(int(x) - letter * 4, int(y) - (4 - int(age)));
}
constexpr bool floatingZ(uint8_t x, uint8_t y, uint8_t phase) {
  return streamZ(x, y, phase, 0) || streamZ(x, y, phase, 1);
}
}
