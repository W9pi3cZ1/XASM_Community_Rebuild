#include <stddef.h>
#include <stdio.h>

enum CodeUnitType {
    Normal,
    String,
    FileEOF,
    Comment,
    BlockComment,
    Invalid,
};

typedef struct CodeUnit {
    enum CodeUnitType type;
    size_t length;
    char *data;
} CodeUnit;

CodeUnit get_unit(FILE *input);

void compile(FILE *input, FILE *output);

CodeUnit read_string_unit(FILE *input, const char end);

CodeUnit read_normal_unit(FILE *input);

int compile_normal_unit(CodeUnit unit);