//===========================================
// Finally, we've done the 2.0 version, and we've combined every Modules
// Function is:
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
*/

#include <Arduino.h>
#include <Bluepad32.h>
#include <Servo.h>
#include <IRremoteESP8266.h>
#include <IRsend.h>
#include <IRrecv.h>
#include <IRutils.h>

//=================================================================
//configuration zone
const float TURRET_SPEED = 15;
//=================================================================






//define pins & startup

//=================================================================
//motor
static const int PIN_L_FWD  = 18;
static const int PIN_L_BWD  = 19;
static const int PIN_R_FWD  = 21;
static const int PIN_R_BWD  = 22;
//ir
static const int PIN_IR_TX  = 16;
static const int PIN_IR_RX  = 17;
//hit led
static const int PIN_HIT_LED = 23;
//servco
static const int PIN_TURRET_SERVO = 5;
static const int PWM_FREQ = 20000;
static const int PWM_RES  = 8;
//motor channel
static const int PWM_CH_L_FWD = 0;
static const int PWM_CH_L_BWD = 1;
static const int PWM_CH_R_FWD = 2;
static const int PWM_CH_R_BWD = 3;
//=================================================================
ControllerPtr myControllers[BP32_MAX_GAMEPADS];
// chatgpt gave me the advise to add a deadzone, so i added this 
static const int Deadzone = 35;
//hit code
#define HIT_CODE 0x4517416A

IRsend irsend(PIN_IR_TX);
IRrecv irrecv(PIN_IR_RX, 1024, 50, true);
decode_results results;
//servo obj
Servo turretServo;
float turretAngle = 90;
unsigned long lastTurretUpdate = 0;
//=================================================================
bool r2PressedLast = false;
//=================================================================
//functions

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
    float dt = (now - lastTurretUpdate) / 1000.0f;
    if (dt <= 0) return;

    float delta = 0;

    if (rx > dz) {
        delta = TURRET_SPEED * dt;
    }
    else if (rx < -dz) {
        delta = -TURRET_SPEED * dt;
    }

    turretAngle += delta;
    if (turretAngle < 0) turretAngle = 0;
    if (turretAngle > 180) turretAngle = 180;

    turretServo.write((int)turretAngle);
    lastTurretUpdate = now;
}

int clampi(int v, int lo, int hi) {
    // preventing out of bound
    if (v < lo) return lo;
    if (v > hi) return hi;
    return v;
}

void motorWrite(int chfwd, int chbwd, int speed255) {
    //send thing to tank
    speed255 = clampi(speed255, -255, 255);
    if (speed255 > 0) {
        ledcWrite(chfwd, speed255);
        ledcWrite(chbwd, 0);
    } else if (speed255 < 0) {
        ledcWrite(chfwd, 0);
        ledcWrite(chbwd, -speed255);
    } else {
        ledcWrite(chfwd, 0);
        ledcWrite(chbwd, 0);
    }
}

void driveTank(int left255, int right255) {
    motorWrite(PWM_CH_L_FWD, PWM_CH_L_BWD, left255);
    motorWrite(PWM_CH_R_FWD, PWM_CH_R_BWD, right255);
}

void mixAndDrive(float forward, float turn) {
    //Forward, backward, rotating in place, and turning in an arc.
    float left = forward + turn;
    float right = forward - turn;

    float m = max(fabs(left), fabs(right));
    if (m > 1.0f) { left /= m; right /= m; }

    driveTank(left * 255, right * 255);
}
//=================================================================
//controller stuff, we've met some issues when debugging, and chatgpt solved that
void onConnectedController(ControllerPtr ctl) {
    for (int i = 0; i < BP32_MAX_GAMEPADS; i++) {
        if (!myControllers[i]) {
            myControllers[i] = ctl;
            Serial.println(">>> Controller connected");
            return;
        }
    }
}

void onDisconnectedController(ControllerPtr ctl) {
    for (int i = 0; i < BP32_MAX_GAMEPADS; i++) {
        if (myControllers[i] == ctl) {
            myControllers[i] = nullptr;
            Serial.println(">>> Controller disconnected");
            driveTank(0, 0);
            return;
        }
    }
}

void processGamepad(ControllerPtr ctl) {
    uint8_t d = ctl->dpad();
    //movement
    if (d & DPAD_UP )  { driveTank(255, 255); updateTurret(ctl); return; }
    if (d & DPAD_DOWN) { driveTank(-255, -255); updateTurret(ctl); return; }
    if (d & DPAD_LEFT) { driveTank(-255, +255); updateTurret(ctl); return; }
    if (d & DPAD_RIGHT){ driveTank(+255, -255); updateTurret(ctl); return; }

    int lx = ctl->axisX();
    int ly = ctl->axisY();
    if (abs(lx) < Deadzone) lx = 0;
    if (abs(ly) < Deadzone) ly = 0;

    if (lx == 0 && ly == 0) driveTank(0, 0);
    else mixAndDrive(-(float)ly / 512.0f, (float)lx / 512.0f);
    //turret
    updateTurret(ctl);

    bool r2 = ctl->r2();

    if (r2 && !r2PressedLast) {
        Serial.println("fired");
        irsend.sendSony(HIT_CODE, 32);
        r2PressedLast = true;
    }
    else if (!r2) {
        r2PressedLast = false;
    }
}

void processControllers() {
    for (int i = 0; i < BP32_MAX_GAMEPADS; i++) {
        ControllerPtr c = myControllers[i];
        if (!c || !c->isConnected() || !c->hasData()) continue;
        processGamepad(c);
    }
}

void handleIrReceive() {
    if (irrecv.decode(&results)) {
        uint32_t val = results.value;
        Serial.print("IR code-- ");
        Serial.println(val, HEX);

        if (val == HIT_CODE) {
            Serial.println("HIT!");
            digitalWrite(PIN_HIT_LED, HIGH);
            delay(120);
            digitalWrite(PIN_HIT_LED, LOW);
        }

        irrecv.resume();
    }
}



//=================================================================
//main
void setup() {
    Serial.begin(115200);
    delay(300);

    ledcSetup(PWM_CH_L_FWD, PWM_FREQ, PWM_RES);
    ledcSetup(PWM_CH_L_BWD, PWM_FREQ, PWM_RES);
    ledcSetup(PWM_CH_R_FWD, PWM_FREQ, PWM_RES);
    ledcSetup(PWM_CH_R_BWD, PWM_FREQ, PWM_RES);

    ledcAttachPin(PIN_L_FWD, PWM_CH_L_FWD);
    ledcAttachPin(PIN_L_BWD, PWM_CH_L_BWD);
    ledcAttachPin(PIN_R_FWD, PWM_CH_R_FWD);
    ledcAttachPin(PIN_R_BWD, PWM_CH_R_BWD);

    turretServo.attach(PIN_TURRET_SERVO);
    turretServo.write(turretAngle);
    lastTurretUpdate = millis();

    irsend.begin();
    irrecv.enableIRIn();

    pinMode(PIN_HIT_LED, OUTPUT);

    BP32.setup(&onConnectedController, &onDisconnectedController);

    Serial.println("start");
}

void loop() {
    BP32.update();
    processControllers();
    handleIrReceive();
    delay(5);
}
