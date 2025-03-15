#include "./compile.h"
#include "logger.h"
#include "streq.c"
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

void read_string_unit(FILE *input, const char end, CodeUnit *unit) {
    char *string = NULL;
    size_t string_len = 0;
    int cur_byte;
    while (1) {
        ++string_len;
        string = realloc(string, sizeof(char) * string_len);
        cur_byte = getc(input);
        string[string_len - 1] = cur_byte;
        switch (cur_byte) {
        case '\\':
            cur_byte = getc(input);
            string[string_len - 1] = cur_byte;
            switch (cur_byte) {
            case EOF:
                logger(DEBUG, "FileEOF");
                unit->type = FileEOF;
                return;
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
            unit->type = FileEOF;
            return;
        default:
            break;
        }
        if (string[string_len - 1] == end) {
            string[string_len - 1] = '\0';
            break;
        }
    }
    unit->data = string;
    LOG_DEBUG("String: %s", unit->data);
    unit->length = string_len;
    return;
}

void read_generic_unit(FILE *input, CodeUnit *unit) {
    char *unit_data = NULL;
    int cur_byte;
    int unit_len = 0;
    int is_continue = 1;
    while (is_continue) {
        ++unit_len;
        unit_data = realloc(unit_data, sizeof(char) * unit_len);
        cur_byte = getc(input);
        unit_data[unit_len - 1] = cur_byte;
        if (cur_byte == EOF && unit_len == 1) {
            logger(DEBUG, "FileEOF");
            unit->type = FileEOF;
            return;
        }
        switch (cur_byte) {
        case ' ':
        case '\n':
        case '\t':
        case '\r':
        case EOF:
            unit_data[unit_len - 1] = '\0';
            is_continue = 0;
            break;
        case ']':
            if (unit->type == DeclareAddr) {
                unit_data[unit_len - 1] = '\0';
                is_continue = 0;
            }
            break;
        }
    }
    unit->data = unit_data;
    LOG_DEBUG("UnitData: %s", unit->data);
    unit->length = unit_len;
    return;
}

CodeUnit *get_unit_type(CodeUnit *unit, int *buf, int *offset_ref) {
    switch (buf[0]) {
    case '"':
    case '\'':
        *offset_ref += 1; // Skip
        unit->type = String;
        break;
    case '#':
        *offset_ref += 1; // Skip
        unit->type = Comment;
        break;
    case '/':
        switch (buf[1]) {
        case '/':
            *offset_ref += 2; // Skip
            unit->type = Comment;
            break;
        case '*':
            *offset_ref += 2; // Skip
            unit->type = BlockComment;
            break;
        default:
            unit->type = Invalid;
        }
        break;
    case '-':
        switch (buf[1]) {
        case '-':
            *offset_ref += 2; // Skip
            unit->type = Comment;
            break;
        default:
            unit->type = Invalid;
            break;
        }
        break;
    case '[':
        *offset_ref += 1; // Skip
        unit->type = DeclareAddr;
        break;
    case '@':
        *offset_ref += 1; // Skip
        unit->type = RefAddr;
        break;
    default:
        unit->type = Normal;
        break;
    }
    return unit;
}

void skip_unit(CodeUnit *unit, FILE *input) {
    int buf[1]; // For EOF support
    // The whole unit is invalid
    do {
        buf[0] = getc(input);
    } while (buf[0] != ' ' && buf[0] != '\n' && buf[0] != '\t' &&
             buf[0] != '\r' && buf[0] != EOF);
    if (buf[0] == EOF) {
        fseek(input, -1, SEEK_CUR);
    }
}

int skip_whitespace(FILE *input) {
    int tmp;
    tmp = getc(input);
    while (tmp == ' ' || tmp == '\n' || tmp == '\t' || tmp == '\r') {
        tmp = getc(input);
    };
    fseek(input, -1, SEEK_CUR);
    return tmp;
}

int buf_read(int *buf, const int BUFFER_SIZE, FILE *input) {
    int offset = 0;
    for (offset = 0; offset < BUFFER_SIZE; offset++) {
        buf[offset] = getc(input);
        if (buf[offset] == EOF) {
            logger(DEBUG, "FileEOF at %d", offset);
            break;
        }
    }
    fseek(input, -offset, SEEK_CUR); // Make file pointer idle
    return offset;
}

CodeUnit *get_unit(FILE *input) {
    CodeUnit *unit = malloc(sizeof(CodeUnit));
    const int BUFFER_SIZE = 32;
    int buf[BUFFER_SIZE];
    int offset = 0;
    int *offset_ref = malloc(sizeof(int)); // For offset file pointer
    *offset_ref = offset;

    // Skip whitespace characters ahead
    buf[0] = skip_whitespace(input);
    if (buf[0] == EOF) {
        logger(DEBUG, "FileEOF");
        unit->type = FileEOF;
        return unit;
    }

    // Read non-empty bytes
    buf_read(buf, BUFFER_SIZE, input);

    // Get the unit type
    *offset_ref = 0;
    get_unit_type(unit, buf, offset_ref);
    offset = *offset_ref;
    LOG_DEBUG("Offset: %d", offset);
    fseek(input, offset, SEEK_CUR);
    free(offset_ref);

    // Read the unit data, or skip this unit
    switch (unit->type) {
    case FileEOF:
        break;
    case Invalid:
        skip_unit(unit, input);
        logger(DEBUG, "Invalid");
        break;
    case String:
        LOG_DEBUG("String buf[0]: %c", buf[0]);
        read_string_unit(input, buf[0], unit);
        break;
    case Comment:
        LOG_DEBUG("Skipped comment", NULL);
        while (buf[0] != '\n') {
            buf[0] = getc(input);
        }
        break;
    case BlockComment:
        while (buf[0] != '*' || buf[1] != '/') {
            // Move window
            buf_read(buf, BUFFER_SIZE, input);
            fseek(input, 1, SEEK_CUR);
        }
    case Normal:
    case DeclareAddr:
    case RefAddr:
        read_generic_unit(input, unit);
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

int compile_normal_unit(CodeUnit *unit) {
    unsigned int encode_res = 0xf0f0f;
    if (sscanf(unit->data, "%x", &encode_res) == EOF) {
        encode_res = 0xf0f0f;
    }
    for (size_t code_kind_idx = 0; code_kind_idx < CODE_KIND_CNT;
         ++code_kind_idx) {
        if (str_eqi(unit->data, code_table[code_kind_idx])) {
            encode_res = encode_table[code_kind_idx];
        }
    }
    LOG_DEBUG("Res: %02x", encode_res);
    return encode_res;
}

AddrNode *new_node(char *label, size_t addr) {
    AddrNode *new_node = malloc(sizeof(AddrNode));
    new_node->label = label;
    new_node->addr = addr;
    new_node->next = NULL;
    return new_node;
}

void append_node(AddrList *addr_list, AddrNode *node) {
    if (addr_list->head != NULL) {
        node->next = addr_list->head;
    }
    addr_list->head = node;
}

AddrNode *find_addr(AddrList *addr_list, char *label) {
    AddrNode *cur_node = addr_list->head;
    while (cur_node != NULL) {
        if (str_eq(cur_node->label, label)) {
            return cur_node;
        }
        cur_node = cur_node->next;
    }
    return NULL;
}

char *addr_to_bytes(AddrNode *addr_node) {
    char *addr_bytes = malloc(4 * sizeof(char));
    size_t addr = addr_node->addr;
    for (int i = 0; i < 4; i++) {
        addr_bytes[4 - i - 1] = (addr >> (8 * i)) & 0xff;
    }
    return addr_bytes;
}

void compile(FILE *input, FILE *output) {
    CodeUnit *unit = NULL;
    int tmp;
    int eof_sig = 0;
    AddrList *addr_list = malloc(sizeof(AddrList));
    addr_list->head = NULL;
    size_t cur_addr = 0;
    AddrNode *cur_node = NULL;
    char *addr_bytes;
    // 1 Loop for append addrs to addr_list
    LOG_DEBUG("Loop 1 Started", NULL);
    while (!eof_sig) {
        unit = get_unit(input);
        switch (unit->type) {
        case FileEOF:
            eof_sig = 1;
            free(unit);
            break;
        case DeclareAddr:
            cur_node = new_node(unit->data, cur_addr);
            append_node(addr_list, cur_node);
            printf("Declare(0x%08lx) -> %s\n", cur_node->addr, cur_node->label);
            printf("%d", cur_node->next == NULL);
            fflush(stdout);
            break;
        case Normal:
            cur_addr += 1;
            free(unit->data);
            free(unit);
            break;
        case String:
            cur_addr += (unit->length - 1);
            free(unit->data);
            free(unit);
            break;
        case Invalid:
            LOG_ERROR("Invalid code: %s", unit->data);
            exit(3);
        case RefAddr:
            cur_addr += 4;
            free(unit->data);
            free(unit);
            break;
        default:
            break;
        }
    }
    LOG_DEBUG("Declared Addrs: ", NULL);
    cur_node = addr_list->head;
    while (cur_node != NULL) {
        printf("Declare(0x%08lx) -> %s\n", cur_node->addr, cur_node->label);
        fflush(stdout);
        cur_node = cur_node->next;
    }
    fseek(input, 0, SEEK_SET);
    eof_sig = 0;
    LOG_DEBUG("Loop 2 Started", NULL);
    while (!eof_sig) {
        unit = get_unit(input);
        switch (unit->type) {
        case FileEOF:
            eof_sig = 1;
            fflush(output);
            return;
        case Normal:
            tmp = compile_normal_unit(unit);
            if (tmp != 0xf0f0f)
                fputc(tmp, output);
            else {
                LOG_ERROR("Unknown code: %s", unit->data);
                exit(3);
            }
            free(unit->data);
            free(unit);
            break;
        case String:
            fputs(unit->data, output);
            free(unit->data);
            free(unit);
            break;
        case RefAddr:
            cur_node = find_addr(addr_list, unit->data);
            if (cur_node == NULL) {
                LOG_ERROR("Unknown label: %s", unit->data);
                exit(3);
            }
            cur_addr = cur_node->addr;
            addr_bytes = addr_to_bytes(cur_node);
            printf("Ref(%s) -> 0x%02x 0x%02x 0x%02x 0x%02x\n", unit->data,
                   addr_bytes[0], addr_bytes[1], addr_bytes[2], addr_bytes[3]);
            fflush(stdout);
            fwrite(addr_bytes, 1, 4, output);
            free(unit->data);
            free(unit);
            break;
        default:
            break;
        }
    }
}