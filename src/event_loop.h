#pragma once
#include "http_server.h"

#define MAX_EVENTS 11000

namespace http {
class HttpServer;

class EventLoop {
   private:
    int epoll_fd_;
    int tcp_sock_;
    HttpServer *server_;

   public:
    static EventLoop &get_instance() {
        static EventLoop instance;
        return instance;
    }
    EventLoop();
    ~EventLoop();
    EventLoop(const EventLoop &) = delete;
    EventLoop &operator=(const EventLoop &) = delete;
    void Start();

    HttpServer *get_server();
    void set_tcp_sock(const int tcp_sock);
    void set_non_blocking(const int fd);
    void set_server(HttpServer *server);
};
}  // namespace http