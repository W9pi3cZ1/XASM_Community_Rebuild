#include <stdarg.h> // for va_list, va_start, va_end
#include <stdio.h>

#ifndef _LOGGER_H
#define _LOGGER_H

typedef enum LogLevel {
  INFO = 0,
  WARN,
  ERROR,
  FATAL,
  DEBUG = 4,
} LogLevel;

extern FILE *log_file;

extern enum LogLevel log_lvl;

void set_log_lvl(char **argv);

extern const char *logLevelNames[];

int logger(enum LogLevel lvl, const char *fmt, ...);
int logger_feat(int no_newline, int no_lvl_name, enum LogLevel lvl,
                const char *fmt, ...);

#define LOG_DEBUG(fmt, ...) logger(DEBUG, fmt, __VA_ARGS__)
#define LOG_WARN(fmt, ...) logger(WARN, fmt, __VA_ARGS__)
#define LOG_ERROR(fmt, ...) logger(ERROR, fmt, __VA_ARGS__)
#define LOG_FATAL(fmt, ...) logger(FATAL, fmt, __VA_ARGS__)
#define LOG_INFO(fmt, ...) logger(INFO, fmt, __VA_ARGS__)
#endif // _LOGGER_H
