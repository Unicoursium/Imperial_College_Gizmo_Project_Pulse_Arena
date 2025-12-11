// ==========================================
// NeoPixel HP Bar Test
// HP starts at 10 and decreases by 2 in every 0.5s
// ==========================================

#include <Arduino.h>
#include <Adafruit_NeoPixel.h>

#define LED_PIN   2
#define LED_COUNT 10

//startup
Adafruit_NeoPixel strip(LED_COUNT, LED_PIN, NEO_GRB + NEO_KHZ800);

// initialize hp
int hp = 10;
unsigned long lastUpdate = 0;
const unsigned long hpDrainInterval = 500;

void showHP() {
    strip.clear();
    //blood color
    for (int i = 0; i < hp; i++) {
        if (hp >= 7) strip.setPixelColor(i, strip.Color(0, 255, 0));        
        else if (hp >= 4) strip.setPixelColor(i, strip.Color(255, 150, 0)); 
        else strip.setPixelColor(i, strip.Color(255, 0, 0));                
    }

    strip.show();
}

void setup() {
    Serial.begin(115200);
    //strip init
    strip.begin();
    //it's toooooooooooooooooooo shiny
    strip.setBrightness(85);
    strip.show();

    Serial.println("hp test");
}

void loop() {
  unsigned long now = millis();

  // Decrease HP every 0.5 seconds
  if (now - lastUpdate >= hpDrainInterval) {
    lastUpdate = now;
    //start decreasing
    if (hp > 0) {
      hp--;
      showHP();
    } 
    else {
    
      strip.clear();
      strip.show();
      while (true) delay(100);
    }
  }
}
