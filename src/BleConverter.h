#ifndef BLECONVERTER_H
#define BLECONVERTER_H

#include <SoftwareSerial.h>
#include <functional>
#include <queue>
#include <vector>
#include"SendMessageRequest.hpp"


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
  uint64_t get_millis();
  
  
  /**
    * @brief Функция для вычисления хэш суммы.
    * @detailed Логика была задана изначально.
    * @param[in] *pcBlock Входной массив, хэш которого нужно найти.
    * @param[in] len Длина входного массива.
    * @return 8 бит результата хэш суммы.
  */
  char crc(unsigned char *pcBlock, int len);
  

  /**
    * @brief Компаратор для очереди с приоритетом.
  */
  struct CustomCompare{
    bool operator()(SendMessageRequest lhs, SendMessageRequest rhs){
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
  std::vector<byte> message_converter(std::vector<byte> input_message);
  

  /**
    * @brief Обработчик новых символов.
    * @detailed Сделано отдельной функцией с целью экономии
    * процессорного времени.
    * @param[in] symbol Новый символ, пришедший по Ble.
  */
  void Ble_update_sym(byte symbol);


  /**
    * @brief Функция обновления очереди с приоритетом
    * для запросов на отправку сообщений.
  */
  void Ble_update_send_queue();
  
  
  /**
    * @brief Операция отправки первого сообщения, чья очередь дошла.
  */
  void Ble_send_one_message();


  public:
  /**
    * @brief Один цикл работы программы.
    * @detailed Он была вынесен из loop() с целью
    * Упрощения процесса использования кода.
  */
  void BleSerial_cycle();


  /**
    * @brief Операция инициализации преобразователя BLE сообщений в Serial.
    * @detailed Она была вынесенв из setup() в конструктор с целью
    * Упрощения процесса использования кода.
    * @param[in] SerialBaudrate Частота работы UART BLE. Для HM-10 - 9600.
    * @param[in] BleBaudrate Частота работы Serial UART.
    * @param[in] BleRxPin Номер пина RX UART BLE.
    * @param[in] BleTxPin Номер пина TX UART BLE.
  */
  BleConverter(int SerialBaudrate, int BleBaudrate, int BleRxPin, int BleTxPin);
};

#endif
