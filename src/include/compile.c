#include "./compile.h"
#include "logger.h"
#include "streq.c"
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

CodeUnit read_string(FILE *input, const char end) {
    CodeUnit unit;
    char *string = NULL;
    size_t string_len = 0;
    while (1) {
        ++string_len;
        string = realloc(string, sizeof(char) * string_len);
        string[string_len - 1] = getc(input);
        switch (string[string_len - 1]) {
        case '\\':
            string[string_len - 1] = getc(input);
            switch (string[string_len - 1]) {
            case EOF:
                logger(DEBUG, "FileEOF");
                unit.type = FileEOF;
                return unit;
            case 'n':
                string[string_len - 1] = '\n';
                break;
            case 't':
                string[string_len - 1] = '\t';
                break;
            case 'r':
                string[string_len - 1] = '\r';
                break;
            case 'x':
                fscanf(input, "%02hhx", &string[string_len - 1]);
            default:
                break;
            }

            break;
        case EOF:
            logger(DEBUG, "FileEOF");
            unit.type = FileEOF;
            return unit;
        default:
            break;
        }
        if (string[string_len - 1] == end) {
            string[string_len - 1] = '\0';
            break;
        }
    }
    unit.data = string;
    LOG_DEBUG("String: %s", unit.data);
    unit.length = string_len;
    unit.type = String;
    return unit;
}

CodeUnit read_normal_unit(FILE *input) {
    CodeUnit unit;
    char *unit_data = NULL;
    int unit_len = 0;
    while (1) {
        ++unit_len;
        unit_data = realloc(unit_data, sizeof(char) * unit_len);
        unit_data[unit_len - 1] = getc(input);
        if (unit_data[unit_len - 1] == EOF && unit_len == 1) {
            logger(DEBUG, "FileEOF");
            unit.type = FileEOF;
            return unit;
        }
        if (unit_data[unit_len - 1] == ' ' || unit_data[unit_len - 1] == '\n' ||
            unit_data[unit_len - 1] == '\t' ||
            unit_data[unit_len - 1] == '\r' || unit_data[unit_len - 1] == EOF) {
            unit_data[unit_len - 1] = '\0';
            break;
        }
    }
    unit.data = unit_data;
    LOG_DEBUG("UnitData: %s", unit.data);
    unit.length = unit_len;
    unit.type = Normal;
    return unit;
}

CodeUnit get_unit(FILE *input) {
    CodeUnit unit;
    char begin[2];

    if (fscanf(input, "%1s", begin) == EOF) {
        unit.type = FileEOF;
        logger(DEBUG, "FileEOF");
        return unit;
    }

    LOG_DEBUG("Begin: %s", begin);

    switch (begin[0]) {
    case '"':
        unit.type = String;
        unit = read_string(input, '"');
        break;
    case '\'':
        unit.type = String;
        unit = read_string(input, '\'');
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
        } else if (begin[1] == '*') {
            logger(DEBUG, "BlockComment Begin");
            unit.type = BlockComment;
        } else {
            unit.type = Invalid;
        }
        break;

    case '-':
        begin[1] = getc(input);
        if (begin[1] == '-') {
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
    case Comment:
        fscanf(input, "%*[^\n]\n");
        logger(DEBUG, "Comment");
        break;
    case Normal:
        fseek(input, -1, SEEK_CUR);
        unit = read_normal_unit(input);
        logger(DEBUG, "Normal");
        break;
    case BlockComment:
        logger(DEBUG, "BlockComment");
        while ((begin[0] = getc(input)) != EOF) {
            if (begin[0] == '*' && (begin[1] = getc(input)) == '/') {
                break;
            }
        }
        logger(DEBUG, "BlockComment End");
        break;
    default:
        break;
    }

    return unit;
}

static char *code_table[] = {
    "add",  "sub",  "xadd", "xsub", "and", "or", "not",  "mov", "copy", "goto",
    "geta", ">",    "<",    "=",    "lm",  "rm", "exit", "sto", "putc", "putn",
    "puth", "getc", "getn", "geth", "&",   "|",  "^",    "~",   "<<",   ">>",
};

static char encode_table[] = {
    0x01, 0x02, 0x03, 0x04, 0x11, 0x12, 0x13, 0x21, 0x22, 0x23,
    0x24, 0x25, 0x26, 0x27, 0x28, 0x29, 0x2a, 0xff, 0x31, 0x32,
    0x33, 0x34, 0x35, 0x36, 0x41, 0x42, 0x43, 0x44, 0x45, 0x46,
};

const size_t CODE_KIND_CNT = 30;

int compile_normal_unit(CodeUnit unit) {
    unsigned int encode_res = 0xf0f0f;
    if (sscanf(unit.data, "%x", &encode_res) == EOF) {
        encode_res = 0xf0f0f;
    }
    for (size_t code_kind_idx = 0; code_kind_idx < CODE_KIND_CNT;
         ++code_kind_idx) {
        if (str_eqi(unit.data, code_table[code_kind_idx])) {
            encode_res = encode_table[code_kind_idx];
        }
    }
    LOG_DEBUG("Res: %02x", encode_res);
    return encode_res;
}

void compile(FILE *input, FILE *output) {
    CodeUnit unit;
    int tmp;
    while (1) {
        unit = get_unit(input);
        switch (unit.type) {
        case FileEOF:
            fflush(output);
            return;
        case Normal:
            tmp = compile_normal_unit(unit);
            if (tmp != 0xf0f0f)
                fputc(tmp, output);
            else {
                LOG_ERROR("Unknown code: %s", unit.data);
                exit(3);
            }
            break;
        case String:
            fputs(unit.data, output);
            break;
        case Invalid:
            LOG_ERROR("Invalid code: %s", unit.data);
            exit(3);
        default:
            break;
        }
    }
}