//===========================================
// This is the test on the controller output and keys
// some of the codes are copied from Example
//===========================================


#include <Bluepad32.h>

// Controller startup
ControllerPtr myController = nullptr;

void onConnected(ControllerPtr ctl) {
  Serial.println("Controller connected");
  myController = ctl;
}

void onDisconnected(ControllerPtr ctl) {
  Serial.println("Controller disconnected");
  if (myController == ctl) {
    myController = nullptr;
  }
}


//startup
void setup() {
  Serial.begin(115200);
  // initialize bp32
  BP32.setup(&onConnected, &onDisconnected);
  BP32.forgetBluetoothKeys();
  Serial.println("Pairing");
}

void loop() {

  BP32.update();

  if (myController && myController->isConnected()) {
    int lx = myController->axisX(); 
    int ly = myController->axisY();   

    int rx = myController->axisRX();
    int ry = myController->axisRY();

    bool aPressed = myController->a();

    // print the info to the serial
    Serial.print("LX: "); Serial.print(lx);
    Serial.print("  LY: "); Serial.print(ly);
    Serial.print("  RX: "); Serial.print(rx);
    Serial.print("  RY: "); Serial.print(ry);
    Serial.print("  A: "); Serial.println(aPressed ? "Pressed" : "Released");
    delay(50);

  } else {
    static uint32_t lastPrint = 0;
    if (millis() - lastPrint > 1000) {
      Serial.println("Pairing");
      lastPrint = millis();
    }
  }
}
