#include <iostream>
#include <string>

#define DEBUG false

namespace logging {
enum class LogLevel {
    kDebug,
    kInfo,
    kWarn,
    kError,
};

class Logger {
   public:
    static void Log(const LogLevel level, const std::string &msg);
};
}  // namespace logging