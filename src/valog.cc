#include "valog.h"
#include "valog.hh"
#include <iostream>
#include <sstream>
#include <cstdarg>
#include <unistd.h>
#include <thread>
#include <cmath>

#ifdef __cplusplus
extern "C" {
#endif

    const char* _orca_log_level_to_string(const ORCA_LOG_LEVEL* level)
    {
        switch (*level) {
            case ORCA_LOG_LEVEL_ERROR:
                return "ERROR";
            case ORCA_LOG_LEVEL_WARNING:
                return "WARN";
            case ORCA_LOG_LEVEL_TRACE:
                return "TRACE";
            default:
                return "invalid log level";
        }
    }

    void _orca_log(
        ORCA_LOG_LEVEL level,
        const char* file,
        int line,
        const char* funcname,
        const char* fmt,
        ...)
    {
        std::string msg{ "orca log subsystem error" };
        va_list var_args;
        va_start(var_args, fmt);

        char* msg_fmtd = nullptr;
        int size = vasprintf(&msg_fmtd, fmt, var_args);
        if (size >= 0) {
            std::stringstream header{};
            header << _orca_log_level_to_string(&level) << " ";
            header << getpid() << " ";
            header << std::this_thread::get_id() << " ";
            header << file << ":";
            header << line << " ";
            header << funcname << " ";
            msg = header.str() + " - " + std::string(msg_fmtd);
        }
        free(msg_fmtd);
        Orca::Log::log(msg);
        va_end(var_args);
    }

#ifdef __cplusplus
}
#endif

std::ostream& operator<<(std::ostream& o, const ORCA_LOG_LEVEL* level)
{
    return o << std::string(_orca_log_level_to_string(level));
}

namespace Orca {
    namespace Utils {
        std::string to_termcol(const Color& col)
        {
            switch (col) {
                case Color::RESET:
                    return "\033[0m";
                case Color::BLACK:
                    return "\033[30m";
                case Color::RED:
                    return "\033[31m";
                case Color::GREEN:
                    return "\033[32m";
                case Color::YELLOW:
                    return "\033[33m";
                case Color::BLUE:
                    return "\033[34m";
                case Color::MAGENTA:
                    return "\033[35m";
                case Color::CYAN:
                    return "\033[36m";
                case Color::WHITE:
                    return "\033[37m";
                default:
                    return "\033[0m";
            }
        }
    } // namespace Utils
} // namespace Orca

namespace Orca {
    namespace Log {
        int line_width = 120;

        // NON CLASS
        std::mutex mtx;
        std::atomic_bool is_enabled{ false };

        void set_enabled(const bool& enabled)
        {
            is_enabled.store(enabled);
        }

        bool get_enabled()
        {
            return is_enabled.load();
        }

        // Common "print" function implementing the actual "backends"
        void _log(const std::string& msg, Utils::Color col = Utils::Color::WHITE)
        {
            std::lock_guard<std::mutex> l(mtx);
            std::cerr << Utils::to_termcol(col) << msg << Utils::to_termcol(Utils::Color::RESET)
                      << std::endl; //endl also flushes, but cerr is unbuffered anyways
        }

        void log(const std::string& msg, Utils::Color col)
        {
            _log(msg, col);
        }

        void logH1(const std::string& msg, Utils::Color col)
        {
            log(decorate_three_lines(msg, '='), col);
        }

        void logH2(const std::string& msg, Utils::Color col)
        {
            log("\n" + decorate_centered(msg, '='), col);
        }

        void logH3(const std::string& msg, Utils::Color col)
        {
            log(decorate_centered(msg, '-'), col);
        }

        std::string decorate_three_lines(const std::string& msg, char decoration)
        {
            std::stringstream tmp;
            tmp << std::string(line_width, decoration) << std::endl
                << msg << std::endl
                << std::string(line_width, decoration);
            return tmp.str();
        }

        std::string decorate_centered(const std::string& msg, char decoration)
        {
            std::stringstream tmp;
            size_t max_len = line_width - 10;
            // truncate msg
            std::string msg_truncated = msg;
            if (msg.length() >= max_len) {
                msg_truncated = msg.substr(0, max_len);
                msg_truncated += "...";
            }

            // define decolen
            int decolen = static_cast<int>(floor((double(line_width - msg_truncated.length()))) / 2.0);

            tmp << std::string(decolen, decoration) << ' ' << msg_truncated << ' '
                << std::string(decolen, decoration);
            return tmp.str();
        }
    } // namespace Log
} // namespace Orca
