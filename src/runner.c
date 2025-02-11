#include "./include/argparse.c"
#include "./include/vm.c"
#include <stddef.h>

FILE *input_file;
size_t cpu_clock;
size_t mem_size;

void set_cpu_clock(char **argv) {
    cpu_clock = atoi(argv[0]);
    LOG_INFO("Setting CPU clock to %zu", cpu_clock);
}

void enter_input(char **argv) {
    char *input_path = argv[0];
    input_file = fopen(input_path, "r");
    LOG_INFO("Opening File `%s`", input_path);
    if (input_file == NULL) {
        LOG_FATAL("Cannot open input file `%s`", input_path);
    } else {
        LOG_INFO("Openned File `%s` ", input_path);
    }
}

void set_memory(char **argv) {
    mem_size = atol(argv[0]);
    LOG_INFO("Setting memory size to %zu", mem_size);
}

const char *input_aliases[] = {"-i", "--input"};

Option input_opt = {
    .alias_cnt = 2,
    .aliases = input_aliases,
    .argc = 1,
    .callback = enter_input,
    .name = "input",
    .description = "Path to the input file",
    .usage = "<input_file>",
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

const char *clock_aliases[] = {"-c", "--clock"};

Option clock_opt = {
    .alias_cnt = 2,
    .aliases = clock_aliases,
    .argc = 1,
    .name = "clock",
    .description = "Sets the CPU clock speed",
    .usage = "XXhz",
    .callback = set_cpu_clock,
};

const char *mem_aliases[] = {"-m", "--memory"};

Option mem_opt = {
    .alias_cnt = 2,
    .aliases = mem_aliases,
    .argc = 1,
    .name = "memory",
    .description = "Sets the memory size",
    .usage = "XX(bytes)",
    .callback = set_memory,
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

static Option *opts[] = {&input_opt, &help_opt, &clock_opt, &mem_opt,
                         &log_lvl_opt};

Application app = {
    .name = "XASM Community Runner",
    .description = "A Runner for the XASM language",
    .usage = "[<options> ...]",
    .version = "a0.0.1",
    .opt_cnt = 5,
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
    Application runner = init_app(argc, argv);
    parse_args(&runner);
    if (input_file == NULL) {
        LOG_FATAL("No input file provided", NULL);
    } else {
        LOG_INFO("Running VM", NULL);
        if (mem_size == 0) {
            mem_size = 0xffff;
        }
        VirtMem *virt_mem = init_virt_mem(mem_size);
        run_file(virt_mem, input_file, cpu_clock);
    }
    return 0;
}