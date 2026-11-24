#pragma once
#include <stdint.h>
#include "personality.h"
namespace face {
void begin();
void update(uint32_t now, personality::Mood mood, bool reacting);
}
