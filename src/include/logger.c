#include "logger.h"
#include <stdlib.h>
#include <string.h>

FILE *log_file = NULL;

enum LogLevel log_lvl = DEBUG;

const char *logLevelNames[] = {
    "INFO", "WARN", "ERROR", "FATAL", "DEBUG",
};

void set_log_lvl(char **argv) { log_lvl = (enum LogLevel)atoi(argv[0]); }

int logger(enum LogLevel lvl, const char *fmt, ...) {
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

int logger_feat(int no_newline, int no_lvl_name, enum LogLevel lvl,
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