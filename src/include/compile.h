#include <stddef.h>
#include <stdio.h>

enum CodeUnitType {
    Normal,
    String,
    FileEOF,
    Comment,
    BlockComment,
    Invalid,
    DeclareAddr,
    RefAddr,
};

typedef struct AddrNode {
    size_t addr;
    char *label;
    struct AddrNode *next;
} AddrNode;

typedef struct AddrList {
    AddrNode *head;
} AddrList;

typedef struct CodeUnit {
    enum CodeUnitType type;
    size_t length;
    char *data;
} CodeUnit;

CodeUnit *get_unit(FILE *input);

void compile(FILE *input, FILE *output);

void read_string_unit(FILE *input, const char end, CodeUnit *unit);

void read_generic_unit(FILE *input, CodeUnit *unit);

int compile_normal_unit(CodeUnit *unit);