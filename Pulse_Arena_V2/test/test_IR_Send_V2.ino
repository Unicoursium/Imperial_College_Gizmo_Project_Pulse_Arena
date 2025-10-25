//===========================================
// This is the test on the IR send
// Version 2.0
// Update: we specified the send code
// we have tried on the communication between 2 ESP32s
//===========================================

#include <Arduino.h>
#include <IRremoteESP8266.h>
#include <IRsend.h>

IRsend irsend(16);

void setup() {
    irsend.begin();
}

void loop() {
    irsend.sendSony(0x5682E8A2, 32);
    delay(500);
}
