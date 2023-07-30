#include <iostream>
#include <sstream>

#include "http_server.h"

using namespace http;

void hello_handler(http::Request& request, http::Response& response) {
    response.set_body("Hello World!");
}

void echo_handler(http::Request& request, http::Response& response) {
    response.set_body(request.get_body());
}

void params_handler(http::Request& request, http::Response& response) {
    std::ostringstream oss;
    for (auto& [key, value] : request.get_url_params()) {
        oss << key << ": " << value << '\n';
    }
    response.set_body(oss.str());
}

int main() {
    HttpServer server = http::HttpServer();
    server.AddHandler("/hello", hello_handler);
    server.AddHandler("/echo", echo_handler);
    server.AddHandler("/params", params_handler);

    server.Start();
    return 0;
}