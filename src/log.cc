#include "log.h"
#include <iostream>
#include <sstream>
#include <cstdarg>
#include <unistd.h>
#include <thread>
#include <cmath>

//-------------------------------------------------------------------------------------------------
// Private functions Prototypes
//-------------------------------------------------------------------------------------------------
#ifdef __cplusplus
extern "C" {
#endif
    const char* _orca_log_level_to_string(const ORCA_LOG_LEVEL* level);
#ifdef __cplusplus
}
#endif

namespace Orca {
    namespace Utils {
        std::string _to_termcol(const ORCA_LOG_COLOR& col);
        std::string _decorate_three_lines(const std::string& msg, char decoration = '-');
        std::string _decorate_centered(const std::string& msg, char decoration = '-');
    } // namespace Utils
    namespace Log {
        std::mutex _mtx_raw_log{};
        std::mutex _mtx_log{};
        std::atomic<ORCA_LOG_LEVEL> _log_level{ ORCA_LOG_LEVEL_NONE };
        std::atomic<ORCA_LOG_COLOR> _log_color{ ORCA_LOG_COLOR_WHITE };
        namespace Backend {
            void _log_stderr(const std::string& msg, ORCA_LOG_COLOR col = ORCA_LOG_COLOR_DEFAULT);
        }
    } // namespace Log
} // namespace Orca

//-------------------------------------------------------------------------------------------------
// C linkage functions
//-------------------------------------------------------------------------------------------------
#ifdef __cplusplus
extern "C" {
#endif

    void orca_log_set_level(ORCA_LOG_LEVEL level)
    {
        Orca::Log::_log_level.store(level);
    }

    ORCA_LOG_LEVEL orca_log_get_level()
    {
        return Orca::Log::_log_level.load();
    }

    void orca_log_set_color(ORCA_LOG_COLOR col)
    {
        if (col == ORCA_LOG_COLOR_DEFAULT) {
            col = ORCA_LOG_COLOR_RESET;
        }
        Orca::Log::_log_color.store(col);
    }

    ORCA_LOG_COLOR orca_log_get_color()
    {
        return Orca::Log::_log_color.load();
    }

    void orca_log(const char* msg)
    {
        Orca::Log::Backend::_log_stderr(msg, Orca::Log::_log_color.load());
    }

    void orca_log_h1(const char* msg)
    {
        Orca::Log::Backend::_log_stderr(
            Orca::Utils::_decorate_three_lines(msg, '='),
            Orca::Log::_log_color.load());
    }

    void orca_log_h2(const char* msg)
    {
        Orca::Log::Backend::_log_stderr(
            "\n" + Orca::Utils::_decorate_centered(msg, '='),
            Orca::Log::_log_color.load());
    }

    void orca_log_h3(const char* msg)
    {
        Orca::Log::Backend::_log_stderr(Orca::Utils::_decorate_centered(msg, '-'), Orca::Log::_log_color.load());
    }


    void _orca_log_for_macro(
        ORCA_LOG_LEVEL level,
        const char* file,
        int line,
        const char* funcname,
        const char* fmt,
        ...)
    {
        std::lock_guard<std::mutex> l{ Orca::Log::_mtx_log };
        if (level > Orca::Log::_log_level) {
            return;
        }
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
            msg = header.str();
            if (!std::string(msg_fmtd).empty()) {
                msg += "- " + std::string(msg_fmtd);
            }
        }
        free(msg_fmtd);
        Orca::Log::log(msg);
        va_end(var_args);
    }

    const char* _orca_log_level_to_string(const ORCA_LOG_LEVEL* level)
    {
        switch (*level) {
            case ORCA_LOG_LEVEL_NONE:
                return "NONE";
            case ORCA_LOG_LEVEL_ERROR:
                return "ERROR";
            case ORCA_LOG_LEVEL_WARN:
                return "WARN";
            case ORCA_LOG_LEVEL_INFO:
                return "INFO";
            case ORCA_LOG_LEVEL_ALL:
                return "ALL";
            default:
                return "invalid log level";
        }
    }
#ifdef __cplusplus
}
#endif

//-------------------------------------------------------------------------------------------------
// C++ Linkage functions
//-------------------------------------------------------------------------------------------------

namespace Orca {
    namespace Log {
        // for C++ some functions are just wrappers for the
        // C linkage functions
        // to be nicely available in the namespace

        void set_level(ORCA_LOG_LEVEL level)
        {
            ::orca_log_set_level(level);
        }

        ORCA_LOG_LEVEL get_level()
        {
            return ::orca_log_get_level();
        }

        void set_color(ORCA_LOG_COLOR col)
        {
            ::orca_log_set_color(col);
        }

        ORCA_LOG_COLOR get_color()
        {
            return ::orca_log_get_color();
        }

        void _log_for_macro(
            ORCA_LOG_LEVEL level,
            const std::string& file,
            int line,
            const std::string& funcname,
            const std::string& msg)
        {
            ::_orca_log_for_macro(level, file.c_str(), line, funcname.c_str(), "%s", msg.c_str());
        }

        void log(const std::string& msg, ORCA_LOG_COLOR col)
        {
            Backend::_log_stderr(msg, col);
        }

        void logH1(const std::string& msg, ORCA_LOG_COLOR col)
        {
            Backend::_log_stderr(Utils::_decorate_three_lines(msg, '='), col);
        }

        void logH2(const std::string& msg, ORCA_LOG_COLOR col)
        {
            Backend::_log_stderr("\n" + Utils::_decorate_centered(msg, '='), col);
        }

        void logH3(const std::string& msg, ORCA_LOG_COLOR col)
        {
            Backend::_log_stderr(Utils::_decorate_centered(msg, '-'), col);
        }

        namespace Backend {
            // TODO: add file backend
            void _log_stderr(const std::string& msg, ORCA_LOG_COLOR col)
            {
                std::lock_guard<std::mutex> l{ _mtx_raw_log };
                std::cerr << Utils::_to_termcol(col) << msg
                          << Utils::_to_termcol(ORCA_LOG_COLOR_RESET)
                          << std::endl; //endl also flushes, but cerr is unbuffered anyways
            }
        }
    } // namespace Log



    namespace Utils {
        int termsize_x = 120;

        std::string _to_termcol(const ORCA_LOG_COLOR& col)
        {
            switch (col) {
                case ORCA_LOG_COLOR_DEFAULT:
                    return _to_termcol(Orca::Log::_log_color);
                case ORCA_LOG_COLOR_RESET:
                    return "\033[0m";
                case ORCA_LOG_COLOR_BLACK:
                    return "\033[30m";
                case ORCA_LOG_COLOR_RED:
                    return "\033[31m";
                case ORCA_LOG_COLOR_GREEN:
                    return "\033[32m";
                case ORCA_LOG_COLOR_YELLOW:
                    return "\033[33m";
                case ORCA_LOG_COLOR_BLUE:
                    return "\033[34m";
                case ORCA_LOG_COLOR_MAGENTA:
                    return "\033[35m";
                case ORCA_LOG_COLOR_CYAN:
                    return "\033[36m";
                case ORCA_LOG_COLOR_WHITE:
                    return "\033[37m";
                default:
                    return _to_termcol(ORCA_LOG_COLOR_RESET);
            }
        }

        std::string _decorate_three_lines(const std::string& msg, char decoration)
        {
            std::stringstream tmp;
            tmp << std::string(termsize_x, decoration) << std::endl
                << msg << std::endl
                << std::string(termsize_x, decoration);
            return tmp.str();
        }

        std::string _decorate_centered(const std::string& msg, char decoration)
        {
            std::stringstream tmp;
            size_t max_len = termsize_x - 10;
            // truncate msg
            std::string msg_truncated = msg;
            if (msg.length() >= max_len) {
                msg_truncated = msg.substr(0, max_len);
                msg_truncated += "...";
            }

            // define decolen
            int decolen = static_cast<int>(floor((double(termsize_x - msg_truncated.length()))) / 2.0);

            tmp << std::string(decolen, decoration) << ' ' << msg_truncated << ' '
                << std::string(decolen, decoration);
            return tmp.str();
        }
    } // namespace Utils
} // namespace Orca

//-------------------------------------------------------------------------------------------------
// C++ Linkage global namespace members
//-------------------------------------------------------------------------------------------------
std::ostream& operator<<(std::ostream& o, const ORCA_LOG_LEVEL* level)
{
    return o << std::string(_orca_log_level_to_string(level));
}

// Stuff like that would be fucking handy
// TODO: make a lib with operator<< for ALL libstc++ types
template<typename T> std::ostream& operator<<(std::ostream& o, std::vector<T>& vec)
{
    std::stringstream ss{};
    for (const T& elem : vec) {
        o << elem;
    }
    return o;
}
