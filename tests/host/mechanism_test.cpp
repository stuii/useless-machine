#include <Arduino.h>
#include "../../src/main.cpp"
uint32_t fakeNow=0;
bool fakeOn=false;
namespace face { void begin() {} void update(uint32_t,personality::Mood,bool) {} }
namespace sound { void begin() {} void play(personality::Mood,uint32_t) {} void update(uint32_t) {} void stop() {} bool toneActive(){return false;} }
void check(bool ok,const char* why) { if(!ok){std::fprintf(stderr,"FAIL at %lu state %d: %s\n",(unsigned long)fakeNow,int(state),why);std::exit(1);} }
void tick() {
 const int oldArm=arm.value,oldLid=lid.value;
 ++fakeNow; loop();
 if (state == State::Idle || state == State::WaitOff || state == State::MischiefWait)
   check(!lid.attached(), "stowed lid must not receive holding pulses");
 if (lid.value != oldLid || lidReady)
   check(lid.attached(), "moving/open lid must remain driven for clearance");
 if (arm.enabled && arm.value != oldArm && arm.value != config::HOME_DEG && state != State::Retracting)
   check(lidReady,"arm extends before lid ready");
 const bool closing = config::LID_OPEN_DEG > config::LID_CLOSED_DEG ? lid.value < oldLid : lid.value > oldLid;
 if (closing) check(angle == config::HOME_DEG,"lid closes before stowing");
 check(angle >= (config::HOME_DEG < config::PUSH_DEG ? config::HOME_DEG : config::PUSH_DEG) && angle <= (config::HOME_DEG > config::PUSH_DEG ? config::HOME_DEG : config::PUSH_DEG),"arm outside calibrated limits");
}
void advance(uint32_t ms){while(ms--)tick();}
void until(State wanted,uint32_t timeout=30000){while(state!=wanted && timeout--)tick();check(state==wanted,"state timeout");}
void boot(bool on=false){fakeOn=on;setup();while(!startupComplete)tick();if(!on){check(state==State::Idle,"startup must stow and idle");check(mind.mood()==personality::Mood::Asleep,"startup must preserve sleep");}}
int main(int argc,char** argv){
 const int scenario=argc>1?std::atoi(argv[1]):0;
 boot(scenario==7);
 if(scenario==8 || scenario==9) mind.values.annoyance = scenario==8 ? 60 : 90;
 if(scenario==11) mind.values.annoyance=20; // First poke produces suspicion.
 if(scenario==12 || scenario==13) mind.values.energy=10;
 if(scenario>=14){mind.values.curiosity=0;mind.values.excitement=20;mind.values.mischief=60;}
 if(scenario>=20) mind.activations=scenario-19;
 if(scenario!=7){fakeOn=true;advance(31);}
 const auto acceptedActivations=mind.activations;
 if(scenario==0){until(State::Extending);advance(100);fakeOn=false;until(State::Idle);}
 if(scenario==1){until(State::LidReady);fakeOn=false;until(State::Idle);}
 if(scenario==2){until(State::Extending);advance(100);fakeOn=false;advance(35);const auto count=mind.activations;fakeOn=true;advance(31);check(mind.activations==count+1 && (state==State::Extending || state==State::Holding),"rapid flip must resume ready arm");fakeOn=false;until(State::Idle);}
 if(scenario==3){until(State::Extending);fakeOn=false;until(State::ClosingLid);advance(120);fakeOn=true;advance(31);check(state!=State::Extending,"closing lid must reopen before extension");until(State::Extending);fakeOn=false;until(State::Idle);}
 if(scenario==4){until(State::WaitOff);const auto count=mind.activations;advance(10000);check(state==State::WaitOff && mind.activations==count,"held ON repeats");fakeOn=false;until(State::Idle);}
 if(scenario==5){until(State::OpeningLid);fakeOn=false;advance(35);fakeOn=true;until(State::Extending);fakeOn=false;until(State::Idle);}
 if(scenario==6){until(State::Extending);fakeOn=false;until(State::Settling);advance(50);fakeOn=true;until(State::Holding);fakeOn=false;until(State::Idle);}
 if(scenario==7){until(State::Extending);check(mind.activations==1,"ON at boot lost");fakeOn=false;until(State::Idle);}
 if(scenario==10 || scenario==11 || scenario==12){
   until(State::Approaching);
   const int before=arm.value;
   advance(cycleBehavior.approachMs);
   check(std::abs(arm.value-before)==1,"creep must advance one degree at a time");
   until(State::Peeking);
   check(arm.value==cycleBehavior.peekDeg,"creep stops at peek position");
   if(scenario==12) check(arm.value==100 && cycleBehavior.pauseMs==900,"tired pauses at 100 degrees for 900 ms");
   advance(cycleBehavior.pauseMs-1);
   check(state==State::Peeking,"brief pause must precede slam");
   until(State::Extending); tick();
   check(state==State::Holding && arm.value==config::PUSH_DEG,"creep must finish with direct slam");
   fakeOn=false; until(State::Settling);
   check(arm.value==config::HOME_DEG,"slam routine snaps back home");
   until(State::Idle);
 }
 if(scenario==13){
   until(State::Peeking); fakeOn=false; advance(31);
   check(state==State::Retracting && arm.value!=config::PUSH_DEG,"OFF during tired pause cancels slam");
   until(State::Idle);
 }
 if(scenario>=14){
   check(feint==Feint::Peek,"playful activation must select fake-out");
   until(State::Approaching); tick();
   check(state==State::Peeking && arm.value==80,"fake-out darts directly to 80 degrees");
   if(scenario==15){fakeOn=false;until(State::Idle);}
   else {
     advance(cycleBehavior.pauseMs-1);
     check(state==State::Peeking && arm.value==80,"fake-out holds before retracting");
     until(State::ClosingLid);
     check(arm.value==config::HOME_DEG,"fake-out retracts before closing");
     if(scenario==18){
       fakeOn=false;advance(35);fakeOn=true;advance(31);
       check(feint==Feint::None,"new quick poke replaces closing fake-out");
       until(State::Holding);fakeOn=false;until(State::Idle);
     } else {
       until(State::MischiefWait);
       check(lidAngle==config::LID_CLOSED_DEG && !lid.attached(),"fake-out waits closed with lid released");
       if(scenario==16 || scenario==17){
         fakeOn=false;until(State::Idle);
         if(scenario==17){fakeOn=true;advance(31);check(feint==Feint::None,"quick retry cancels old fake-out");until(State::Holding);fakeOn=false;until(State::Idle);}
       } else {
         advance(cycleBehavior.closedWaitMs-1);
         check(state==State::MischiefWait,"closed pause must finish before reopening");
         until(State::OpeningLid);
         if(scenario==19){fakeOn=false;until(State::Idle);check(arm.value==config::HOME_DEG,"OFF cancels second-stage slam");}
         else {
           until(State::Extending);tick();
           check(state==State::Holding && arm.value==config::PUSH_DEG && mind.activations==acceptedActivations,"second opening slams without another activation");
           until(State::WaitOff);advance(3000);
           check(state==State::WaitOff && mind.activations==acceptedActivations,"held ON must not repeat fake-out");
           fakeOn=false;until(State::Idle);
         }
       }
     }
   }
 }
 if(scenario==8 || scenario==9){
   until(State::Extending); tick();
   check(state==State::Holding && arm.value==config::PUSH_DEG,"fast push must command target in one tick");
   fakeOn=false; until(State::Settling);
   check(arm.value==config::HOME_DEG,"fast return must command home");
   advance(config::HOME_SETTLE_MS-1);
   check(state==State::Settling && lidReady,"direct return still needs physical settling before lid closes");
   until(State::Idle);
 }
 check(lidAngle==config::LID_CLOSED_DEG,"final stow position");
 check(!lid.attached(), "lid must release after stowing");
 advance(10000);
 check(!lid.attached(), "idle must not reattach lid");
 fakeOn=true; until(State::OpeningLid);
 check(lid.attached(), "next interaction must reattach lid");
 until(State::Extending); fakeOn=false; until(State::Idle);
 check(!lid.attached(), "lid must release again after repeat cycle");
 std::printf("PASS scenario %d\n",scenario);
}
