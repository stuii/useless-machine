#pragma once
static unsigned servoInstances=0;
class Servo {
 public:
 Servo(){++servoInstances;}
 int pin=-1;
 int value=90; bool enabled=false;
 void write(int v) { value=v; }
 void attach(int p) { pin=p; enabled=true; }
 void detach() { enabled=false; }
 bool attached() const { return enabled; }
};
