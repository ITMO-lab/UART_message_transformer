//-----------------------------
//------------DEBUG------------
//-----------------------------

#include "src/BleConverter.h"

BleConverter *ble;
void setup() {
  ble = new BleConverter(11, 12, 9600, 2, 3, 9600);
}

void loop() {
  ble->BleSerial_cycle();
}
