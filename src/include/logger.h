#include <stdarg.h> // for va_list, va_start, va_end
#include <stdio.h>

#ifndef _LOGGER_H
#define _LOGGER_H

enum LogLevel {
    INFO = 0,
    WARN,
    ERROR,
    FATAL,
    DEBUG,
};

static const char *logLevelNames[] = {"INFO", "WARN", "ERROR", "FATAL",
                                      "DEBUG"};

static inline int logger(enum LogLevel lvl, char *fmt, ...) {
    int res;
    fprintf(stderr, "[%s] ", logLevelNames[lvl]);
    va_list args;
    va_start(args, fmt);
    res = vfprintf(stderr, fmt, args);
    va_end(args);
    fprintf(stderr, "\n");
    return res;
}

#define LOG_DEBUG(fmt, ...) logger(DEBUG, fmt, __VA_ARGS__)
#define LOG_WARN(fmt, ...) logger(WARN, fmt, __VA_ARGS__)
#define LOG_ERROR(fmt, ...) logger(ERROR, fmt, __VA_ARGS__)
#define LOG_FATAL(fmt, ...) logger(FATAL, fmt, __VA_ARGS__)
#define LOG_INFO(fmt, ...) logger(INFO, fmt, __VA_ARGS__)
#endif // _LOGGER_H