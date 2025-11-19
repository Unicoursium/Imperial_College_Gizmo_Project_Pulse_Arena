//===========================================
// We've done the 3.0 version, and we've added a PCA9685 for low latency control and it is more accurate as well
// Functions are the same as v2:
// Shoot! (Sending IR Code) [ON Controller: ZR/RT button]
// Hit! (Receiving IR Code)
// Rotate Turret [ON Controller: Right handle]
// Movement [ON Controller: Left handle]
//===========================================

/*
Requirements:
1. ESP x2
2. IR transmitter x2
3. IR receiver x2
4. Controller x2
5. Motor x4
6. Servo x2
7. H bridge x2
8. PCA9685 x2
*/

#include <Arduino.h>
#include <Wire.h>
#include <Bluepad32.h>
#include <Adafruit_PWMServoDriver.h>
#include <IRremoteESP8266.h>
#include <IRsend.h>
#include <IRrecv.h>
#include <IRutils.h>

//=================================================================
//configuration zone
const float TURRET_SPEED = 40;
//=================================================================

//define pins & startup
//=================================================================
//PCA
Adafruit_PWMServoDriver pca = Adafruit_PWMServoDriver(0x40);
//PCA Channel
#define CH_SERVO 6
#define CH_LEFT_FWD   1
#define CH_RIGHT_FWD  2
#define CH_LEFT_BWD   3
#define CH_RIGHT_BWD  4
#define PCA_FREQ 50
#define SERVO_MIN_US 500
#define SERVO_MAX_US 2500
//=================================================================
//hit code
uint32_t HIT_CODE = 0x4517416A;
IRsend irsend(16);
IRrecv irrecv(17, 1024, 50, true);
decode_results results;
//hit led
#define PIN_HIT_LED 25
//controller setup
ControllerPtr controllers[BP32_MAX_GAMEPADS];
static const int DEADZONE = 35;
//=================================================================
//motor stuff
void motorWrite(int channel, int value) {
    //send things to pca
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

void mixAndDrive(float fwd, float turn) {
    //Forward, backward, rotating in place, and turning in an arc.
    float left = fwd + turn;
    float right = fwd - turn;

    float m = max(fabs(left), fabs(right));
    if (m > 1.0f) { left /= m; right /= m; }

    motorSetLR(left * 4095, right * 4095);
}
//=================================================================
//turret control
float turretAngle = 90.0;
unsigned long lastTurretUpdate = 0;

void writeServoAngle(float angle) {
    //send to pca
    angle = constrain(angle, 0, 180);
    int us = map(angle, 0, 180, SERVO_MIN_US, SERVO_MAX_US);
    pca.writeMicroseconds(CH_SERVO, us);
}

void updateTurret(ControllerPtr ctl) {
    /*
    rx:right handle x value(left&right)
    dz deadzone
    dt time interval

    in order to let turret move smoothly
    1. we need to update the turret angle in each loop
    2. we need to update the angle, so the time interval*speed=angle need to change.
    */
    int rx = ctl->axisRX();
    int dz = 80;

    unsigned long now = millis();
    float dt = (now - lastTurretUpdate) / 1000.0;
    if (dt <= 0) return;

    if (rx > dz) turretAngle += TURRET_SPEED * dt;
    else if (rx < -dz) turretAngle -= TURRET_SPEED * dt;

    if (ctl->l1()) turretAngle -= TURRET_SPEED * dt * 1.2;
    if (ctl->r1()) turretAngle += TURRET_SPEED * dt * 1.2;

    turretAngle = constrain(turretAngle, 0, 180);
    writeServoAngle(turretAngle);

    lastTurretUpdate = now;
}
//=================================================================
//controller stuff
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

bool lastR2 = false;

void processGamepad(ControllerPtr ctl) {
    uint8_t d = ctl->dpad();
    //movement
    if (d & DPAD_UP)    { motorSetLR(4095, 4095); updateTurret(ctl); return; }
    if (d & DPAD_DOWN)  { motorSetLR(-4095, -4095); updateTurret(ctl); return; }
    if (d & DPAD_LEFT)  { motorSetLR(-4095, 4095); updateTurret(ctl); return; }
    if (d & DPAD_RIGHT) { motorSetLR(4095, -4095); updateTurret(ctl); return; }

    int lx = ctl->axisX();
    int ly = ctl->axisY();

    if (abs(lx) < DEADZONE) lx = 0;
    if (abs(ly) < DEADZONE) ly = 0;

    if (lx == 0 && ly == 0)
        motorSetLR(0, 0);
    else
        mixAndDrive(-(float)ly / 512.0f, (float)lx / 512.0f);
    //turret
    updateTurret(ctl);

    bool r2 = ctl->r2();
    if (r2 && !lastR2) {
        Serial.println(">>> FIRE");
        irsend.sendOnkyo(HIT_CODE, 32);
    }
    lastR2 = r2;
}

void processControllers() {
    for (int i = 0; i < BP32_MAX_GAMEPADS; i++) {
        if (!controllers[i]) continue;
        ControllerPtr ctl = controllers[i];
        if (ctl->hasData() && ctl->isConnected()) processGamepad(ctl);
    }
}

void handleIrReceive() {
    if (irrecv.decode(&results)) {
        if (results.value == HIT_CODE) {
            digitalWrite(PIN_HIT_LED, HIGH);
            delay(150);
            digitalWrite(PIN_HIT_LED, LOW);
        }
        irrecv.resume();
    }
}

//=================================================================
//main
void setup() {
    Serial.begin(115200);
    //pca
    pca.begin();
    pca.setPWMFreq(PCA_FREQ);
    delay(10);
    //servo reset
    writeServoAngle(90);
    //set hit led pinout
    pinMode(PIN_HIT_LED, OUTPUT);

    irsend.begin();
    irrecv.enableIRIn();

    BP32.setup(&onConnected, &onDisconnected);

    lastTurretUpdate = millis();

    Serial.println("start");
}

void loop() {
    BP32.update();
    processControllers();
    handleIrReceive();
    delay(5);
}
