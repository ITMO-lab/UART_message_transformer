//-----------------------------
//------------DEBUG------------
//-----------------------------

#include"BleConverter.cpp"

BleConverter *ble;
void setup() {
  ble = new BleConverter(9600, 9600, 2, 3);
}

void loop() {
  ble->BleSerial_cycle();
}
