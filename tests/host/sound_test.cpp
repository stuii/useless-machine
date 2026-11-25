#include <Arduino.h>
#include "../../src/sound.cpp"
uint32_t fakeNow=0;
bool fakeOn=false;
unsigned hostToneHz=0;
void check(bool ok,const char* why){if(!ok){std::fprintf(stderr,"FAIL: %s\n",why);std::exit(1);}}
int main(){
  sound::begin();check(!sound::toneActive(),"boot silent");
  sound::playCue(sound::Cue::Countdown,100);
  check(hostToneHz==900,"tick 3");sound::update(160);check(!sound::toneActive(),"rest between digits");
  sound::update(800);check(hostToneHz==1100,"tick 2 exactly 700ms later");
  sound::update(1500);check(hostToneHz==1400,"tick 1 exactly 1400ms later");
  sound::update(2300);check(!sound::toneActive(),"countdown ends");
  sound::playCue(sound::Cue::Surrender,2000);check(sound::toneActive(),"surrender cue plays");
  sound::stop();check(!sound::toneActive() && hostToneHz==0,"OFF silences effect");
  for(unsigned c=0;c<8;++c){
    sound::playCue(sound::Cue(c),0xfffffff0UL);
    for(uint32_t age=0;age<4000;age+=4)sound::update(uint32_t(0xfffffff0UL+age));
    check(!sound::toneActive(),"all effects terminate across rollover");
  }
  for(unsigned c=4;c<7;++c){
    sound::playCue(sound::Cue(c),0);unsigned heard=0,rests=0;
    for(unsigned age=0;age<1400;age+=4){sound::update(age);if(sound::toneActive())++heard;else ++rests;}
    check(heard>50 && rests>20 && !sound::toneActive(),"victory tune is audible, has LED-safe rests, and fits celebration");
  }
  sound::playCue(sound::Cue::VictoryTease,0);sound::play(personality::Mood::Happy,1);
  check(hostToneHz==1100,"fresh interaction replaces effect");
  std::puts("PASS sound timing, interruption, and rollover");
}
