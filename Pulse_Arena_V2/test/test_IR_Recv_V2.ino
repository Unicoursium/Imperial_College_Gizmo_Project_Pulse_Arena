//===========================================
// This is the test on the IR receive
// Version 2.0
// Update: we specified the send code
// we have tried on the communication between 2 ESP32s

//if received the code, the LED on GPIO25 will blink!

//===========================================

#include <Arduino.h>
#include <IRremoteESP8266.h>
#include <IRrecv.h>
#include <IRutils.h>

IRrecv irrecv(17);
decode_results results;

void setup() {
    pinMode(25, OUTPUT);
    irrecv.enableIRIn();
}

void loop() {
    if (irrecv.decode(&results)) {
        if (results.value == 0x5682E8A2) {
            digitalWrite(25, HIGH);
            delay(150);
            digitalWrite(25, LOW);
        }
        irrecv.resume();
    }
}
