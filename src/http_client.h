#include <string>

#include "http_message.h"

namespace http {
class Client {
   private:
    int fd_;
    std::string ip_addr_;
    Request request_;
    Response response_;

   public:
    int get_fd();
    Request& get_request();
    Response& get_response();

    void set_fd(const int fd);
    void set_ip_addr(const std::string& ip_addr);
};

}  // namespace http