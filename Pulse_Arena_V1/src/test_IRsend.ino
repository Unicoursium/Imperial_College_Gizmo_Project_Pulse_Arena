//===========================================
// This is the test on the IR send
// test platform is ARDUINO NANO!!!
//===========================================

#include <IRremote.h>

IRsend irsend_3(3);

void setup(){

}

void loop(){
  irsend_3.sendRC5(0x89ABCDEF,32);

}