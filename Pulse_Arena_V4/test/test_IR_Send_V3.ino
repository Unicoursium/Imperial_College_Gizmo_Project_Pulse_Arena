//===========================================
// IR SENDING TEST
// Only send HIT_CODE & RESET_CODE
// ZR / R2  Send HIT_CODE
// A button Send RESET_CODE
//===========================================

#include <Arduino.h>
#include <Bluepad32.h>
#include <IRremoteESP8266.h>
#include <IRsend.h>

//define pins
#define IR_SEND_PIN 16
IRsend irsend(IR_SEND_PIN);

//hit code
#define HIT_CODE   0x5682E8A2
#define RESET_CODE 0x4985D7B4

//controller initial state
ControllerPtr controllers[BP32_MAX_GAMEPADS];
bool lastR2 = false;
bool lastA  = false;

//=================================================================
//controller setup
//copied from pulse arena v3, forgot to state that this code is ChatGPT refined
//every file i'll add this block of code lol

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

//=================================================================
//sending code
void sendHit() {
  Serial.println("fire");
  irsend.sendSony(HIT_CODE, 32);
}

void sendReset() {
  Serial.println("reset");
  irsend.sendSony(RESET_CODE, 32);
}

//=================================================================
//controller stuff
void processGamepad(ControllerPtr gp) {
  // detecting press
  bool r2 = gp->r2();
  if (r2 && !lastR2) sendHit();
  lastR2 = r2;

  bool a = gp->a();
  if (a && !lastA) sendReset();
  lastA = a;
}

void processControllers() {
  if (!controllers[0]) return;

  ControllerPtr gp = controllers[0];
  if (gp->hasData() && gp->isConnected())
    processGamepad(gp);
}

//=================================================================
//main
void setup() {
  Serial.begin(115200);

  // IR sender
  irsend.begin();

  // Controller init
  BP32.setup(&onConnectedController, &onDisconnectedController);
}

void loop() {
  BP32.update();
  processControllers();
  delay(5);
}
