#ifndef _ORCA_VALOG_INTERNAL_H
#define _ORCA_VALOG_INTERNAL_H

#define ORCA_HAVE_PRETTY_FUNCTION 1
#ifdef ORCA_HAVE_PRETTY_FUNCTION
    #define ORCA_MACRO_FUNCNAME __PRETTY_FUNCTION__
#else
    #define ORCA_func_OR_PRETTY_FUNCTION __func__
#endif

typedef enum {
    ORCA_LOG_LEVEL_ERROR = 100,
    ORCA_LOG_LEVEL_WARNING = 200,
    ORCA_LOG_LEVEL_TRACE = 300,
} ORCA_LOG_LEVEL;


#ifdef __cplusplus
extern "C" {
#endif

    void _orca_log(
        ORCA_LOG_LEVEL level,
        const char* file,
        int line,
        const char* funcname,
        const char* fmt,
        ...);

#ifdef __cplusplus
}
#endif


#endif
