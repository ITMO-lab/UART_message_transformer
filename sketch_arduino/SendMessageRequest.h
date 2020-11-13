#include <vector>

/*
 * Priority queue item - message to sent.
 */
class SendMessageRequest{
  public:

  SendMessageRequest(std::vector<byte> message, uint64_t send_time, bool software_serial){
    for (int i = 0; i < message.size(); i++){
      this->message = message;
    }
    this->send_time = send_time;
    this->software_serial = software_serial;
  }

  uint64_t get_send_time(){
    return send_time;
  }

  std::vector<byte> get_message(){
    return message;
  }

  bool is_software_serial(){
    return software_serial;
  }
  
  private:
  
  std::vector<byte> message;
  uint64_t send_time;
  bool software_serial;
};
