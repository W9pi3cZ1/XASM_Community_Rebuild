#include "logger.h"
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if defined __linux__ || defined __APPLE__
#include <termios.h>
#include <unistd.h>
int getch(void) {
    struct termios oldT, newT;
    int ch;
    tcgetattr(STDIN_FILENO, &oldT);
    newT = oldT;
    newT.c_lflag &= ~(ICANON | ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &newT);
    ch = getchar();
    tcsetattr(STDIN_FILENO, TCSANOW, &oldT);
    return ch;
}
#elif defined _WIN32
#include <conio.h>
#include <windows.h>
void usleep(int usec) {
    HANDLE timer;
    LARGE_INTEGER ft;

    ft.QuadPart = -(10 * usec);

    timer = CreateWaitableTimer(NULL, TRUE, NULL);
    SetWaitableTimer(timer, &ft, 0, NULL, NULL, 0);
    WaitForSingleObject(timer, INFINITE);
    CloseHandle(timer);
}
#endif

typedef struct VirtMem {
    int *mem;
    size_t size;
} VirtMem;

VirtMem init_virt_mem(size_t size) {
    size_t mem_size = 0;
    LOG_DEBUG("Initializing virtual memory with size: %zu", size);
    int *mem_vec;
    if (size < 0xf00000) {
        mem_size = size;
        mem_vec = malloc(mem_size * sizeof(int));
    } else {
        mem_size = size;
        mem_vec = malloc(mem_size * sizeof(int));
        if (mem_vec == NULL) {
            for (mem_size = 0xf00000; mem_size < size; mem_size += 0xf00000) {
                if (mem_vec == NULL) {
                    mem_vec = malloc(mem_size * sizeof(int));
                } else {
                    mem_vec = realloc(mem_vec, mem_size * sizeof(int));
                }
            }
            mem_vec = realloc(mem_vec, (size * sizeof(int)));
        }
    }
    if (mem_vec == NULL) {
        LOG_ERROR("Failed to allocate memory", NULL);
    }
    VirtMem virt_mem = {
        .mem = mem_vec,
        .size = mem_size,
    };
    return virt_mem;
}

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

size_t get_addr(int *mem_ptr) {
    // LOG_DEBUG("Bytes[0]:%02x", mem_ptr[0]);
    // LOG_DEBUG("Bytes[3]:%02x", mem_ptr[3]);
    size_t addr = (mem_ptr[0] << 24) + (mem_ptr[1] << 16) + (mem_ptr[2] << 8) +
                  mem_ptr[3];
    LOG_DEBUG("Addr:%08x", addr);
    return addr;
}

void log_mem(VirtMem virt_mem, size_t begin, size_t end, LogLevel lvl) {
    int idx;
    for (idx = begin; idx < end && idx < virt_mem.size; idx++) {
        if (idx % 0x10 == 0) {
            logger_feat(1, 0, lvl, "0x%08zx|", idx);
        }
        logger_feat(1, 1, lvl, "%02x ", virt_mem.mem[idx]);
        if ((idx + 1) % 0x10 == 0) {
            logger_feat(1, 1, lvl, "\n", NULL);
        }
    }
    logger_feat(1, 1, lvl, "\n", NULL);
}

void execute_code(VirtMem virt_mem, size_t cpu_clock, size_t src_size) {
    size_t byte_idx = 0;
    State state = Run;
    LOG_DEBUG("CPU Clock:%zu", cpu_clock);
    size_t microsec_per_cmd = cpu_clock == 0 ? 0 : 1000000 / cpu_clock;
    int data[16];
    size_t addr[4];
    size_t cmd_cnt = 0;
    while (byte_idx < virt_mem.size && state != Halt) {
        ++cmd_cnt;
        if (cmd_cnt % 100 == 0) {
            log_mem(virt_mem, 0, src_size, DEBUG);
            log_mem(virt_mem, 0xf000, 0xf100, DEBUG);
        }
        if (cpu_clock != 0) {
            usleep(microsec_per_cmd);
        };
        memcpy(data, virt_mem.mem + byte_idx, 16 * sizeof(int));
        addr[0] = get_addr(&data[1]);
        addr[1] = get_addr(&data[5]);
        addr[2] = get_addr(&data[9]);
        addr[3] = get_addr(&data[13]);
        switch (data[0]) {
        case Add:
            LOG_DEBUG("Add", NULL);
            virt_mem.mem[addr[0]] += virt_mem.mem[addr[1]];
            LOG_DEBUG("Mem[%08x]:%08x", addr[0], virt_mem.mem[addr[0]]);
            byte_idx += ADDR_SIZE * 2;
            break;
        case Sub:
            LOG_DEBUG("Sub", NULL);
            virt_mem.mem[addr[0]] -= virt_mem.mem[addr[1]];
            LOG_DEBUG("Mem[%08x]:%08x", addr[0], virt_mem.mem[addr[0]]);
            byte_idx += ADDR_SIZE * 2;
            break;
        case XAdd:
            LOG_DEBUG("XAdd", NULL);
            virt_mem.mem[addr[0]] *= virt_mem.mem[addr[1]];
            LOG_DEBUG("Mem[%08x]:%08x", addr[0], virt_mem.mem[addr[0]]);
            byte_idx += ADDR_SIZE * 2;
            break;
        case XSub:
            LOG_DEBUG("XSub", NULL);
            virt_mem.mem[addr[0]] /= virt_mem.mem[addr[1]];
            LOG_DEBUG("Mem[%08x]:%08x", addr[0], virt_mem.mem[addr[0]]);
            byte_idx += ADDR_SIZE * 2;
            break;
        case And:
            LOG_DEBUG("And", NULL);
            virt_mem.mem[addr[2]] =
                virt_mem.mem[addr[0]] && virt_mem.mem[addr[1]];
            byte_idx += ADDR_SIZE * 3;
            break;
        case Or:
            LOG_DEBUG("Or", NULL);
            virt_mem.mem[addr[2]] =
                virt_mem.mem[addr[0]] || virt_mem.mem[addr[1]];
            byte_idx += ADDR_SIZE * 3;
            break;
        case Not:
            LOG_DEBUG("Not", NULL);
            virt_mem.mem[addr[1]] = !virt_mem.mem[addr[0]];
            byte_idx += ADDR_SIZE * 2;
            break;
        case Mov:
            LOG_DEBUG("Mov", NULL);
            virt_mem.mem[addr[0]] = addr[1];
            byte_idx += ADDR_SIZE * 2;
            break;
        case Copy:
            LOG_DEBUG("Copy", NULL);
            virt_mem.mem[addr[0]] = virt_mem.mem[addr[1]];
            byte_idx += ADDR_SIZE * 2;
            break;
        case Goto:
            LOG_DEBUG("Goto", NULL);
            byte_idx = virt_mem.mem[addr[0]];
            byte_idx--;
            break;
        case Geta:
            LOG_DEBUG("Geta", NULL);
            byte_idx += ADDR_SIZE;
            virt_mem.mem[addr[0]] = byte_idx + 1;
            LOG_DEBUG("Mem[%08x]:%08x", addr[0], virt_mem.mem[addr[0]]);
            break;
        case Gt:
            LOG_DEBUG("Gt", NULL);
            if (virt_mem.mem[addr[0]] > virt_mem.mem[addr[1]]) {
                byte_idx = virt_mem.mem[addr[2]] - 1;
                LOG_DEBUG("%08x > %08x = True", virt_mem.mem[addr[0]],
                          virt_mem.mem[addr[1]]);
            } else {
                byte_idx += ADDR_SIZE * 3;
            }
            break;
        case Lt:
            LOG_DEBUG("Lt", NULL);
            if (virt_mem.mem[addr[0]] < virt_mem.mem[addr[1]]) {
                byte_idx = virt_mem.mem[addr[2]] - 1;
                LOG_DEBUG("%08x < %08x = True", virt_mem.mem[addr[0]],
                          virt_mem.mem[addr[1]]);
            } else {
                byte_idx += ADDR_SIZE * 3;
            }
            break;
        case Eq:
            LOG_DEBUG("Eq", NULL);
            if (virt_mem.mem[addr[0]] == virt_mem.mem[addr[1]]) {
                byte_idx = virt_mem.mem[addr[2]] - 1;
                LOG_DEBUG("%08x == %08x = True", virt_mem.mem[addr[0]],
                          virt_mem.mem[addr[1]]);
            } else {
                byte_idx += ADDR_SIZE * 3;
            }
            break;
        case Lm:
            LOG_DEBUG("Lm", NULL);
            virt_mem.mem[virt_mem.mem[addr[0]] - virt_mem.mem[addr[1]]] =
                virt_mem.mem[addr[0]];
            virt_mem.mem[addr[0]] = Void;
            byte_idx += ADDR_SIZE * 2;
            break;
        case Rm:
            LOG_DEBUG("Rm", NULL);
            virt_mem.mem[virt_mem.mem[addr[0]] + virt_mem.mem[addr[1]]] =
                virt_mem.mem[addr[0]];
            virt_mem.mem[addr[0]] = Void;
            byte_idx += ADDR_SIZE * 2;
            break;
        case Exit:
            LOG_DEBUG("Halted", NULL);
            state = Halt;
            return;
            break;
        case Sto:
            LOG_DEBUG("Sto", NULL);
            while (virt_mem.mem[++byte_idx] != Sto)
                ;
            break;
        case Putc:
            LOG_DEBUG("Putc", NULL);
            putchar(virt_mem.mem[virt_mem.mem[addr[0]]]);
            byte_idx += ADDR_SIZE;
            break;
        case Putn:
            LOG_DEBUG("Putn", NULL);
            printf("%d", virt_mem.mem[virt_mem.mem[addr[0]]]);
            byte_idx += ADDR_SIZE;
            break;
        case Puth:
            LOG_DEBUG("Puth", NULL);
            printf("%x", virt_mem.mem[virt_mem.mem[addr[0]]]);
            byte_idx += ADDR_SIZE;
            break;
        case Getc:
            LOG_DEBUG("Getc", NULL);
            virt_mem.mem[addr[0]] = getch();
            byte_idx += ADDR_SIZE;
            break;
        case Getn:
            LOG_DEBUG("Getn", NULL);
            data[15] = 0;
            scanf("%d", &data[15]);
            virt_mem.mem[addr[0]] = data[15];
            byte_idx += ADDR_SIZE;
            break;
        case Geth:
            LOG_DEBUG("Geth", NULL);
            data[15] = 0;
            scanf("%x", &data[15]);
            virt_mem.mem[addr[0]] = data[15];
            byte_idx += ADDR_SIZE;
            break;
            // TODO: ADD MORE
        case BAnd:
            LOG_DEBUG("BAnd", NULL);
            virt_mem.mem[addr[0]] =
                virt_mem.mem[addr[0]] & virt_mem.mem[addr[1]];
            byte_idx += ADDR_SIZE * 2;
            break;
        case BOr:
            LOG_DEBUG("BOr", NULL);
            virt_mem.mem[addr[0]] =
                virt_mem.mem[addr[0]] | virt_mem.mem[addr[1]];
            byte_idx += ADDR_SIZE * 2;
            break;
        case XOr:
            LOG_DEBUG("XOr", NULL);
            virt_mem.mem[addr[0]] =
                virt_mem.mem[addr[0]] ^ virt_mem.mem[addr[1]];
            byte_idx += ADDR_SIZE * 2;
            break;
        case BNot:
            LOG_DEBUG("BNot", NULL);
            virt_mem.mem[addr[0]] = ~virt_mem.mem[addr[0]];
            byte_idx += ADDR_SIZE;
            break;
        case Shl:
            LOG_DEBUG("Shl", NULL);
            virt_mem.mem[addr[0]] <<= virt_mem.mem[addr[1]];
            byte_idx += ADDR_SIZE * 2;
            break;
        case Shr:
            LOG_DEBUG("Shr", NULL);
            virt_mem.mem[addr[0]] >>= virt_mem.mem[addr[1]];
            byte_idx += ADDR_SIZE * 2;
            break;
        case Void:
            LOG_WARN("Segfault", NULL);
            state = Halt;
            return;
        default:
            LOG_FATAL("Unknown ByteCode `%02x`", data[0], NULL);
            break;
        }
        fflush(stdout);
        ++byte_idx;
    }
}

void run_file(VirtMem virt_mem, FILE *src_file, size_t cpu_clock) {
    size_t byte_idx;
    for (byte_idx = 0; byte_idx < virt_mem.size &&
                       (virt_mem.mem[byte_idx] = fgetc(src_file)) != EOF;
         byte_idx++)
        ;

    size_t src_size;
    LOG_INFO("FirstByte: %02x", virt_mem.mem[0]);
    LOG_INFO("LastByte: %02x", virt_mem.mem[byte_idx]);
    if (virt_mem.mem[byte_idx] == EOF) {
        src_size = byte_idx;
        virt_mem.mem[byte_idx] = Void;
    } else {
        LOG_FATAL("Memory not enough to load the source file", NULL);
    }

    // DEBUG MEM CONTENT
    LOG_INFO("Memory content", NULL);
    log_mem(virt_mem, 0, src_size, INFO);

    execute_code(virt_mem, cpu_clock, src_size);
}