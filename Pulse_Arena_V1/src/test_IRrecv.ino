//===========================================
// This is the test on the IR receiving
//===========================================
#include <IRremote.h>

//set IR protocol
const String IR_PROTOCOL_TYPE[] = {
  "UNKNOWN",
  "SONY"
};
IRrecv irrecv_0(0);

void setup(){
  Serial.begin(115200);
  irrecv_0.enableIRIn();
}

//test IR receiving
void loop(){
  if (irrecv_0.decode()) {
    struct IRData *pIrData = &irrecv_0.decodedIRData;
    long ir_item = pIrData->decodedRawData;
    String irProtocol = IR_PROTOCOL_TYPE[pIrData->protocol];
    Serial.print("IR TYPE:" + irProtocol + "\tVALUE:");
    Serial.println(ir_item, HEX);
    irrecv_0.resume();
    Serial.println(ir_item,HEX);

  } else {
    //do nothing lol
  }

}