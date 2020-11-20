#include <SoftwareSerial.h>
#include <functional>
#include <queue>
#include <vector>
#include "BleConverter.h"


uint64_t BleConverter::get_millis(){
  uint32_t current_HAL_Tick = HAL_GetTick();
  uint32_t add_time = current_HAL_Tick - prev_HAL_Tick;
  prev_HAL_Tick = current_HAL_Tick;
  current_time += (uint64_t) add_time;
  return current_time;
}


char BleConverter::crc(unsigned char *pcBlock, int len){
  unsigned char crc = 0xFF;
  unsigned int i;
  while (len--){
    crc ^= *pcBlock++;
    for (i = 0; i < 8; i++) crc = crc & 0x80 ? (crc << 1) ^ 0x31 : crc << 1;
  }
  return crc;
}


std::vector<byte> BleConverter::message_converter(std::vector<byte> input_message){
  std::vector<byte> result(output_message_length);
  result[0] = 0xFF;
  result[1] = 0x02;
  result[2] = 0x14;
  result[3] = (input_message[2] << 7) | input_message[3];
  byte result_for_hash[4];
  std::copy(result.begin(), result.end(), result_for_hash);
  result[4] = crc(result_for_hash, 4);
  return result;
}


void BleConverter::Ble_update_sym(byte symbol){
  if (symbol & 0x80){
    Ble_input_pointer = 0;
  }
  Ble_messages[Ble_input_pointer] = symbol;
  Ble_input_pointer++;
  if (Ble_input_pointer >= Ble_data_size){
    Ble_input_pointer = 0;
  }
  if (Ble_input_pointer >= input_message_length){
    Ble_update_send_queue();
  }
}


void BleConverter::Ble_update_send_queue(){
  current_time = get_millis();
  std::vector<byte> message_to_send(input_message_length);
  for (int i = 0; i < input_message_length; i++){
    message_to_send[i] = Ble_messages[i];
  }
  byte hash_fast = message_to_send[0];
  for (int i = 1; i < input_message_length - 1; i++){
    hash_fast ^= message_to_send[i];
  }
  hash_fast &= 0x7F;
  if (hash_fast != message_to_send[4]){
    return;
  }
  if (Ble_send_queue.size() > priority_queue_max_size){
    return;
  }
  std::vector<byte> message_to_send_converted = message_converter(message_to_send);
  Ble_send_queue.push(SendMessageRequest(message_to_send, current_time, true));
  for (uint64_t i = 0; i < 3; i++){
    Ble_send_queue.push(SendMessageRequest(message_to_send_converted, i*Serial_send_delay + current_time, false));
  }
  Ble_input_pointer = 0;
}


void BleConverter::Ble_send_one_message(){
  if (Ble_send_queue.empty())
    return;
  current_time = get_millis();
  SendMessageRequest message_request = Ble_send_queue.top();
  if (current_time < message_request.get_send_time()){
    return;
  }
  std::vector<byte> message_bytes = message_request.get_message();
  if (message_request.is_ble_serial()){
    for (int i = 0; i < input_message_length; i++){
      BleSerial->write(message_bytes[i]);
    }
  }
  else {
    for (int i = 0; i < output_message_length; i++){
      ProsthesisSerial->write(message_bytes[i]);
    }
  }
  Ble_send_queue.pop();
}


void BleConverter::BleSerial_cycle(){
  if (BleSerial->available()) {
    Ble_update_sym(BleSerial->read());
  }
  Ble_send_one_message();
}


BleConverter::BleConverter(int SerialRxPin, int SerialTxPin, int SerialBaudrate, int BleRxPin, int BleTxPin, int BleBaudrate){
  Ble_messages.reserve(Ble_data_size);
  ProsthesisSerial = new SoftwareSerial(SerialRxPin, SerialTxPin);
  ProsthesisSerial->begin(SerialBaudrate);
  BleSerial = new SoftwareSerial(BleRxPin, BleTxPin);
  BleSerial->begin(BleBaudrate);
}
