#pragma once
#include <netinet/in.h>
#include <sys/epoll.h>

#include <functional>
#include <string>
#include <unordered_map>

#include "event_loop.h"
#include "http_client.h"
#include "http_message.h"

#define IP_ADDR "127.0.0.5"
#define PORT 8080
#define LISTEN_BACKLOG 1024
#define BUFFER_SIZE 1024

namespace http {

class EventLoop;

using Handler = std::function<void(http::Request&, http::Response&)>;
class HttpServer {
   private:
    int tcp_sock_, port_;
    std::string ip_addr_;
    sockaddr_in sock_addr_;
    socklen_t sock_addr_len_;

    std::unordered_map<std::string, Handler> path_handler_map_;
    EventLoop& event_loop_;

    void Route(Request& request, Response& response);
    void Handle(Request& request, Response& response);

   public:
    HttpServer();
    ~HttpServer();
    void Start();
    Client* Accept(epoll_event& event);
    void AddHandler(const std::string& path, const Handler& handler);
    void ReadRequest(epoll_event& event);
    void WriteResponse(epoll_event& event);
};
}  // namespace http