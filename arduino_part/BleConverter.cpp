#include <SoftwareSerial.h>
#include <functional>
#include <queue>
#include <vector>
#include"SendMessageRequest.h"


class BleConverter
{
  protected:
  
  SoftwareSerial *BleSerial;

  /**
    * @brief Размер входящего по Ble сообщения.
  */
  const int input_message_length = 5;
  
  
  /**
    * @brief Размер выходящего по Serial сообщения.
  */
  const int output_message_length = 5;
  
  
  /**
    * @brief Размер очереди входящих по BLE сообщений.
    * @detailed Это значение определяет размер очереди сообщений
    * на пересылку.
    * Если на новое сообщение не будет места, оно будет
    * пропущено. Как только появится свободное место,
    * самое новое не использованное сообщение будет 
    * записано из очереди.
    * Если программе не хватает оперативной
    * памяти, можно уменьшить это значение.
  */
  const int priority_queue_max_size = 16;
  
  
  /**
    * @brief Количество символов в очереди на преобразование и отправку.
    * @detailed В случае переполнения оперативной памяти - снизить.
    * Это значение всё равно должно быть
    * не меньше input_message_length.
  */
  const int Ble_data_size = 16;
  
  
  /**
    * @brief миллисекунды.
  */
  const uint64_t Serial_send_delay = 100;
  
  
  /**
    * @brief Буфер входящих сообщений.
  */
  std::vector<byte> Ble_messages;
  
  
  /**
    * @brief Указатель на конец буфера
    * @detailed Простейшие динамический массив.
  */
  int Ble_input_pointer = 0;
  
  
  uint64_t current_time = 0;
  uint32_t prev_HAL_Tick = 0;


  /**
    * @brief Функция, возвращающая количество миллисекунд с момента старта программы.
    * @detailed Решение проблемы с переполнением таймера
    * примерно после 3 лет непрерывной работы.
    * Теперь этот код переживёт плату, на которой запущен.
    * @return uint64_t Миллисекунды с момента старта программы.
  */
  uint64_t get_millis(){
    uint32_t current_HAL_Tick = HAL_GetTick();
    uint32_t add_time = current_HAL_Tick - prev_HAL_Tick;
    prev_HAL_Tick = current_HAL_Tick;
    current_time += (uint64_t) add_time;
    return current_time;
  }
  
  
  /**
    * @brief Функция для вычисления хэш суммы.
    * @detailed Логика была задана изначально.
    * @param[in] *pcBlock Входной массив, хэш которого нужно найти.
    * @param[in] len Длина входного массива.
    * @return 8 бит результата хэш суммы.
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
  

  /**
    * @brief Компаратор для очереди с приоритетом.
  */
  struct CustomCompare
  {
      bool operator()(SendMessageRequest lhs, SendMessageRequest rhs)
      {
          return lhs.get_send_time() > rhs.get_send_time();
      }
  };
  
  
  /**
    * @brief Очередь запросов на отправку сообщений.
  */
  std::priority_queue<SendMessageRequest, std::deque<SendMessageRequest>, CustomCompare> Ble_send_queue;
  
  
  /**
    * @brief Функция для преобразования входящего сообщения в выходящее.
    * @detailed Вынесена отдельно с целью удобства внесения изменений.
    * @param[in] input_message Входящее в BLE сообщение для преобразования.
    * @return Выходящее сообщение, отправляемое по Serial порту.
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
  

  /**
    * @brief Обработчик новых символов.
    * @detailed Сделано отдельной функцией с целью экономии
    * процессорного времени.
    * @param[in] symbol Новый символ, пришедший по Ble.
  */
  void Ble_update_sym(byte symbol){
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


  /**
    * @brief Функция обновления очереди с приоритетом
    * для запросов на отправку сообщений.
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
  
  
  /**
    * @brief Операция отправки первого сообщения, чья очередь дошла.
  */
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
        BleSerial->write(message_bytes[i]);
      }
    }
    else {
      for (int i = 0; i < output_message_length; i++){
        Serial.write(message_bytes[i]);
      }
    }
    Ble_send_queue.pop();
  }


  public:
  /**
    * @brief Один цикл работы программы.
    * @detailed Он была вынесен из loop() с целью
    * Упрощения процесса использования кода.
  */
  void BleSerial_cycle(){
    if (BleSerial->available()) {
      Ble_update_sym(BleSerial->read());
    }
    Ble_send_one_message();
  }


  /**
    * @brief Операция инициализации преобразователя BLE сообщений в Serial.
    * @detailed Она была вынесенв из setup() в конструктор с целью
    * Упрощения процесса использования кода.
    * @param[in] SerialBaudrate Частота работы UART BLE. Для HM-10 - 9600.
    * @param[in] BleBaudrate Частота работы Serial UART.
    * @param[in] BleRxPin Номер пина RX UART BLE.
    * @param[in] BleTxPin Номер пина TX UART BLE.
  */
  BleConverter(int SerialBaudrate, int BleBaudrate, int BleRxPin, int BleTxPin){
    Ble_messages.reserve(Ble_data_size);
    BleSerial = new SoftwareSerial(BleRxPin, BleTxPin);
    BleSerial->begin(BleBaudrate);
    Serial.begin(SerialBaudrate);
  }
};
