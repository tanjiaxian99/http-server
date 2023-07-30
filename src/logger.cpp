#include "logger.h"

void logging::Logger::Log(const LogLevel level, const std::string &msg) {
    switch (level) {
        case logging::LogLevel::kDebug:
            if (DEBUG) {
                std::cout << "[DEBUG] " << msg << '\n';
            }
            break;
        case logging::LogLevel::kInfo:
            std::cout << "[INFO] " << msg << '\n';
            break;
        case logging::LogLevel::kWarn:
            std::cout << "[WARN] " << msg << '\n';
            break;
        case logging::LogLevel::kError:
            std::cout << "[ERROR] " << msg << '\n';
            break;
    }
}
