//===========================================
// We've done the 4.0 version, and we've added a neopixel and NRF24L01 for better tank gameplay
// Functions added from v3:
// DOOM STATE
// blood indicator
// when tank is hit, blood indicator flashes the 2 leds that will be dimmed
// added fire cooldown, invincible time
// A button for resetting tank health
// in our tests, we've found an esp32 may connect to 2 controllers
// so we forced it to connect only one controller
//===========================================

/*
Requirements:
1. ESP x2
2. IR transmitter x2
3. IR receiver x4
4. Controller x2
5. Motor x4
6. Servo x2
7. H bridge x2
8. PCA9685 x2
9. Neopixel x2
10. power supply board x2
11. nrf24l01 x3
12. raspberrypi x1
13. speaker x1
*/

#include <Arduino.h>
#include <Wire.h>
#include <SPI.h>
#include <Bluepad32.h>
#include <RF24.h>
#include <Adafruit_PWMServoDriver.h>
#include <Adafruit_NeoPixel.h>
#include <IRremoteESP8266.h>
#include <IRsend.h>
#include <IRrecv.h>


//=================================================================
//define pins & startup


#define LED_PIN    2
#define LED_COUNT 10
Adafruit_NeoPixel strip(LED_COUNT, LED_PIN, NEO_GRB + NEO_KHZ800);

//execution button
#define PIN_EXEC_BUTTON 25
bool lastDoomKillBtn = HIGH;

//IR
#define IR_SEND_PIN 16
#define IR_RECV_PIN 27
IRsend irsend(IR_SEND_PIN);
IRrecv irrecv(IR_RECV_PIN);
decode_results irResults;

//hit code
#define HIT_CODE    0x5682E8A2
#define RESET_CODE  0x4985D7B4

//NRF24
RF24 radio(4, 5); // CE pin, CSN pin
const byte rfAddress[6] = "1SNSR"; //channel
char rfPayload[9]; //character size

//movement & turret
Adafruit_PWMServoDriver pca = Adafruit_PWMServoDriver(0x40);

#define CH_LEFT_FWD   1
#define CH_RIGHT_FWD  2
#define CH_LEFT_BWD   3
#define CH_RIGHT_BWD  4
#define CH_SERVO      5  

//controller setup
ControllerPtr controllers[BP32_MAX_GAMEPADS]; // using controllers[0]
bool controllerLocked = false;
static const int DEADZONE = 40;

//=================================================================
// Game State 

int hp = 10;

// Doom State
bool doomState = false;
int doomHP = 10;

unsigned long doomBlinkLast = 0;
bool doomBlinkOn = true;
const unsigned long DOOM_BLINK_MS = 250;

// Dead State
bool deadState = false;
unsigned long deadBlinkLast = 0;
bool deadBlinkOn = true;
const unsigned long DEAD_BLINK_MS = 400;

// Invincible timer
unsigned long lastHitTime = 0;
const unsigned long INVINCIBLE_MS = 1500;

// Turret
int turretAngle = 90;

// controller A button
bool lastA = false;

// Firing cooldown
unsigned long lastFireUpdate = 0;
const unsigned long FIRE_COOLDOWN_MS = 4000;

// The flashing function, flash when hit 
bool hitFlashActive = false;
int  hitFlashStart  = 0;
int  hitFlashEnd    = 0;
int  hitFlashToggles = 0;
bool hitFlashOn = true;
unsigned long hitFlashLast = 0;
const unsigned long HIT_FLASH_INTERVAL_MS = 120;

//=================================================================
// NRF24 setup

void sendText9(const char* text) {
    memset(rfPayload, 0, 9);
    strncpy(rfPayload, text, 9);
    radio.write(rfPayload, 9);
    }

    void sendShoot()    { sendText9("Shoot");    Serial.println("Shoot"); }
    void sendHit()      { sendText9("Hit");      Serial.println("Hit"); }
    void sendGameOver() { sendText9("Gameover"); Serial.println("Gameover"); }

//=================================================================
// NEOPIXEL functions

//flash when hit
void startHitFlash(int oldHp, int newHp) {
    if (newHp <= 0) return;
    if (oldHp <= newHp) return;

    hitFlashActive = true;
    hitFlashStart = newHp;
    hitFlashEnd = oldHp;
    hitFlashToggles = 6;
    hitFlashOn = true;
    hitFlashLast = millis();
}
//normal state
void showNormalHP() {
    if (hitFlashActive) {
        unsigned long now = millis();
        if (now - hitFlashLast >= HIT_FLASH_INTERVAL_MS) {
        hitFlashLast = now;
        hitFlashOn = !hitFlashOn;
        hitFlashToggles--;
        if (hitFlashToggles <= 0) hitFlashActive = false;
        }
    }

    strip.clear();
    for (int i = 0; i < hp; i++) {
        if (hp >= 7) strip.setPixelColor(i, strip.Color(0, 255, 0));
        else if (hp >= 4) strip.setPixelColor(i, strip.Color(255, 150, 0));
        else strip.setPixelColor(i, strip.Color(255, 0, 0));
    }

    if (hitFlashActive && hitFlashOn) {
        for (int i = hitFlashStart; i < hitFlashEnd; i++) {
        if (i >= 0 && i < LED_COUNT) strip.setPixelColor(i, strip.Color(255, 80, 0));
        }
    }

    strip.show();
}

//blink yellow at doom state
void showDoomBlink() {
    if (millis() - doomBlinkLast >= DOOM_BLINK_MS) {
        doomBlinkLast = millis();
        doomBlinkOn = !doomBlinkOn;
    }

    strip.clear();
    if (doomBlinkOn) {
        for (int i = 0; i < doomHP; i++) {
        strip.setPixelColor(i, strip.Color(255, 80, 0));
        }
    }
    strip.show();
}
//blink red at dead state
void showDeadBlink() {
    if (millis() - deadBlinkLast >= DEAD_BLINK_MS) {
        deadBlinkLast = millis();
        deadBlinkOn = !deadBlinkOn;
    }

    strip.clear();
    if (deadBlinkOn) {
        for (int i = 0; i < LED_COUNT; i++) {
        strip.setPixelColor(i, strip.Color(255, 0, 0));
        }
    }
    strip.show();
}
//send to neopixel
void updateHPLED() {
    if (deadState) showDeadBlink();
    else if (doomState) showDoomBlink();
    else showNormalHP();
}

//=================================================================
// State functions

//cannot move fwd & bwd when doom, fwd bwd leftrightturn when dead
void stopMotors() {
    pca.setPWM(CH_LEFT_FWD, 0, 0);
    pca.setPWM(CH_LEFT_BWD, 0, 0);
    pca.setPWM(CH_RIGHT_FWD, 0, 0);
    pca.setPWM(CH_RIGHT_BWD, 0, 0);
}

//doom state settings
void enterDoomState() {
    doomState = true;
    deadState = false;
    //doom state hp=10, normal hp=0
    hp = 0;
    doomHP = 10;

    hitFlashActive = false;

    doomBlinkLast = millis();
    doomBlinkOn = true;

    stopMotors();
    updateHPLED();
}

//dead state
void enterDeadState() {
    if (deadState) return;

    deadState = true;
    doomState = false;
    hitFlashActive = false;

    stopMotors();
    sendGameOver();

    deadBlinkLast = millis();
    deadBlinkOn = true;

    updateHPLED();
}

//reset all settings
void resetAll() {
    hp = 10;
    doomHP = 10;
    doomState = false;
    deadState = false;

    hitFlashActive = false;

    lastHitTime = millis();
    lastFireUpdate = 0;

    updateHPLED();
}

//=================================================================
// movement

void motorSetLR(int left, int right) {
    left  /= 2;
    right /= 2;

    left  = constrain(left, -4095, 4095);
    right = constrain(right, -4095, 4095);

    pca.setPWM(CH_LEFT_FWD,  0, (left > 0) ? left : 0);
    pca.setPWM(CH_LEFT_BWD,  0, (left < 0) ? -left : 0);

    pca.setPWM(CH_RIGHT_FWD, 0, (right > 0) ? right : 0);
    pca.setPWM(CH_RIGHT_BWD, 0, (right < 0) ? -right : 0);
}

void mixAndDrive(float forward, float turn) {
    //Forward, backward, rotating in place, and turning in an arc.
    float left = forward + turn;
    float right = forward - turn;

    float m = max(fabs(left), fabs(right));
    if (m > 1.0f) { left /= m; right /= m; }

    motorSetLR(left * 4095, right * 4095);
}

//=================================================================
// Turret 

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

    if (rx > 50) turretAngle--;
    if (rx < -50) turretAngle++;

    turretAngle = constrain(turretAngle, 30, 150);

    int pulse = map(turretAngle, 0, 180, 102, 512);
    pca.setPWM(CH_SERVO, 0, pulse);
}

//=================================================================
// Fire control
// cooldown
bool lastR2 = false;

void tryFire(const char* tag) {
    unsigned long now = millis();
    if (now - lastFireUpdate >= FIRE_COOLDOWN_MS) {
        lastFireUpdate = now;

        Serial.print("fire sent");
        if (tag) { Serial.print(" "); Serial.print(tag); }
        Serial.println();

        irsend.sendSony(HIT_CODE, 32);
        sendShoot();
    } else {
        Serial.print("in cooldown: ");
        Serial.println((FIRE_COOLDOWN_MS - (now - lastFireUpdate)) / 1000.0f, 2);
    }
}

//=================================================================
// Controller stuff

void processGamepad(ControllerPtr ctl) {
    //a button
    bool a = ctl->a();
    if (a && !lastA) {
        resetAll();
        Serial.println("reset sent");
        irsend.sendSony(RESET_CODE, 32);
    }
    lastA = a;
    //dead state 
    if (deadState) {
        stopMotors();
        return;
    }

    /*dpad, we've made a function about PWM
    DPad: Full speed, 
    left joystick: depend on the angle, we control the speed of the motor
    */
    uint8_t d = ctl->dpad();
    //doom state, left right turn only
    if (doomState) {
        if (d & DPAD_LEFT)  { motorSetLR(-4095, 4095); updateTurret(ctl); return; }
        if (d & DPAD_RIGHT) { motorSetLR(4095, -4095); updateTurret(ctl); return; }
        if (d & (DPAD_UP | DPAD_DOWN)) { motorSetLR(0, 0); updateTurret(ctl); return; }

        int lx = ctl->axisX();
        if (abs(lx) < DEADZONE) lx = 0;

        if (lx == 0) motorSetLR(0, 0);
        else mixAndDrive(0.0f, (float)lx / 512.0f);

        updateTurret(ctl);

        bool r2 = ctl->r2();
        if (r2 && !lastR2) tryFire("(doom)");
        lastR2 = r2;

        return;
    }

    if (d & DPAD_UP)    { motorSetLR(4095, 4095); updateTurret(ctl); return; }
    if (d & DPAD_DOWN)  { motorSetLR(-4095, -4095); updateTurret(ctl); return; }
    if (d & DPAD_LEFT)  { motorSetLR(-4095, 4095); updateTurret(ctl); return; }
    if (d & DPAD_RIGHT) { motorSetLR(4095, -4095); updateTurret(ctl); return; }

    int lx = ctl->axisX();
    int ly = ctl->axisY();

    if (abs(lx) < DEADZONE) lx = 0;
    if (abs(ly) < DEADZONE) ly = 0;

    if (lx == 0 && ly == 0) motorSetLR(0, 0);
    else mixAndDrive(-(float)ly / 512.0f, (float)lx / 512.0f);

    updateTurret(ctl);

    bool r2 = ctl->r2();
    if (r2 && !lastR2) tryFire(nullptr);
    lastR2 = r2;
}

void processControllers() {
    ControllerPtr ctl = controllers[0];
    if (!ctl) return;
    if (ctl->hasData() && ctl->isConnected())
        processGamepad(ctl);
}

//=================================================================
// controller stuff
void onConnectedController(ControllerPtr ctl) {
    if (!controllers[0]) {
        controllers[0] = ctl;
        controllerLocked = true;
        BP32.enableNewBluetoothConnections(false);
        Serial.println("Controller connected (slot 0) - LOCKED");
        return;
    }

    if (controllers[0] != ctl) {
        Serial.println("Reject extra controller (locked)");
        ctl->disconnect();
    }
}

void onDisconnectedController(ControllerPtr ctl) {
    if (controllers[0] == ctl) {
        controllers[0] = nullptr;
        Serial.println("Controller disconnected - still locked until RST");
    }
}

//=================================================================
// IR hit and Reset functions

void handleIRValue(uint32_t v) {
    if (v == RESET_CODE) {
        Serial.println("IR reset");
        resetAll();
        return;
    }

    if (v == HIT_CODE) {
        unsigned long now = millis();
        if (now - lastHitTime > INVINCIBLE_MS) {
        lastHitTime = now;
        sendHit();

        if (deadState) return;

        if (doomState) {
            doomHP -= 5;
            if (doomHP < 0) doomHP = 0;
            Serial.printf("doomHP=%d\n", doomHP);
            if (doomHP == 0) enterDeadState();
        } else {
            int oldHp = hp;
            hp -= 2;
            if (hp < 0) hp = 0;

            Serial.printf("HIT HP=%d\n", hp);

            if (hp == 0) enterDoomState();
            else startHitFlash(oldHp, hp);
        }
        }
        return;
    }
    
    Serial.print("0x");
    Serial.println(v, HEX);
}

void handleIR() {
    if (!irrecv.decode(&irResults)) return;
    handleIRValue((uint32_t)irResults.value);
    irrecv.resume();
}

//=================================================================
// execution button

void executionButton() {
    bool state = digitalRead(PIN_EXEC_BUTTON);
    if (lastDoomKillBtn == HIGH && state == LOW) {
        if (doomState && !deadState)
        enterDeadState();
    }
    lastDoomKillBtn = state;
}

//=================================================================
//main

void setup() {
    Serial.begin(115200);
    //pca start
    pca.begin();
    pca.setPWMFreq(50);
    //ir start
    irsend.begin();
    irrecv.enableIRIn();
    //neopixel start
    strip.begin();
    strip.setBrightness(85);
    strip.show();
    //execution
    pinMode(PIN_EXEC_BUTTON, INPUT_PULLUP);
    //rf24
    radio.begin();
    radio.setChannel(100);
    radio.setDataRate(RF24_250KBPS);
    radio.setPALevel(RF24_PA_MIN);
    radio.setPayloadSize(9);
    radio.openWritinctlipe(rfAddress);
    //bluepad32
    BP32.setup(&onConnectedController, &onDisconnectedController);

    Serial.println("start");
    updateHPLED();
}

void loop() {
    BP32.update();
    processControllers();
    handleIR();
    executionButton();
    updateHPLED();
    delay(5);
}
