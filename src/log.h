#ifndef _ORCA_VALOG_H
#define _ORCA_VALOG_H

#define ORCA_LOG_FUNCTION_ENTRY 1

//TODO implement compile time max level/disable
#ifndef ORCA_LOG_LEVEL_MAXIMUM
    #define ORCA_LOG_LEVEL_MAXIMUM ORCA_LOG_LEVEL_INFO
#endif

//TODO auto detect via compiler
#ifdef ORCA_LOG_HAVE_PRETTY_FUNCTION
    #define ORCA_LOG_MACRO_FUNCNAME __PRETTY_FUNCTION__
#else
    #define ORCA_LOG_MACRO_FUNCNAME __func__
#endif

typedef enum {
    ORCA_LOG_BACKEND_STDOUT,
    ORCA_LOG_BACKEND_STDERR,
    ORCA_LOG_BACKEND_FILE
} ORCA_LOG_BACKEND;

typedef enum {
    ORCA_LOG_LEVEL_NONE = 0, // Logging Disabled
    ORCA_LOG_LEVEL_ERROR = 100,
    ORCA_LOG_LEVEL_WARN = 200,
    ORCA_LOG_LEVEL_INFO = 300,
    ORCA_LOG_LEVEL_ALL = 1000000 // Convenience
} ORCA_LOG_LEVEL;

typedef enum {
    ORCA_LOG_COLOR_DEFAULT,
    ORCA_LOG_COLOR_RESET,
    ORCA_LOG_COLOR_BLACK,
    ORCA_LOG_COLOR_RED,
    ORCA_LOG_COLOR_GREEN,
    ORCA_LOG_COLOR_YELLOW,
    ORCA_LOG_COLOR_BLUE,
    ORCA_LOG_COLOR_MAGENTA,
    ORCA_LOG_COLOR_CYAN,
    ORCA_LOG_COLOR_WHITE,
} ORCA_LOG_COLOR;


//---------------------------------------------------------------------------------------
// C99
//---------------------------------------------------------------------------------------
#ifndef __cplusplus
    #include <stdbool.h>

    #define ORCA_LOG_ERR(...) ORCA_LOG(ORCA_LOG_LEVEL_ERROR, __VA_ARGS__)

    #define ORCA_LOG_WARN(...) ORCA_LOG(ORCA_LOG_LEVEL_WARN, __VA_ARGS__)

    #define ORCA_LOG_INFO(...) ORCA_LOG(ORCA_LOG_LEVEL_INFO, __VA_ARGS__)

    #define ORCA_LOG(level, ...)                                                                     \
        do {                                                                                         \
            _orca_log_for_macro(level, __FILE__, __LINE__, ORCA_LOG_MACRO_FUNCNAME, "" __VA_ARGS__); \
        } while (0)

// config/settings
void orca_log_level_set(ORCA_LOG_LEVEL level);
ORCA_LOG_LEVEL orca_log_level_get(void);

void orca_log_logfile_path_set(const char* path);
void orca_log_logfile_path_get(const char** path);

void orca_log_color_set(ORCA_LOG_COLOR col);
ORCA_LOG_COLOR orca_log_color_get(void);

void orca_log_logfile_clear_on_open_set(bool clear);
bool orca_log_logfile_clear_on_open_get(void);

// logging
void orca_log(const char* msg);
void orca_log_h1(const char* msg);
void orca_log_h2(const char* msg);
void orca_log_h3(const char* msg);

// helper function for macro use only
void _orca_log_for_macro(
    ORCA_LOG_LEVEL level,
    const char* file,
    int line,
    const char* funcname,
    const char* fmt,
    ...);

#endif // NOT __cplusplus

//---------------------------------------------------------------------------------------
// C++
//---------------------------------------------------------------------------------------
#ifdef __cplusplus
    #include <string>
    #include <sstream>
    #include <filesystem>

    #define ORCA_LOG_ERR(msg) ORCA_LOG(ORCA_LOG_LEVEL_ERROR, msg)

    #define ORCA_LOG_WARN(msg) ORCA_LOG(ORCA_LOG_LEVEL_WARN, msg)

    #define ORCA_LOG_INFO(msg) ORCA_LOG(ORCA_LOG_LEVEL_INFO, msg)

    #define ORCA_LOG(level, msg)                                                                       \
        do {                                                                                           \
            std::stringstream ss{};                                                                    \
            ss << msg;                                                                                 \
            ::Orca::Log::_log_for_macro(level, __FILE__, __LINE__, ORCA_LOG_MACRO_FUNCNAME, ss.str()); \
        } while (0)

namespace Orca {
    namespace Log {
        // config/settings
        void set_level(ORCA_LOG_LEVEL level);
        ORCA_LOG_LEVEL get_level();
        void set_color(ORCA_LOG_COLOR col);
        ORCA_LOG_COLOR get_color();

        namespace Backend {
            namespace Logfile {
                void set_path(const std::filesystem::path& path);
                const std::filesystem::path& get_path();
                void set_clear_on_open(bool clear);
                bool get_clear_on_open();
            } // namespace Logfile
        }     // namespace Backend

        // logging
        void log(std::string_view msg, ORCA_LOG_COLOR col = ORCA_LOG_COLOR_DEFAULT);
        void logH1(std::string_view msg, ORCA_LOG_COLOR col = ORCA_LOG_COLOR_DEFAULT);
        void logH2(std::string_view msg, ORCA_LOG_COLOR col = ORCA_LOG_COLOR_DEFAULT);
        void logH3(std::string_view msg, ORCA_LOG_COLOR col = ORCA_LOG_COLOR_DEFAULT);

        // helper function for macro use only
        void _log_for_macro(
            ORCA_LOG_LEVEL level,
            const std::string& file,
            int line,
            const std::string& funcname,
            const std::string& msg);
    } // namespace Log
} // namespace Orca

std::ostream& operator<<(std::ostream& o, const ORCA_LOG_LEVEL* level);
#endif //__cplusplus
#endif
