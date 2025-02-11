#include "argparse.h"
#include "streq.c"
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void show_opt_help(Application *app, Option *opt,
                   char *alias) { // Display a help message for options
    printf("%s\n\nUsage: %s %s %s\n\nAlias(es):\n", opt->description,
           app->argv[0], alias, opt->usage);

    for (size_t alias_idx = 0; alias_idx < opt->alias_cnt; alias_idx++) {
        printf("  %s\n", opt->aliases[alias_idx]);
    }

    if (opt->help != NULL) {
        printf("\nHelp:\n%s\n", opt->help);
    }
}

int parse_arg(Application *app, size_t *arg_idx) {
    char *arg = app->argv[*arg_idx];
    int arg_valid = 0;
    char **opt_argv;
    for (size_t opt_idx = 0; opt_idx < app->opt_cnt && !arg_valid; opt_idx++) {
        Option *opt = app->opts[opt_idx];
        for (size_t alias_idx = 0; alias_idx < opt->alias_cnt && !arg_valid;
             alias_idx++) {

            if (str_eq(arg, opt->aliases[alias_idx])) {
                LOG_INFO("Found option: %s", opt->name);
                arg_valid = 1;

                if (opt->argc == 0) {
                    opt->callback(NULL);
                } else if (*arg_idx + 1 < app->argc &&
                           str_eq(app->argv[*arg_idx + 1], "--h")) {
                    show_opt_help(app, opt, app->argv[*arg_idx]);
                    *arg_idx += 1;
                    exit(0);
                } else if (*arg_idx + opt->argc < app->argc) {
                    opt_argv = malloc(sizeof(char *) * opt->argc);
                    for (size_t opt_arg_idx = 0; opt_arg_idx < opt->argc;
                         opt_arg_idx++) {
                        opt_argv[opt_arg_idx] = app->argv[ *arg_idx += 1];
                    }
                    LOG_INFO("Calling callback for option `%s`", opt->name);
                    opt->callback(opt_argv);
                } else {
                    LOG_ERROR("Missing %d argument(s) for option `%s`",
                              opt->argc - app->argc + *arg_idx + 1, opt->name);
                    exit(1);
                }
            }
        }
    }

    return arg_valid;
}

void parse_args(Application *app) {
    if (app->argc < 2) {
        show_app_help(app);
        exit(0);
    }
    size_t *arg_idx = malloc(sizeof(size_t));
    *arg_idx = 1;
    for (; *arg_idx < app->argc; (*arg_idx)++) {
        LOG_INFO("argv[%d]: %s", *arg_idx, app->argv[*arg_idx]);
        if (!parse_arg(app, arg_idx)) {
            LOG_ERROR("Invalid option `%s`", app->argv[*arg_idx]);
            exit(2);
        }
        LOG_INFO("Parse progress: %d/%d", *arg_idx, app->argc - 1);
    }
    LOG_INFO("Parse finished.", NULL);
}

void show_app_help(Application *app) {
    printf("%s  (%s)\n%s\n\n%s %s\n\nOptions:\n", app->name, app->version,
           app->description, app->argv[0], app->usage);

    // Print Options
    for (size_t opt_idx = 0; opt_idx < app->opt_cnt; opt_idx++) {
        Option *opt = app->opts[opt_idx];
        printf("  ");

        // Print aliases
        for (size_t alias_idx = 0; alias_idx < opt->alias_cnt; alias_idx++) {
            printf("%s", opt->aliases[alias_idx]);
            if (alias_idx + 1 < opt->alias_cnt) {
                printf(", ");
            }
        }
        printf(" %s\t%s\n", opt->usage, opt->description);
    }

    printf("\nTips: `%s <option> --h` for more information\n", app->argv[0]);
    exit(0);
}