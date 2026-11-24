#pragma once
#include <stdint.h>
#include "personality.h"
namespace sound {
void begin();
void play(personality::Mood mood, uint32_t now);
void update(uint32_t now);
void stop();
bool toneActive(); // LED transfers may proceed during rests, never during notes.
}
