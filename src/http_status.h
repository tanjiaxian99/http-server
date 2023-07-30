#include <string>
#include <unordered_map>

namespace http {
static std::unordered_map<int, std::string> kStatusCodeMessageMap = {
    {200, "OK"}, {404, "Not Found"}, {500, "Internal Server Error"}};
}