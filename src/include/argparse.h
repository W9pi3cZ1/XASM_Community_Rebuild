#include "./logger.h"
#include <stddef.h>

typedef struct Option {
    char *name;
    char *description;
    char *usage;
    size_t argc;
    const char **aliases;
    size_t alias_cnt;
    char *help; // For more help message
    void (*callback)(char **argv);
} Option;

typedef struct Application {
    char *name;
    char *description;
    char *version;
    char *usage;
    size_t opt_cnt;
    Option **opts;
    size_t argc;
    char **argv;
} Application;

int parse_arg(Application *app, size_t *arg_idx);

void parse_args(Application *app);

void show_app_help(Application *app);

void show_opt_help(Application *app, Option *opt, char *alias);