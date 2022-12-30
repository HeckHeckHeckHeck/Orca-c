#include "log.h"
#include <iostream>
#include <sstream>
#include <cstdarg>
#include <unistd.h>
#include <thread>
#include <string_view>
#include <cmath>
#include <filesystem>
#include <fstream>

//-------------------------------------------------------------------------------------------------
// Internal functions Prototypes
// Global State
//-------------------------------------------------------------------------------------------------
#ifdef __cplusplus
extern "C" {
#endif
    const char* _orca_log_level_to_string(const ORCA_LOG_LEVEL* level);
    bool _orca_logfile_open(void);
    void _orca_logfile_close(void);
#ifdef __cplusplus
}
#endif

namespace Orca {
    namespace Utils {
        std::atomic<int> termsize_x = 120;
        std::string _to_termcol(const ORCA_LOG_COLOR& col);
        std::string _decorate_three_lines(std::string_view msg, char decoration = '-');
        std::string _decorate_centered(std::string_view msg, char decoration = '-');
    } // namespace Utils
    namespace Log {
        std::atomic<ORCA_LOG_LEVEL> _log_level{ ORCA_LOG_LEVEL_NONE };
        std::atomic<ORCA_LOG_COLOR> _log_color{ ORCA_LOG_COLOR_WHITE };
        namespace Backend {
            void _dispatcher(std::string_view msg, ORCA_LOG_COLOR col = ORCA_LOG_COLOR_DEFAULT);
            namespace Console {
                std::mutex _mtx_stdout{};
                void _write_stdout(std::string_view msg, ORCA_LOG_COLOR col = ORCA_LOG_COLOR_DEFAULT);
                std::mutex _mtx_stderr{};
                void _write_stderr(std::string_view msg, ORCA_LOG_COLOR col = ORCA_LOG_COLOR_DEFAULT);
            } // namespace Console
            namespace Logfile {
                std::recursive_mutex _mtx_path{};
                std::filesystem::path _path{ "orca.log" };
                std::recursive_mutex _mtx_stream{};
                std::ofstream _stream{};
                std::atomic<bool> _clear_on_open{ true };
                void _write(std::string_view msg);
            } // namespace Logfile
        }     // namespace Backend
    }         // namespace Log
} // namespace Orca

//-------------------------------------------------------------------------------------------------
// C linkage functions
//-------------------------------------------------------------------------------------------------
#ifdef __cplusplus
extern "C" {
#endif
    void orca_log_level_set(ORCA_LOG_LEVEL level)
    {
        Orca::Log::_log_level = level;
    }

    ORCA_LOG_LEVEL orca_log_level_get(void)
    {
        return Orca::Log::_log_level;
    }

    void orca_log_logfile_path_set(const char* path)
    {
        std::lock_guard<std::recursive_mutex> l{ Orca::Log::Backend::Logfile::_mtx_path };
        std::lock_guard<std::recursive_mutex> l2{ Orca::Log::Backend::Logfile::_mtx_stream };
        Orca::Log::Backend::Logfile::_path = path;
        if (Orca::Log::Backend::Logfile::_stream.is_open()) {
            Orca::Log::Backend::Logfile::_stream.close();
        }
    }

    void orca_log_logfile_path_get(const char** path)
    {
        std::lock_guard<std::recursive_mutex> l{ Orca::Log::Backend::Logfile::_mtx_path };
        *path = Orca::Log::Backend::Logfile::_path.c_str();
    }

    void orca_log_color_set(ORCA_LOG_COLOR col)
    {
        if (col == ORCA_LOG_COLOR_DEFAULT) {
            col = ORCA_LOG_COLOR_RESET;
        }
        Orca::Log::_log_color = col;
    }

    ORCA_LOG_COLOR orca_log_color_get(void)
    {
        return Orca::Log::_log_color;
    }

    void orca_log_logfile_clear_on_open_set(bool clear)
    {
        Orca::Log::Backend::Logfile::_clear_on_open = clear;
    }

    bool orca_log_logfile_clear_on_open_get(void)
    {
        return Orca::Log::Backend::Logfile::_clear_on_open;
    }

    void orca_log(const char* msg)
    {
        Orca::Log::Backend::_dispatcher(msg, Orca::Log::_log_color);
    }

    void orca_log_h1(const char* msg)
    {
        Orca::Log::Backend::_dispatcher(
            Orca::Utils::_decorate_three_lines(msg, '='),
            Orca::Log::_log_color);
    }

    void orca_log_h2(const char* msg)
    {
        Orca::Log::Backend::_dispatcher(
            "\n" + Orca::Utils::_decorate_centered(msg, '='),
            Orca::Log::_log_color);
    }

    void orca_log_h3(const char* msg)
    {
        Orca::Log::Backend::_dispatcher(
            Orca::Utils::_decorate_centered(msg, '-'),
            Orca::Log::_log_color);
    }

    void _orca_log_for_macro(
        ORCA_LOG_LEVEL level,
        const char* file,
        int line,
        const char* funcname,
        const char* fmt,
        ...)
    {
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
            std::string_view msg_fmtd_cxx{ msg_fmtd };
            if (!msg_fmtd_cxx.empty()) {
                msg += "- ";
                msg += msg_fmtd_cxx;
            }
        }
        free(msg_fmtd);
        Orca::Log::Backend::_dispatcher(msg);
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

    bool _orca_logfile_open(void)
    {
        std::lock_guard<std::recursive_mutex> l{ Orca::Log::Backend::Logfile::_mtx_stream };
        std::lock_guard<std::recursive_mutex> l2{ Orca::Log::Backend::Logfile::_mtx_path };

        if (Orca::Log::Backend::Logfile::_stream.is_open()) {
            return true;
        }

        // Append or clear
        unsigned int openmode = std::ios::app;
        if (Orca::Log::Backend::Logfile::_clear_on_open) {
            openmode = std::ios::out | std::ios::trunc;
        }

        Orca::Log::Backend::Logfile::_stream.open(Orca::Log::Backend::Logfile::_path, openmode);
        if (Orca::Log::Backend::Logfile::_stream.fail()) {
            // TODO: Where to log error
            std::cerr << "LOGFILE ERROR" << std::endl;
            Orca::Log::Backend::Logfile::_stream.clear();
            return false;
        }
        return true;
    }

    void _orca_logfile_close(void)
    {
        std::lock_guard<std::recursive_mutex> l{ Orca::Log::Backend::Logfile::_mtx_stream };
        Orca::Log::Backend::Logfile::_stream.close();
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
        // C linkage functions are mutexed and therefore
        // wrapped here even if otherwise trivial

        void set_level(ORCA_LOG_LEVEL level)
        {
            ::orca_log_level_set(level);
        }

        ORCA_LOG_LEVEL get_level()
        {
            return ::orca_log_level_get();
        }

        void set_color(ORCA_LOG_COLOR col)
        {
            ::orca_log_color_set(col);
        }

        ORCA_LOG_COLOR get_color()
        {
            return ::orca_log_color_get();
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

        void log(std::string_view msg, ORCA_LOG_COLOR col)
        {
            Backend::_dispatcher(msg, col);
        }

        void logH1(std::string_view msg, ORCA_LOG_COLOR col)
        {
            Backend::_dispatcher(Utils::_decorate_three_lines(msg, '='), col);
        }

        void logH2(std::string_view msg, ORCA_LOG_COLOR col)
        {
            Backend::_dispatcher("\n" + Utils::_decorate_centered(msg, '='), col);
        }

        void logH3(std::string_view msg, ORCA_LOG_COLOR col)
        {
            Backend::_dispatcher(Utils::_decorate_centered(msg, '-'), col);
        }

        namespace Backend {
            void _dispatcher(std::string_view msg, ORCA_LOG_COLOR col)
            {
                Console::_write_stderr(msg, col);
                Logfile::_write(msg);
            }

            namespace Console {
                void _write_stdout(std::string_view msg, ORCA_LOG_COLOR col)
                {
                    std::lock_guard<std::mutex> l{ _mtx_stdout };
                    std::cerr << Utils::_to_termcol(col) << msg
                              << Utils::_to_termcol(ORCA_LOG_COLOR_RESET)
                              << std::endl; //endl also flushes, but cerr is unbuffered anyways
                }
                void _write_stderr(std::string_view msg, ORCA_LOG_COLOR col)
                {
                    std::lock_guard<std::mutex> l{ _mtx_stderr };
                    std::cerr << Utils::_to_termcol(col) << msg
                              << Utils::_to_termcol(ORCA_LOG_COLOR_RESET)
                              << std::endl; //endl also flushes, but cerr is unbuffered anyways
                }
            } // namespace Console

            namespace Logfile {
                void _write(std::string_view msg)
                {
                    std::lock_guard<std::recursive_mutex> l{ _mtx_stream };
                    if (!_orca_logfile_open()) {
                        std::cerr << "ERROR: Logfile open failed" << std::endl;
                    } else {
                        _stream << msg << std::endl;
                        if (_stream.fail()) {
                            std::cerr << "ERROR: Logfile Write failed" << std::endl;
                            _stream.clear();
                        }
                    }
                }

                void set_path(const std::filesystem::path& path)
                {
                    ::orca_log_logfile_path_set(path.c_str());
                }

                const std::filesystem::path& get_path()
                {
                    std::lock_guard<std::recursive_mutex> l{ Orca::Log::Backend::Logfile::_mtx_path };
                    return Orca::Log::Backend::Logfile::_path;
                }

                void set_clear_on_open(bool clear)
                {
                    ::orca_log_logfile_clear_on_open_set(clear);
                }

                bool get_clear_on_open()
                {
                    return ::orca_log_logfile_clear_on_open_get();
                }

            } // namespace Logfile
        }     // namespace Backend
    }         // namespace Log


    namespace Utils {
        std::string _to_termcol(const ORCA_LOG_COLOR& col)
        {
            switch (col) {
                case ORCA_LOG_COLOR_DEFAULT:
                    // Caution: Make sure Orca::Log::_log_color can
                    // NEVER be set to ORCA_LOG_COLOR_DEFAULT
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

        std::string _decorate_three_lines(std::string_view msg, char decoration)
        {
            std::stringstream tmp;
            tmp << std::string(termsize_x, decoration) << std::endl
                << msg << std::endl
                << std::string(termsize_x, decoration);
            return tmp.str();
        }

        std::string _decorate_centered(std::string_view msg, char decoration)
        {
            std::stringstream tmp;
            size_t max_len = termsize_x - 10;
            // truncate msg
            std::string msg_truncated{ msg };
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
    return o << std::string_view(_orca_log_level_to_string(level));
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
