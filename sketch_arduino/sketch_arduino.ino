#include <SoftwareSerial.h>
#include <functional>
#include <queue>
#include <vector>
#include"SendMessageRequest.h"

SoftwareSerial BleSerial(2, 3); // RX,TX

const int input_message_length = 5;
const int output_message_length = 5;


/* This value determines the size of the forwarding message queue. 
 *  If there is not enough space for a new message, 
 *  it will be dropped. When space becomes available, 
 *  the newest message will be recorded.
 */
const int priority_queue_max_size = 16;

/* Amount of messages in the queue for conversion and sending.
 * In case of memory overflow, reduce.
 * It's still must be at least as input_message_length size.
 */
const int Ble_data_size = 16;

const uint64_t Serial_send_delay = 100; // milliseconds.


// Input message buffer.
byte Ble_messages[Ble_data_size];

// Pointer to the end of the buffer. (Simplest dynamic array)
int Ble_input_pointer = 0;


/*
 * Solving problem with timer overflow after ~3 years.
 * Now it will survive our galaxy.
 */
uint64_t current_time = 0;
uint32_t prev_HAL_Tick = 0;
uint64_t get_millis(){
  uint32_t current_HAL_Tick = HAL_GetTick();
  uint32_t add_time = current_HAL_Tick - prev_HAL_Tick;
  prev_HAL_Tick = current_HAL_Tick;
  current_time += (uint64_t) add_time;
  return current_time;
}


/* Function for calculating hash sum.
 * Logic was set initially.
 */
char crc(unsigned char *pcBlock, int len)
{
    unsigned char crc = 0xFF;
    unsigned int i;
    while (len--)
    {
        crc ^= *pcBlock++;
        for (i = 0; i < 8; i++) crc = crc & 0x80 ? (crc << 1) ^ 0x31 : crc << 1;
    }
    return crc;
}

// Comparator for priority queue
auto cmp = [](SendMessageRequest a, SendMessageRequest b) { return a.get_send_time() > b.get_send_time(); };
// Queue of requests to send messages.
std::priority_queue<SendMessageRequest, std::deque<SendMessageRequest>, decltype(cmp)> Ble_send_queue(cmp);

/*
 * Сonvertint an input message to an output message. 
 * Made separately to make changes convenient.
 */
std::vector<byte> message_converter(std::vector<byte> input_message){
  std::vector<byte> result(output_message_length);
  result[0] = 0xFF;
  result[1] = 0x02;
  result[2] = 0x14;
  result[3] = (input_message[2] << 7) | input_message[3];
  byte result_for_hash[4];
  std::copy(result.begin(), result.end(), result_for_hash);
  //result[4] = crc(result_for_hash, 4);
  result[4] = (byte)Ble_send_queue.size();
  return result;
}


/*
 * New symbol handler. 
 * It was made as a separate function in order to save CPU time.
 */
void Ble_update_sym(byte symbol){
  if (symbol & 0x80){
    Ble_input_pointer = 0;
  }
  Ble_messages[Ble_input_pointer] = symbol;
  Ble_input_pointer++;
  if (Ble_input_pointer >= input_message_length){
    Ble_update_send_queue();
  }
}



/*
 * Priority queue for requests to send messages update operation.
 */
void Ble_update_send_queue(){
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


/*
 * The operation of sending the first valid message 
 * from the priority queue, to which the time has come.
 */
void Ble_send_one_message(){
  current_time = get_millis();
  SendMessageRequest message_request = Ble_send_queue.top();
  if (current_time < message_request.get_send_time()){
    return;
  }
  std::vector<byte> message_bytes = message_request.get_message();
  if (message_request.is_software_serial()){
    for (int i = 0; i < input_message_length; i++){
      BleSerial.write(message_bytes[i]);
    }
  }
  else {
    for (int i = 0; i < output_message_length; i++){
      Serial.write(message_bytes[i]);
    }
  }
  Ble_send_queue.pop();
}


/*
 * Whole program initialization function. 
 * It was moved out from setup() 
 * for esier integration into existing code.
 */
void BleSerial_init(){
  Serial.begin(115200);
  BleSerial.begin(9600);
}


/*
 * One loop program function.
 * It was moved out from loop() 
 * for esier integration into existing code.
 */
void BleSerial_cycle(){
  if (BleSerial.available()) {
    Ble_update_sym(BleSerial.read());
  }
  Ble_send_one_message();
}


//-----------------------------
//-----------------------------
//-----------------------------

void setup() {
  BleSerial_init();
}

void loop() {
  BleSerial_cycle();
}
