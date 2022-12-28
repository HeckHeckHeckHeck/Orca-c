#ifndef _ORCA_VALOG_H
#define _ORCA_VALOG_H

#include "valog_internal.h"

#define ORCA_LOG_ERR(...) ORCA_LOG(ORCA_LOG_LEVEL_ERROR, __VA_ARGS__)

#define ORCA_LOG_WARN(...) ORCA_LOG(ORCA_LOG_LEVEL_WARN, __VA_ARGS__)

#define ORCA_LOG_TRACE(...) ORCA_LOG(ORCA_LOG_LEVEL_TRACE, __VA_ARGS__)

#define ORCA_LOG(level, ...)                                                                       \
    do {                                                                                           \
        _orca_log(level, __FILE__, __LINE__, ORCA_MACRO_FUNCNAME, "" __VA_ARGS__);              \
    } while (0)

#endif
