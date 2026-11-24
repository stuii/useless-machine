#include <Arduino.h>
#include "../../src/main.cpp"
uint32_t fakeNow=0;
bool fakeOn=false;
unsigned plays=0,pushes=0;
bool seen[32]={};
namespace face {void begin(){} void update(uint32_t,personality::Mood,bool){}}
namespace sound {void begin(){} void play(personality::Mood,uint32_t){++plays;} void update(uint32_t){} void stop(){} bool toneActive(){return false;}}
void check(bool ok,const char* why){if(!ok){std::fprintf(stderr,"FAIL t=%lu state=%d: %s\n",(unsigned long)fakeNow,int(state),why);std::exit(1);}}
void tick(){
  const State old=state;
  ++fakeNow;loop();seen[int(state)]=true;
  if(state==State::Holding && old!=State::Holding)++pushes;
  check(angle>=config::PUSH_DEG && angle<=config::HOME_DEG,"angle outside calibrated travel");
  check(servoInstances==1 && arm.pin==config::ARM_SERVO_PIN,"exactly one servo on D9");
  if(state==State::Idle || state==State::WaitOff || state==State::MischiefWait)
    check(angle==config::HOME_DEG,"rest/wait must be home");
}
void advance(uint32_t ms){while(ms--)tick();}
void until(State wanted,uint32_t limit=30000){while(state!=wanted && limit--)tick();check(state==wanted,"state timeout");}
void select(unsigned mode){
  mind=personality::Model();
  if(mode==0)mind.values.curiosity=0;
  if(mode==2)mind.values.annoyance=20;
  if(mode==3){mind.values.curiosity=0;mind.values.excitement=40;mind.values.mischief=60;mind.activations=2;mind.playStreak=2;}
  if(mode==4)mind.values.annoyance=60;
  if(mode==5)mind.values.annoyance=90;
  if(mode==6)mind.values.energy=10;
}
void trigger(){
  const auto previousPlays=plays;
  fakeOn=true;advance(31);
  check(state==State::ResponsePause && plays==previousPlays+1,"sound must start before response pause");
  const int heldAngle=angle;
  advance(config::RESPONSE_PAUSE_MS-1);
  check(state==State::ResponsePause && angle==heldAngle,"every response must hold its target for the sound lead-in");
  advance(2); // Dispatch the selected routine after the pause, including quick repeats.
}
void cancel(){fakeOn=false;until(State::Idle);}
void complete(){until(State::WaitOff);const auto count=mind.activations;const auto hits=pushes;advance(2000);check(mind.activations==count && pushes==hits,"held ON repeated routine");cancel();}
void boot(bool on=false){fakeOn=on;setup();while(!startupComplete)tick();if(!on)check(state==State::Idle && mind.mood()==personality::Mood::Asleep,"boot must rest asleep");check(plays==0,"startup must be silent");}
int main(int argc,char** argv){
  const unsigned n=argc>1?std::atoi(argv[1]):0;
  boot(n==7);
  // Legacy scenarios explicitly exercise the baseline routine; new cases verify production defaults.
  if(n<61)for(auto &v:nextVariant)v=0;
  if(n<=6){
    select(n);trigger();
    if(n==6){until(State::Approaching);const auto start=fakeNow;until(State::Peeking);check(fakeNow-start>=80UL*config::TIRED_CREEP_MS && angle==100,"tired must creep slowly to 100");advance(899);check(state==State::Peeking,"tired pause truncated");}
    complete();check(pushes==(n==5?2u:1u),"wrong bounded jab count");
    if(n==0)check(seen[int(State::GreetingPause)] && seen[int(State::GreetingBackPause)],"happy greeting missing");
    if(n==1)check(seen[int(State::Peeking)] && seen[int(State::InspectPause)],"curious needs two inspections");
    if(n==2)check(seen[int(State::SuspiciousBack)] && seen[int(State::SuspiciousPause)],"suspicious retreat missing");
    if(n==3)check(seen[int(State::MischiefWait)],"playful must hide");
    if(n==4)check(!seen[int(State::Peeking)] && !seen[int(State::Recoil)],"annoyed must swat directly");
    if(n==5)check(seen[int(State::RecoilPause)],"furious must recoil before second jab");
    check(plays==1,"routine must count as one sound/activation");
  } else if(n==7){tick();complete();check(mind.activations==1,"held ON at boot lost");}
  else if(n==8 || n==9){
    select(6);trigger();until(n==8?State::Peeking:State::Holding);
    fakeOn=false;advance(31);trigger();
    check(cycleBehavior.routine==Routine::Creep && state!=State::Holding,"quick tired poke bypassed creep");
    if(n==9)check(state==State::Preparing,"tired must reset from beyond peek");
    until(State::Approaching);const int before=angle;advance(18);check(std::abs(angle-before)==1,"quick tired approach must remain one degree per 18 ms");
    until(State::Peeking);advance(899);check(state==State::Peeking,"quick tired must retain long pause");complete();
  } else if(n==10 || n==23){
    select(n==10?4:1);trigger();until(State::Holding);fakeOn=false;advance(31);trigger();
    check(state!=State::Holding,"quick poke must not bypass selected variation");if(motionVariant)until(State::ScriptMove);complete();
  } else if(n==11 || n==12 || (n>=20 && n<=22)){
    select(3);if(n>=20)mind.activations=n-17;trigger();until(State::Peeking);check(angle==80,"playful peek must stay 80");
    advance(cycleBehavior.pauseMs-1);check(state==State::Peeking,"fake peek hold shortened");
    until(State::MischiefWait);advance(n==11 || n==12 ? cycleBehavior.hideWaitMs/2 : cycleBehavior.hideWaitMs-1);check(state==State::MischiefWait,"hide wait shortened");
    if(n==11){cancel();check(pushes==0,"OFF in hiding must cancel slam");}
    else if(n==12){fakeOn=false;advance(31);trigger();check(state!=State::Holding,"new quick poke must run its replacement variation");complete();}
    else {complete();check(pushes==1,"fake-out must slam once");}
  } else if(n>=13 && n<=17){
    const unsigned modes[]={1,2,0,5,6};
    const State stops[]={State::InspectPause,State::SuspiciousPause,State::GreetingPause,State::RecoilPause,State::Peeking};
    select(modes[n-13]);trigger();until(stops[n-13]);const auto hits=pushes;cancel();advance(2000);check(pushes==hits,"OFF must cancel pending gesture/jab");
  } else if(n==18 || n==27){
    fakeOn=true;advance(20);fakeOn=false;advance(100);check(mind.activations==0 && state==State::Idle,"switch bounce triggered motion");
  } else if(n==19){
    fakeNow=0xfffffff0UL;lastRestTick=fakeNow;stateSince=fakeNow;rawSince=fakeNow;
    select(6);trigger();complete();check(pushes==1,"millis rollover broke cycle");
  } else if(n==24){select(6);trigger();until(State::Approaching);advance(180);cancel();check(pushes==0,"cancel creep must not push");}
  else if(n==25){advance(65000);check(state==State::Idle && mind.mood()==personality::Mood::Asleep && plays==0,"quiet sleep must remain still/silent");}
  else if(n==26){select(5);trigger();until(State::Holding);cancel();check(pushes==1,"successful furious push must cancel second jab");}
  else if(n==28){
    select(4);fakeOn=true;advance(31);check(state==State::ResponsePause,"response pause missing");
    advance(100);cancel();check(pushes==0,"OFF during sound lead-in must cancel the push");
  }
  else if(n>=29 && n<=42){
    const unsigned mode=(n-29)/2, variant=(n-29)%2+1;
    select(mode);
    const Routine routines[]={Routine::Greeting,Routine::Inspect,Routine::Distrust,Routine::FakeOut,Routine::Swat,Routine::DoubleJab,Routine::Creep};
    nextVariant[static_cast<uint8_t>(routines[mode])]=variant;
    trigger();until(State::ScriptMove);
    check(motionVariant==variant,"selected choreography variant lost");
    if(mode==6){until(State::Approaching);until(State::Peeking);check(angle==100,"tired theatre must still reach calibrated creep stop");advance(899);check(state==State::Peeking,"tired theatre must preserve 900 ms pause");}
    complete();
    check(seen[int(State::ScriptPause)] && pushes==(mode==5?2u:1u),"choreography must finish with bounded actual pushes");
    check(plays==1,"theatre must not replay activation sound");
  } else if(n>=43 && n<=56){
    const unsigned mode=(n-43)/2, variant=(n-43)%2+1;
    select(mode);const Routine routines[]={Routine::Greeting,Routine::Inspect,Routine::Distrust,Routine::FakeOut,Routine::Swat,Routine::DoubleJab,Routine::Creep};
    nextVariant[static_cast<uint8_t>(routines[mode])]=variant;
    trigger();until(State::ScriptPause);cancel();advance(2000);
    check(pushes==0,"OFF must cancel every scripted routine");
  } else if(n==57){
    for(unsigned v=0;v<4;++v){select(0);trigger();check(motionVariant==v%3,"same mood must cycle all three routines");complete();advance(1500);}
  } else if(n==58 || n==59){
    const bool tired=n==59;select(tired?6:3);
    nextVariant[static_cast<uint8_t>(tired?Routine::Creep:Routine::FakeOut)]=1;
    trigger();until(State::ScriptPause);fakeOn=false;advance(31);trigger();
    if(tired){until(State::Approaching);const int before=angle;advance(18);check(std::abs(angle-before)==1,"quick tired repeat must still creep after scripted cancellation");}
    else {check(state!=State::Holding,"quick poke must replace script without bypassing new variation");until(State::ScriptMove);}
    complete();
  }
  else if(n==60){
    for(unsigned poke=1;poke<=9;++poke){
      trigger();
      if(poke==6)check(mind.mood()==personality::Mood::Annoyed,"real controller must reach annoyed on sixth poke");
      if(poke==7 || poke==8)check(mind.mood()==personality::Mood::Furious,"real controller must reach furious before tired");
      if(poke==9)check(mind.mood()==personality::Mood::Tired,"real controller must still tire by ninth poke");
      until(State::Holding);cancel();advance(1000);
    }
  }
  else if(n==61){
    for(unsigned mode=0;mode<7;++mode){
      select(mode);trigger();check(motionVariant==1,"every mood must start with an expressive variant");
      until(State::ScriptMove);complete();advance(1500);
    }
  } else if(n==62){
    select(0);trigger();until(State::ScriptPause);check(motionVariant==1,"first expressive routine");
    fakeOn=false;advance(31);select(0);trigger();until(State::ScriptPause);check(motionVariant==2,"rapid repeat must run next expressive routine");
    fakeOn=false;advance(31);select(0);trigger();until(State::GreetingPause);check(motionVariant==0,"third repeat must retain baseline greeting");
    cancel();check(pushes==0,"cancelled variations must not leave pending pushes");
  }
  check(state==State::Idle && angle==config::HOME_DEG,"must finish at rest");
  std::printf("PASS scenario %u\n",n);
}
