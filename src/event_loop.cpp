#include "event_loop.h"

#include <sys/epoll.h>
#include <sys/fcntl.h>
#include <unistd.h>

#include "logger.h"

http::EventLoop::EventLoop() {
    epoll_fd_ = epoll_create1(0);
    if (epoll_fd_ == -1) {
        logging::Logger::Log(logging::LogLevel::kError,
                             "Fail to create epoll loop");
        exit(1);
    }
}

// Reference: https://man7.org/linux/man-pages/man7/epoll.7.html
void http::EventLoop::Start() {
    epoll_event events[MAX_EVENTS];
    while (true) {
        int nfds = epoll_wait(epoll_fd_, events, MAX_EVENTS, -1);
        if (nfds == -1) {
            logging::Logger::Log(logging::LogLevel::kError, "epoll_wait");
            return;
        }

        for (int i = 0; i < nfds; ++i) {
            epoll_event &event = events[i];
            if (event.data.fd == tcp_sock_) {
                Client *client = server_->Accept(event);
                set_non_blocking(client->get_fd());

                epoll_event new_event;
                new_event.events = EPOLLIN | EPOLLET;
                new_event.data.ptr = client;
                int fd = client->get_fd();
                if (epoll_ctl(epoll_fd_, EPOLL_CTL_ADD, fd, &new_event) == -1) {
                    logging::Logger::Log(
                        logging::LogLevel::kError,
                        "Unable to add fd " + std::to_string(fd) + " to epoll");
                }
            } else if (event.events & EPOLLIN) {
                server_->ReadRequest(event);
                epoll_event new_event;
                new_event.events = EPOLLOUT | EPOLLET;
                Client *client = (Client *)event.data.ptr;
                int fd = client->get_fd();

                if (epoll_ctl(epoll_fd_, EPOLL_CTL_MOD, fd, &new_event) == -1) {
                    logging::Logger::Log(logging::LogLevel::kError,
                                         "Unable to change fd " +
                                             std::to_string(fd) +
                                             " to EPOLLOUT");
                }
            } else if (event.events & EPOLLOUT) {
                server_->WriteResponse(event);

                Client *client = (Client *)event.data.ptr;
                int fd = client->get_fd();
                epoll_ctl(epoll_fd_, EPOLL_CTL_DEL, fd, NULL);
                close(fd);
            } else {
                logging::Logger::Log(logging::LogLevel::kError,
                                     "Unknown epoll event");
            }
        }
    }
}

void http::EventLoop::set_tcp_sock(const int tcp_sock) {
    set_non_blocking(tcp_sock);

    epoll_event event;
    event.events = EPOLLIN | EPOLLET;
    event.data.fd = tcp_sock;
    if (epoll_ctl(epoll_fd_, EPOLL_CTL_ADD, tcp_sock, &event) == -1) {
        logging::Logger::Log(logging::LogLevel::kError,
                             "Unable to add TCP server socket " +
                                 std::to_string(epoll_fd_) + " to epoll");
        return;
    }
    tcp_sock_ = tcp_sock;
}

void http::EventLoop::set_non_blocking(const int fd) {
    fcntl(fd, F_SETFL, fcntl(fd, F_GETFL, 0) | O_NONBLOCK);
}

void http::EventLoop::set_server(HttpServer *server) { server_ = server; }

http::EventLoop::~EventLoop() { free(server_); }