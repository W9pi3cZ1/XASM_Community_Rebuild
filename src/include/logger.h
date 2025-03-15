#include <stdarg.h> // for va_list, va_start, va_end
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef _LOGGER_H
#define _LOGGER_H

typedef enum LogLevel {
    INFO = 0,
    WARN,
    ERROR,
    FATAL,
    DEBUG = 4,
} LogLevel;

static FILE *log_file = NULL;

static enum LogLevel log_lvl = DEBUG;

static void set_log_lvl(char **argv) { log_lvl = (enum LogLevel)atoi(argv[0]); }

static const char *logLevelNames[] = {
    "INFO", "WARN", "ERROR", "FATAL", "DEBUG",
};

static int logger(enum LogLevel lvl, const char *fmt, ...) {
    if (log_file == NULL) {
        log_file = stderr;
    }
    int res;
    if (log_lvl < lvl)
        return 0;
    fprintf(log_file, "[%s] ", logLevelNames[lvl]);
    fflush(log_file);
    va_list args;
    va_start(args, fmt);
    res = vfprintf(log_file, fmt, args);
    fflush(log_file);
    va_end(args);
    fprintf(log_file, "\n");
    fflush(log_file);
    return res;
}

static int logger_feat(int no_newline, int no_lvl_name, enum LogLevel lvl,
                       const char *fmt, ...) {
    if (log_file == NULL) {
        log_file = stderr;
    }
    int res = 0;
    if (log_lvl < lvl)
        return 0;
    if (!no_lvl_name) {
        fprintf(log_file, "[%s] ", logLevelNames[lvl]);
        fflush(log_file);
    }
    va_list args;
    va_start(args, fmt);
    res = vfprintf(log_file, fmt, args);
    fflush(log_file);
    va_end(args);
    if (!no_newline) {
        fprintf(log_file, "\n");
        fflush(log_file);
    }
    return res;
}

#define LOG_DEBUG(fmt, ...) logger(DEBUG, fmt, __VA_ARGS__)
#define LOG_WARN(fmt, ...) logger(WARN, fmt, __VA_ARGS__)
#define LOG_ERROR(fmt, ...) logger(ERROR, fmt, __VA_ARGS__)
#define LOG_FATAL(fmt, ...) logger(FATAL, fmt, __VA_ARGS__)
#define LOG_INFO(fmt, ...) logger(INFO, fmt, __VA_ARGS__)
#endif // _LOGGER_H