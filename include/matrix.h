#pragma once
#include <stdint.h>
namespace matrix {
// Logical origin is upper left, X right, Y down. Physical pixel zero defaults
// to upper left, with row 0 left-to-right and alternating row directions.
constexpr uint8_t rotatedIndex(uint8_t x, uint8_t y, bool serpentine, uint8_t rotation) {
  return rotation ? rotatedIndex(7 - y, x, serpentine, rotation - 1)
      : y * 8 + ((serpentine && (y & 1)) ? 7 - x : x);
}
constexpr uint8_t index(uint8_t x, uint8_t y, bool serpentine, uint8_t rotation, bool mirror) {
  return rotatedIndex(mirror ? 7 - x : x, y, serpentine, rotation);
}
}
