#pragma once
#include <stdint.h>
namespace interaction {
constexpr uint32_t QUICK_REPEAT_MS = 1200;
constexpr int approachAngle(int home, int push) { return home + (push - home) * 60 / 100; }
class Gate {
 public:
  bool armed = true;
  bool released = false;
  uint32_t offSince = 0;
  constexpr bool accept(bool on, uint32_t now, bool startup) {
    if (!on) {
      if (!armed) { released = true; offSince = now; }
      armed = true;
      return false;
    }
    if (!armed || startup) return false;
    armed = false;
    return true;
  }
  constexpr bool quick(uint32_t now) const {
    return released && static_cast<uint32_t>(now - offSince) <= QUICK_REPEAT_MS;
  }
};
}
