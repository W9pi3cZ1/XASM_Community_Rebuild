#include "./compile.h"
#include "logger.h"
#include <stdio.h>

CodeUnit get_unit(FILE *input) {
    CodeUnit unit;
    char begin[2];

    if (fscanf(input, "%1s", begin) == EOF) {
        unit.type = FileEOF;
        logger(DEBUG, "FileEOF");

        return unit;
    }

    LOG_DEBUG("Begin: %s", begin);

    long old_read_idx = ftell(input);

    switch (begin[0]) {
    case '"':
        unit.type = String;
        break;
    case '\'':
        unit.type = String;
        break;
    case '#':
        unit.type = Comment;
        break;
    case '/':
        begin[1] = getc(input);
        if (begin[1] == '/') {
            unit.type = Comment;
            break;
        } else if (begin[1] == EOF) {
            unit.type = FileEOF;
            logger(DEBUG, "FileEOF");
            return unit;
        } else {
            unit.type = Invalid;
        }
        break;
    default:
        unit.type = Normal;
    }

    switch (unit.type) {
    case String:
        logger(DEBUG, "String");
        break;
    case Comment:
        fscanf(input, "%*[^\n]\n");
        logger(DEBUG, "Comment");
        break;
    case Normal:
        logger(DEBUG, "Normal");
        break;
    case Invalid:
        logger(DEBUG, "Invalid");
        break;
    case FileEOF:
        logger(DEBUG, "FileEOF");
        break;
    }

    return unit;
}

void compile(FILE *input, FILE *output) {
    char *readbuf;
    CodeUnit unit;
    while ((unit = get_unit(input)).type != FileEOF) {
    }
}