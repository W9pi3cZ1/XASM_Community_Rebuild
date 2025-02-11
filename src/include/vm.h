#include <stddef.h>

// typedef struct VirtMem {
//     int *mem;
//     size_t size;
// } VirtMem;

typedef struct MemChunk {
    size_t start_addr;
    size_t end_addr;
    int *mem;
    struct MemChunk *next_chunk;
} MemChunk;

typedef struct VirtMem {
    int chunk_size;
    size_t max_size;
    MemChunk *head;
} VirtMem;

typedef enum ByteCode {
    Add = 0x01,
    Sub = 0x02,
    XAdd = 0x03,
    XSub = 0x04,
    And = 0x11,
    Or = 0x12,
    Not = 0x13,
    Mov = 0x21,
    Copy = 0x22,
    Goto = 0x23,
    Geta = 0x24,
    Gt = 0x25,
    Lt = 0x26,
    Eq = 0x27,
    Lm = 0x28,
    Rm = 0x29,
    Exit = 0x2a,
    Sto = 0xff,
    Putc = 0x31,
    Putn = 0x32,
    Puth = 0x33,
    Getc = 0x34,
    Getn = 0x35,
    Geth = 0x36,
    BAnd = 0x41,
    BOr = 0x42,
    XOr = 0x43,
    BNot = 0x44,
    Shl = 0x45,
    Shr = 0x46,
    Void = 0x00,
} ByteCode;

typedef enum State {
    Run,
    Halt,
} State;

#define ADDR_SIZE 4