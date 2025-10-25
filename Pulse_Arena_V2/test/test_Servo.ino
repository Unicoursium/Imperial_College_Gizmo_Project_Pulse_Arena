//===========================================
// This is the test on the Servo Functions
// just a simpe loop for servo from 0 to 180 and 180 to 0.
//===========================================


#include <ESP32_Servo.h>

Servo servo_27;

void setup(){
  servo_27.attach(27,500,2500);
}

void loop(){
  servo_27.write(0);
  delay(1000);
  servo_27.write(180);
  delay(1000);

}