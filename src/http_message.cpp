#include "http_message.h"

#include <iostream>
#include <sstream>
#include <string>

#include "http_status.h"
#include "logger.h"

const std::string LINE_SEP = "\r\n";
const std::string BODY_SEP = LINE_SEP + LINE_SEP;

const char REQUEST_LINE_SEP = ' ';
const std::string DEFAULT_PROTOCOL = "HTTP/1.1";
const char REQUEST_TARGET_SEP = '?';
const char PARAMS_SEP = '&';
const char PARAM_SEP = '=';

const std::string HEADER_SEP = ": ";
const std::string CONTENT_LENGTH_HEADER = "Content-Length";

void http::Request::ParseRequest(const std::string &request) {
    size_t request_line_end_pos = request.find(LINE_SEP);
    if (request_line_end_pos == std::string::npos) {
        logging::Logger::Log(logging::LogLevel::kError,
                             "Missing request line: " + request);
        return;
    }

    size_t headers_start_pos = request_line_end_pos + LINE_SEP.size();
    size_t headers_end_pos =
        request.find(LINE_SEP + LINE_SEP, request_line_end_pos);
    std::string request_line = request.substr(0, request_line_end_pos),
                headers = request.substr(headers_start_pos,
                                         headers_end_pos - headers_start_pos);

    ParseRequestLine(request_line);
    ParseHeaders(headers);
    if (method_ == http::HttpMethod::kPost) {
        size_t body_start_pos = headers_end_pos + BODY_SEP.size();
        size_t body_length = request.size() - body_start_pos + 1;
        if (content_length_ != body_length) {
            logging::Logger::Log(logging::LogLevel::kWarn,
                                 "Body length does not match Content-Length");
        }
        std::string body = request.substr(body_start_pos, content_length_);
        ParseBody(body);
    }
}

void http::Request::ParseRequestLine(const std::string &request_line) {
    size_t method_end_pos = request_line.find(REQUEST_LINE_SEP);
    size_t request_target_end_pos =
        request_line.find(REQUEST_LINE_SEP, method_end_pos + 1);
    method_string_ = request_line.substr(0, method_end_pos);
    if (!http::kMethodNameMap.count(method_string_)) {
        logging::Logger::Log(
            logging::LogLevel::kError,
            "HTTP Method " + method_string_ + " does not exist");
        return;
    }

    method_ = kMethodNameMap[method_string_];
    request_target_ = request_line.substr(
        method_end_pos + 1, request_target_end_pos - method_end_pos - 1);
    std::string protocol =
        request_line.substr(request_target_end_pos + 1,
                            request_line.size() - request_target_end_pos + 1);
    if (protocol != DEFAULT_PROTOCOL) {
        logging::Logger::Log(logging::LogLevel::kError,
                             "Protocol " + protocol + " does not exist");
        return;
    }

    size_t params_start_pos = request_target_.find(REQUEST_TARGET_SEP);
    path_ = request_target_.substr(0, params_start_pos);
    if (params_start_pos == std::string::npos) {
        return;
    }

    std::string params = request_target_.substr(params_start_pos + 1);
    std::istringstream iss;
    iss.str(params);
    std::string param;
    while (std::getline(iss, param, PARAMS_SEP)) {
        size_t param_sep_pos = param.find(PARAM_SEP);
        std::string param_key = param.substr(0, param_sep_pos),
                    param_value = param.substr(param_sep_pos + 1);
        url_params_[param_key] = param_value;
    }
}

void http::Request::ParseHeaders(const std::string &headers) {
    std::istringstream iss;
    iss.str(headers);
    std::string header;
    while (std::getline(iss, header)) {
        size_t header_pos_sep = header.find(HEADER_SEP);
        std::string header_key = header.substr(0, header_pos_sep),
                    header_value = header.substr(header_pos_sep + 1);
        headers_[header_key] = header_value;
    }

    content_length_ = headers_.count(CONTENT_LENGTH_HEADER)
                          ? std::stoi(headers_[CONTENT_LENGTH_HEADER])
                          : 0;
}

void http::Request::ParseBody(const std::string &body) { body_ = body; }

std::ostream &http::operator<<(std::ostream &os, const http::Request &request) {
    os << "METHOD: " << request.method_string_ << '\n';
    os << "REQUEST TARGET: " << request.request_target_ << '\n';
    os << "PATH: " << request.path_ << '\n';
    os << "URL PARAMS:" << '\n';
    for (auto &[key, value] : request.url_params_) {
        os << '\t' << key << "=" << value << '\n';
    }
    os << "HEADERS:" << '\n';
    for (auto &[key, value] : request.headers_) {
        os << '\t' << key << "=" << value << '\n';
    }
    os << "BODY: " << request.body_ << '\n';
    return os;
}

std::string http::Request::get_path() { return path_; }

KVMap http::Request::get_url_params() { return url_params_; }

std::string http::Request::get_body() { return body_; }

std::string http::Response::get_response_string() {
    if (response_string_ == "") {
        std::ostringstream oss;
        oss << "HTTP/1.1"
            << " " << status_code_ << " " << status_message_ << LINE_SEP;
        for (auto &[key, value] : headers_) {
            oss << key << HEADER_SEP << value << LINE_SEP;
        }
        oss << LINE_SEP;
        oss << body_;
        response_string_ = oss.str();
    }

    return response_string_;
}

void http::Response::set_status(int status_code) {
    status_code_ = status_code;
    if (!kStatusCodeMessageMap.count(status_code)) {
        logging::Logger::Log(
            logging::LogLevel::kWarn,
            "Status code " + std::to_string(status_code) + " does not exist");
        return;
    }
    status_message_ = kStatusCodeMessageMap[status_code];
}

void http::Response::set_header(const std::string &key,
                                const std::string &value) {
    headers_[key] = value;
}

void http::Response::set_body(const std::string &body) {
    body_ = body;
    set_header(CONTENT_LENGTH_HEADER, std::to_string(body.size()));
}