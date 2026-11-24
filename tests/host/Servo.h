#pragma once
class Servo {
 public:
 int value=90; bool enabled=false;
 void write(int v) { value=v; }
 void attach(int) { enabled=true; }
 void detach() { enabled=false; }
 bool attached() const { return enabled; }
};
