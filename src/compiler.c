#include "./include/argparse.c"
#include "./include/compile.c"
#include <stdlib.h>

FILE *input_file;
FILE *output_file;

FILE *open_file(char *file_path, const char *modes) {
    FILE *file = fopen(file_path, modes);
    if (file == NULL) {
        LOG_FATAL("Cannot open file `%s`", file_path);
        exit(1);
    } else {
        LOG_INFO("Openned File `%s` ", file_path);
    }
    return file;
}

void enter_input(char **argv) {
    char *input_path = argv[0];
    LOG_INFO("Opening File `%s`", input_path);
    input_file = open_file(input_path, "r");
}

void enter_output(char **argv) {
    char *output_path = argv[0];
    LOG_INFO("Opening File `%s`", output_path);
    output_file = open_file(output_path, "w");
}

const char *input_aliases[] = {"-i", "--input"};

Option input_opt = {
    .name = "input",
    .description = "Path to the input file",
    .usage = "<input_file>",
    .argc = 1,
    .aliases = input_aliases,
    .alias_cnt = 2,
    .callback = enter_input,
};

const char *output_aliases[] = {"-o", "--output"};

Option output_opt = {
    .alias_cnt = 2,
    .aliases = output_aliases,
    .argc = 1,
    .callback = enter_output,
    .name = "output",
    .description = "Path to the output file",
    .usage = "<output_file>",
};

const char *help_aliases[] = {"-h", "--help"};

Option help_opt = {
    .alias_cnt = 2,
    .aliases = help_aliases,
    .argc = 0,
    .name = "help",
    .description = "Prints this help message",
    .usage = "\t\t", // Align
};

const char *log_lvl_aliases[] = {"-L", "--log-level"};

Option log_lvl_opt = {
    .alias_cnt = 2,
    .aliases = log_lvl_aliases,
    .argc = 1,
    .callback = set_log_lvl,
    .name = "log-level",
    .description = "Sets the log level (e.g. 0=INFO, 1=WARN, 2=ERROR...)",
    .help = "0=INFO, 1=WARN, 2=ERROR, 3=FATAL, 4=DEBUG",
    .usage = "<log_level>",
};

static Option *opts[] = {&input_opt, &output_opt, &help_opt, &log_lvl_opt};

Application app = {
    .name = "XASM Community Compiler",
    .description = "A compiler for the XASM language",
    .usage = "[<options> ...]",
    .version = "a0.0.4",
    .opt_cnt = 4,
    .opts = opts,
};

void print_help(char **argv) { show_app_help(&app); }

Application init_app(int argc, char **argv) {
    app.argc = argc;
    app.argv = argv;
    help_opt.callback = print_help;
    return app;
}

int main(int argc, char **argv) {
    Application compiler = init_app(argc, argv);
    parse_args(&compiler);
    if (input_file == NULL) {
        LOG_ERROR("Input file was not entered.", NULL);
        exit(1);
    }
    if (output_file == NULL) {
        LOG_WARN("Output file was not entered, using default file `out.bin`.",
                 NULL);
        output_file = open_file("out.bin", "w");
    }
    compile(input_file, output_file);
    return 0;
}