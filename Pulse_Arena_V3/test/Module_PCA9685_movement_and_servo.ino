//===========================================
// Module means this code is a function we wanna use in the project
// From pulse arena v2, we used to use ESP's PWM, but it may not be able to control 5 devices
// So we used a PCA9685 Board to drive everything.
// This time, we use controller
//===========================================

#include <Arduino.h>
#include <Wire.h>
#include <Bluepad32.h>
#include <Adafruit_PWMServoDriver.h>

//pca setup 
Adafruit_PWMServoDriver pca = Adafruit_PWMServoDriver(0x40);

//define pins
#define CH_SERVO 6
#define CH_LEFT_FWD   1
#define CH_RIGHT_FWD  2
#define CH_LEFT_BWD   3
#define CH_RIGHT_BWD  4
//define servo pwm
#define PCA_FREQ 50
#define SERVO_MIN_US 500
#define SERVO_MAX_US 2500
//setup gamepads
ControllerPtr controllers[BP32_MAX_GAMEPADS];
static const int DEADZONE = 35;

//motor stuff
void motorWrite(int channel, int value) {
    value = constrain(value, 0, 4095);
    pca.setPWM(channel, 0, value);
}

void motorSetLR(int left, int right) {
    if (left > 0) {
        motorWrite(CH_LEFT_FWD, left);
        motorWrite(CH_LEFT_BWD, 0);
    } else if (left < 0) {
        motorWrite(CH_LEFT_FWD, 0);
        motorWrite(CH_LEFT_BWD, -left);
    } else {
        motorWrite(CH_LEFT_FWD, 0);
        motorWrite(CH_LEFT_BWD, 0);
    }

    if (right > 0) {
        motorWrite(CH_RIGHT_FWD, right);
        motorWrite(CH_RIGHT_BWD, 0);
    } else if (right < 0) {
        motorWrite(CH_RIGHT_FWD, 0);
        motorWrite(CH_RIGHT_BWD, -right);
    } else {
        motorWrite(CH_RIGHT_FWD, 0);
        motorWrite(CH_RIGHT_BWD, 0);
    }
}

void mixDrive(float fwd, float turn) {
    float left = fwd + turn;
    float right = fwd - turn;

    float m = max(fabs(left), fabs(right));
    if (m > 1.0f) { left /= m; right /= m; }

    motorSetLR(left * 4095, right * 4095);
}

// servo stuff
float turretAngle = 90.0;
unsigned long lastUpdate = 0;
const float TURRET_SPEED = 15.0;

void writeServoAngle(float angle) {
    angle = constrain(angle, 0, 180);
    int us = map(angle, 0, 180, SERVO_MIN_US, SERVO_MAX_US);
    pca.writeMicroseconds(CH_SERVO, us);
}

void updateTurret(ControllerPtr gp) {
    /*
    rx:right handle x value(left&right)
    dz deadzone
    dt time interval

    in order to let turret move smoothly
    1. we need to update the turret angle in each loop
    2. we need to update the angle, so the time interval*speed=angle need to change.
    */
    int rx = gp->axisRX();
    int dz = 80;

    unsigned long now = millis();
    float dt = (now - lastUpdate) / 1000.0;
    if (dt <= 0) return;

    if (rx > dz) turretAngle += TURRET_SPEED * dt;
    else if (rx < -dz) turretAngle -= TURRET_SPEED * dt;

    if (gp->l1()) turretAngle -= TURRET_SPEED * dt * 1.2;
    if (gp->r1()) turretAngle += TURRET_SPEED * dt * 1.2;

    turretAngle = constrain(turretAngle, 0, 180);
    writeServoAngle(turretAngle);

    lastUpdate = now;
}
//=================================================================
//controller stuff, we've met some issues when debugging, and chatgpt solved that
void onConnected(ControllerPtr ctl) {
    for (int i = 0; i < BP32_MAX_GAMEPADS; i++) {
        if (!controllers[i]) {
            controllers[i] = ctl;
            Serial.println(">>> Gamepad Connected");
            return;
        }
    }
}

void onDisconnected(ControllerPtr ctl) {
    for (int i = 0; i < BP32_MAX_GAMEPADS; i++) {
        if (controllers[i] == ctl) {
            controllers[i] = nullptr;
            Serial.println(">>> Gamepad Disconnected");
        }
    }
}

void processGamepad(ControllerPtr gp) {
    uint8_t d = gp->dpad();
    //movement
    if (d & DPAD_UP)    { motorSetLR(4095, 4095); updateTurret(gp); return; }
    if (d & DPAD_DOWN)  { motorSetLR(-4095, -4095); updateTurret(gp); return; }
    if (d & DPAD_LEFT)  { motorSetLR(-4095, 4095); updateTurret(gp); return; }
    if (d & DPAD_RIGHT) { motorSetLR(4095, -4095); updateTurret(gp); return; }

    int lx = gp->axisX();
    int ly = gp->axisY();

    if (abs(lx) < DEADZONE) lx = 0;
    if (abs(ly) < DEADZONE) ly = 0;

    if (lx == 0 && ly == 0)
        motorSetLR(0, 0);
    else
        mixDrive(-(float)ly / 512.0f, (float)lx / 512.0f);
    //turret
    updateTurret(gp);
}

void processControllers() {
    for (int i = 0; i < BP32_MAX_GAMEPADS; i++) {
        if (!controllers[i]) continue;
        ControllerPtr gp = controllers[i];
        if (gp->hasData() && gp->isConnected()) processGamepad(gp);
    }
}

//=================================================================
//main
void setup() {
    Serial.begin(115200);
    pca.begin();
    pca.setPWMFreq(PCA_FREQ);
    delay(10);

    writeServoAngle(90);
    BP32.setup(&onConnected, &onDisconnected);
    lastUpdate = millis();

    Serial.println("=== TANK MOVEMENT + SERVO CONTROL ONLY ===");
}

void loop() {
    BP32.update();
    processControllers();
    delay(5);
}
