#include <iostream>
#include <fstream>
#include <utility>

enum class LOG_LEVEL : unsigned char {
    DEBUG,
    FAIL,
    EXCEPTION,
    MAJOR,
    NORMAL
};

namespace log {
    namespace console_colours {
        static constexpr const char* reset = "\x1b[0m";
        static constexpr const char* red   = "\x1b[31m";
        static constexpr const char* green = "\x1b[32m";
        static constexpr const char* blue  = "\x1b[34m";
        static constexpr const char* orange = "\x1b[38;2;255;165;0m";
    }

    inline static void log_console(
        const char* msg,
        LOG_LEVEL level
    ) {
        switch (level) {
            case LOG_LEVEL::DEBUG:
            std::cout << console_colours::orange << msg << console_colours::reset << '\n';
            break;
            case LOG_LEVEL::FAIL:
            std::cout << console_colours::red << msg << console_colours::reset << '\n';
            break;
            case LOG_LEVEL::EXCEPTION:
            std::cout << console_colours::blue << msg << console_colours::reset << '\n';
            break;
            case LOG_LEVEL::MAJOR:
            std::cout << console_colours::green << msg << console_colours::reset << '\n';
            break;
            case LOG_LEVEL::NORMAL:
            std::cout << msg << '\n';
            break;
            default:
            std::unreachable();
        }
    }
}
