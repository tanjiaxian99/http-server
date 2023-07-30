#pragma once
#include <string>
#include <unordered_map>

#include "http_method.h"

using KVMap = std::unordered_map<std::string, std::string>;

namespace http {

class Request {
    friend std::ostream &operator<<(std::ostream &os,
                                    const http::Request &request);

   private:
    std::string method_string_;
    HttpMethod method_;
    std::string request_target_;
    std::string path_;
    KVMap url_params_;
    KVMap headers_;
    int content_length_;
    std::string body_;

    void ParseRequestLine(const std::string &request_line);
    void ParseHeaders(const std::string &request_headers);
    void ParseBody(const std::string &request_headers);

   public:
    void ParseRequest(const std::string &request);
    std::string get_path();
    KVMap get_url_params();
    std::string get_body();
};

std::ostream &operator<<(std::ostream &os, const Request &request);

class Response {
   private:
    int status_code_;
    std::string status_message_;
    KVMap headers_;
    std::string body_;
    std::string response_string_;

   public:
    std::string get_response_string();
    void set_status(int status_code);
    void set_header(const std::string &key, const std::string &value);
    void set_body(const std::string &body);
};

}  // namespace http