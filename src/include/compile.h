#include <stddef.h>
#include <stdio.h>

enum CodeUnitType { Normal, String, FileEOF, Comment, Invalid };

typedef struct CodeUnit {
    enum CodeUnitType type;
    size_t length;
    char *data;
} CodeUnit;

CodeUnit get_unit(FILE *input);

void compile(FILE *input, FILE *output);