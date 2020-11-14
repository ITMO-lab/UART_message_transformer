#include <SoftwareSerial.h>
#include <functional>
#include <queue>
#include <vector>
#include"SendMessageRequest.h"

SoftwareSerial BleSerial(2, 3); // RX,TX

const int input_message_length = 5;
const int output_message_length = 5;


/*! 
    /brief Размер очереди входящих по BLE сообщений.

    Это значение определяет размер очереди сообщений
    на пересылку.
    Если на новое сообщение не будет места, оно будет
    пропущено. Как только появится свободное место,
    самое новое не использованное сообщение будет 
    записано из очереди.
    Если программе не хватает оперативной
    памяти, можно уменьшить это значение.
 */
const int priority_queue_max_size = 16;

/*!
    \brief Количество сообщений в очереди на преобразование и отправку.

    В случае переполненря опервтивной памяти - снизить.
    Это значение всё равно должно быть
    не меньше input_message_length.
 */
const int Ble_data_size = 16;

// \brief миллисекунды.
const uint64_t Serial_send_delay = 100;


// \brief Буфер входящих сообщений.
byte Ble_messages[Ble_data_size];

/*! 
    \brief Узазатель на конец буфера

    Простейшие динамический массив.
*/
int Ble_input_pointer = 0;


uint64_t current_time = 0;
uint32_t prev_HAL_Tick = 0;
/*!
    \brief Фугкция, возвращающая количество миллисекунд с момента старта программы.

    Решение проблемы с переполнением таймера
    примерно после 3 лет непревной работы.
    Теперь этот код переживёт плату, на которой запущен.
 */
uint64_t get_millis(){
  uint32_t current_HAL_Tick = HAL_GetTick();
  uint32_t add_time = current_HAL_Tick - prev_HAL_Tick;
  prev_HAL_Tick = current_HAL_Tick;
  current_time += (uint64_t) add_time;
  return current_time;
}


/*!
    \brief Функция для вычисления хэш суммы

    Логика былп задана изначально.
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

// \brief Компаратор для очереди с преоритетом.
auto cmp = [](SendMessageRequest a, SendMessageRequest b) { return a.get_send_time() > b.get_send_time(); };
// \brief Очередь запросов на отправку сообщений.
std::priority_queue<SendMessageRequest, std::deque<SendMessageRequest>, decltype(cmp)> Ble_send_queue(cmp);

/*!
    \brief Функция для преобразования входящего сообщения в выходящее.

    Вынесена отдельно с целью удобства внесения изменений.
 */
std::vector<byte> message_converter(std::vector<byte> input_message){
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


/*!
    \brief Обработчик новых символов.

    Сделано отдельной функцией с целью экономии
    процессорного времени.
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


/*!
    \brief Функция обновления очереди с преоритетом
    для запросов на отправку сообщений.
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


// \brief Операция отправки первого сообщения, чья очередь дошла.
void Ble_send_one_message(){
  if (Ble_send_queue.empty())
    return;
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
    \brief Операция инициализации преобразователя BLE сообщений в Serial..

    Она была вынесенв из setup() с целью
    Упрощения процесса вставки 
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
