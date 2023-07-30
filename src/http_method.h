#pragma once
#include <string>
#include <unordered_map>

namespace http {
enum class HttpMethod {
    kGet,
    kPost,
};

static std::unordered_map<std::string, HttpMethod> kMethodNameMap = {
    {"GET", http::HttpMethod::kGet}, {"POST", http::HttpMethod::kPost}};
}  // namespace http