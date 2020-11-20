#include <vector>


/**
  * @brief Класс запроса на отправку сообщения 
  * по одному из каналов данных.
  * @detailed В некотором смысле используется 
  * просто как безопасный аналог struct.
*/
class SendMessageRequest{
  public:

  SendMessageRequest(std::vector<byte> message, uint64_t send_time, bool ble_serial){
    for (int i = 0; i < message.size(); i++){
      this->message = message;
    }
    this->send_time = send_time;
    this->ble_serial = ble_serial;
  }

  uint64_t get_send_time(){
    return send_time;
  }

  std::vector<byte> get_message(){
    return message;
  }

  bool is_ble_serial(){
    return ble_serial;
  }
  
  private:
  
  std::vector<byte> message;
  uint64_t send_time;
  bool ble_serial;
};
