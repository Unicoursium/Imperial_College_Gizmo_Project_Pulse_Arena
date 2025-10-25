//===========================================
// Module means this code is a function we wanna use in the project
// for example, this one is test on servo interactions with controller.
//===========================================

#include <Arduino.h>
#include <Bluepad32.h>
#include <ESP32_Servo.h>

//startup
GamepadPtr gamepad;
Servo servo;

//define pins
const int servoPin = 18;

//Controller Connection startup
void onConnectedGamepad(GamepadPtr gp) {
    gamepad = gp;
}

void onDisconnectedGamepad(GamepadPtr gp) {
    if (gamepad == gp) gamepad = nullptr;
}

void setup() {
    BP32.setup(&onConnectedGamepad, &onDisconnectedGamepad);
    servo.attach(servoPin);
}

void loop() {
    BP32.update();
    if (gamepad && gamepad->isConnected()) {
        int gpx = gamepad->axisRX();
        int angle = map(gpx, -512, 512, 0, 180);
        servo.write(constrain(angle, 0, 180));
    }
    delay(10);
}
