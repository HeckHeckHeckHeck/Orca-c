#ifndef _ORCA_VALOG_HH
#define _ORCA_VALOG_HH

#include <string>
#include "valog_internal.h"

#define ORCA_LOG_ERR(...) ORCA_LOG(ORCA_LOG_LEVEL_ERROR, __VA_ARGS__)

#define ORCA_LOG_WARN(...) ORCA_LOG(ORCA_LOG_LEVEL_WARN, __VA_ARGS__)

#define ORCA_LOG_TRACE(...) ORCA_LOG(ORCA_LOG_LEVEL_TRACE, __VA_ARGS__)

#define ORCA_LOG(level, ...)                                                                       \
    do {                                                                                           \
        _orca_log(level, __FILE__, __LINE__, ORCA_MACRO_FUNCNAME, "" __VA_ARGS__);              \
    } while (0)


namespace Orca {
    namespace Utils {
        enum class Color {
            RESET,
            BLACK,
            RED,
            GREEN,
            YELLOW,
            BLUE,
            MAGENTA,
            CYAN,
            WHITE,
        };
        std::string to_termcol(const Color& col);
    } // namespace Utils

    namespace Log {
        void set_enabled(const bool& is_enabled);
        bool get_enabled();
        void log(const std::string& msg, Utils::Color col = Utils::Color::WHITE);
        void logH1(const std::string& msg, Utils::Color col = Utils::Color::WHITE);
        void logH2(const std::string& msg, Utils::Color col = Utils::Color::WHITE);
        void logH3(const std::string& msg, Utils::Color col = Utils::Color::WHITE);
        std::string decorate_three_lines(const std::string& msg, char decoration = '-');
        std::string decorate_centered(const std::string& msg, char decoration = '-');
    } // namespace Log
} // namespace Orca

std::ostream& operator<<(std::ostream& o, const ORCA_LOG_LEVEL* level);

#endif
