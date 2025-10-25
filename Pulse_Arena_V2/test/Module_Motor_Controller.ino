//===========================================
// Module means this code is a function we wanna use in the project
// for example, this one is test on the movement of the tank base
// we have connected the esp to an h bridge.
// the h bridge is connected to 2 motors.
//===========================================

#include <Arduino.h>
#include <Bluepad32.h>

GamepadPtr gamepad;
//define pins
const int A1 = 18;
const int A2 = 19;
const int B1 = 21;
const int B2 = 22;

//Controller Connection startup
void onConnectedGamepad(GamepadPtr gp) {
    gamepad = gp;
}

void onDisconnectedGamepad(GamepadPtr gp) {
    if (gamepad == gp) gamepad = nullptr;
}

//update value to the h bridge
void setMotor(int p1, int p2, int value) {
    if (value > 0) {
        digitalWrite(p1, HIGH);
        digitalWrite(p2, LOW);
    } else if (value < 0) {
        digitalWrite(p1, LOW);
        digitalWrite(p2, HIGH);
    } else {
        digitalWrite(p1, LOW);
        digitalWrite(p2, LOW);
    }
}

void setup() {
    pinMode(A1, OUTPUT);
    pinMode(A2, OUTPUT);
    pinMode(B1, OUTPUT);
    pinMode(B2, OUTPUT);
    BP32.setup(&onConnectedGamepad, &onDisconnectedGamepad);
}

void loop() {
    //controller input
    BP32.update();
    if (!gamepad || !gamepad->isConnected()) return;
    int x = gamepad->axisX();
    int y = -gamepad->axisY();
    //calculation on the movement
    int left = y + x;
    int right = y - x;
    left = constrain(left, -512, 512);
    right = constrain(right, -512, 512);
    //send everything to h bridge
    setMotor(A1, A2, left);
    setMotor(B1, B2, right);

    delay(10);
}
