#pragma once
#include <stdint.h>
#include "personality.h"
namespace face {
enum class Expression : uint8_t { Normal, Knowing, FakeSleep, Peek, Victory };
void expression(Expression value, uint32_t since, uint8_t variant=0);
void begin();
void update(uint32_t now, personality::Mood mood, bool reacting);
}
