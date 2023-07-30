#include "http_server.h"

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#include <iostream>
#include <sstream>
#include <string>

#include "logger.h"

namespace {
void Log(const std::string &msg) { std::cout << msg << '\n'; }
void ExitWithError(const std::string &msg) {
    Log("ERROR: " + msg);
    exit(1);
}
}  // namespace

http::HttpServer::HttpServer() : event_loop_(EventLoop::get_instance()) {
    ip_addr_ = IP_ADDR;
    port_ = PORT;

    sock_addr_.sin_family = AF_INET;
    sock_addr_.sin_port = htons(port_);
    sock_addr_.sin_addr.s_addr = htonl(INADDR_ANY);
    sock_addr_len_ = sizeof(sock_addr_);

    tcp_sock_ = socket(AF_INET, SOCK_STREAM, 0);
    if (tcp_sock_ < 0) {
        logging::Logger::Log(logging::LogLevel::kError, "Cannot create socket");
        exit(1);
    }

    if (bind(tcp_sock_, (sockaddr *)&sock_addr_, sock_addr_len_) < 0) {
        logging::Logger::Log(logging::LogLevel::kError,
                             "Cannot bind socket to address");
        exit(1);
        return;
    }

    if (listen(tcp_sock_, LISTEN_BACKLOG) < 0) {
        logging::Logger::Log(logging::LogLevel::kError,
                             "Fail to listen on socket");
        exit(1);
    }

    std::ostringstream oss;
    oss << "Listening on ADDRESS: " << inet_ntoa(sock_addr_.sin_addr)
        << ", PORT: " << ntohs(sock_addr_.sin_port);
    logging::Logger::Log(logging::LogLevel::kInfo, oss.str());

    event_loop_.set_tcp_sock(tcp_sock_);
    event_loop_.set_server(this);
}

http::HttpServer::~HttpServer() {
    close(tcp_sock_);
    exit(0);
}

void http::HttpServer::Start() {
    logging::Logger::Log(logging::LogLevel::kInfo,
                         "Waiting for new connection");
    event_loop_.Start();
}

http::Client *http::HttpServer::Accept(epoll_event &event) {
    int fd = event.data.fd;
    sockaddr_in sock_addr;
    socklen_t sock_addr_len = sizeof(sock_addr);
    int connect_sock_ = accept(fd, (sockaddr *)&sock_addr, &sock_addr_len);
    if (connect_sock_ < 0) {
        std::ostringstream oss;
        oss << "Fail to accept connection from ADDRESS: "
            << inet_ntoa(sock_addr_.sin_addr)
            << ", PORT: " << ntohs(sock_addr_.sin_port);
        logging::Logger::Log(logging::LogLevel::kError, oss.str());
        exit(1);
    }

    std::ostringstream oss;
    oss << "*** Accepted connection from ADDRESS: "
        << inet_ntoa(sock_addr_.sin_addr)
        << ", PORT: " << ntohs(sock_addr_.sin_port);
    logging::Logger::Log(logging::LogLevel::kDebug, oss.str());

    Client *client = new Client();
    client->set_fd(connect_sock_);
    client->set_ip_addr(inet_ntoa(sock_addr_.sin_addr));
    return client;
}

void http::HttpServer::Route(Request &request, Response &response) {
    if (!path_handler_map_.count(request.get_path())) {
        response.set_status(404);
        return;
    }

    try {
        Handle(request, response);
        response.set_status(200);
    } catch (...) {
        response.set_status(500);
    }
}

void http::HttpServer::AddHandler(const std::string &path,
                                  const Handler &handler) {
    path_handler_map_[path] = handler;
}

void http::HttpServer::Handle(Request &request, Response &response) {
    path_handler_map_[request.get_path()](request, response);
}

void http::HttpServer::ReadRequest(epoll_event &event) {
    Client *client = (Client *)event.data.ptr;

    char buffer[BUFFER_SIZE];
    read(client->get_fd(), buffer, BUFFER_SIZE);
    logging::Logger::Log(logging::LogLevel::kDebug, "Received request from client");

    Request &request = client->get_request();
    Response &response = client->get_response();
    request.ParseRequest(buffer);

    std::ostringstream oss;
    oss << request;
    logging::Logger::Log(logging::LogLevel::kDebug, oss.str());

    Route(request, response);
}

void http::HttpServer::WriteResponse(epoll_event &event) {
    Client *client = (Client *)event.data.ptr;
    Response &response = client->get_response();
    std::string response_string = response.get_response_string();
    send(client->get_fd(), response_string.c_str(), response_string.size(), 0);
}