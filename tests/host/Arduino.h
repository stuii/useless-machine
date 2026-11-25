#pragma once
#include <stdint.h>
#include <cstdio>
#include <cstdlib>
using __FlashStringHelper = char;
#define F(x) x
#define LOW 0
#define HIGH 1
#define OUTPUT 1
#define INPUT_PULLUP 2
#define LED_BUILTIN 13
extern uint32_t fakeNow;
extern bool fakeOn;
inline uint32_t millis() { return fakeNow; }
inline int digitalRead(int) { return fakeOn ? LOW : HIGH; }
inline void pinMode(int,int) {}
inline void digitalWrite(int,int) {}
struct SerialStub {
 void begin(int) {}
 template<class T> void print(T) {}
 template<class T> void println(T) {}
};
static SerialStub Serial;

extern unsigned hostToneHz;
inline void tone(int,unsigned hz){hostToneHz=hz;}
inline void noTone(int){hostToneHz=0;}
