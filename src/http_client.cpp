#include "http_client.h"

int http::Client::get_fd() { return fd_; }

http::Request& http::Client::get_request() { return request_; }

http::Response& http::Client::get_response() { return response_; }

void http::Client::set_fd(const int fd) { fd_ = fd; }

void http::Client::set_ip_addr(const std::string& ip_addr) {
    ip_addr_ = ip_addr;
}