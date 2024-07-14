#include "./include/argparse.c"
#include "./include/compile.c"

FILE *input_file;
FILE *output_file;

void enter_input(char **argv) {
    char *input_path = argv[0];
    input_file = fopen(input_path, "r");
    LOG_DEBUG("Opening File `%s`", input_path);
    if (input_file == NULL) {
        LOG_FATAL("Cannot open input file `%s`", input_path);
    } else {
        LOG_DEBUG("Openned File `%s` ", input_path);
    }
}

void enter_output(char **argv) {
    char *output_path = argv[0];
    output_file = fopen(output_path, "w");
    LOG_DEBUG("Opening File `%s`", output_path);
    if (input_file == NULL) {
        LOG_FATAL("Cannot open output file `%s`", output_path);
    } else {
        LOG_DEBUG("Openned File `%s` ", output_path);
    }
}

static char *input_aliases[] = {"-i", "--input"};

Option input_opt = {
    .alias_cnt = 2,
    .aliases = input_aliases,
    .argc = 1,
    .callback = enter_input,
    .name = "input",
    .description = "Path to the input file",
    .usage = "<input_file>",
};

static char *output_aliases[] = {"-o", "--output"};

Option output_opt = {
    .alias_cnt = 2,
    .aliases = output_aliases,
    .argc = 1,
    .callback = enter_output,
    .name = "output",
    .description = "Path to the output file",
    .usage = "<output_file>",
};

static char *help_aliases[] = {"-h", "--help"};

Option help_opt = {
    .alias_cnt = 2,
    .aliases = help_aliases,
    .argc = 0,
    .name = "help",
    .description = "Prints this help message",
    .usage = "\t\t", // Align
};

static Option *opts[] = {&input_opt, &output_opt, &help_opt};

Application app = {
    .name = "XASM Community Compiler",
    .description = "A compiler for the XASM language",
    .usage = "[<options> ...]",
    .version = "a0.0.1",
    .opt_cnt = 3,
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
    compile(input_file, output_file);
    return 0;
}