#include "vm.h"
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

VirtMem *init_virt_mem(size_t max_size) {
    VirtMem *virt_mem = malloc(sizeof(VirtMem));
    virt_mem->chunk_size = 0xff;
    virt_mem->max_size = max_size;
    virt_mem->head = NULL;
    return virt_mem;
}

MemChunk *find_chunk(VirtMem *virt_mem, size_t addr) {
    MemChunk *chunk = virt_mem->head;
    if (addr > virt_mem->max_size) {
        LOG_FATAL("Address %zx is out of range.", addr);
        exit(-1);
        return NULL;
    }
    MemChunk *prev_chunk = NULL;
    while (chunk != NULL) {
        if (addr >= chunk->start_addr && addr <= chunk->end_addr) {
            // Optimized multiple access to chunks
            if (prev_chunk != NULL) {
                prev_chunk->next_chunk = chunk->next_chunk;
                chunk->next_chunk = virt_mem->head;
                virt_mem->head = chunk;
            }
            return chunk;
        }
        prev_chunk = chunk;
        chunk = chunk->next_chunk;
    }
    return NULL;
}

VirtMem *alloc_chunk(VirtMem *virt_mem, size_t addr) {
    size_t start_addr = addr / virt_mem->chunk_size * virt_mem->chunk_size;
    size_t end_addr = start_addr + virt_mem->chunk_size - 1;
    if (end_addr > virt_mem->max_size) {
        LOG_FATAL("Address %zx is out of range.", addr);
        exit(-1);
        return virt_mem; // Never return
    }
    MemChunk *chunk = malloc(sizeof(MemChunk));
    chunk->start_addr = start_addr;
    chunk->end_addr = end_addr;
    chunk->mem = malloc(virt_mem->chunk_size * sizeof(int));
    chunk->next_chunk = virt_mem->head;
    virt_mem->head = chunk;
    return virt_mem;
}

MemChunk *get_chunk(VirtMem *virt_mem, size_t addr) {
    MemChunk *chunk = find_chunk(virt_mem, addr);
    if (chunk == NULL) {
        // Allocate chunk
        LOG_DEBUG("Allocating chunk for address %zx", addr);
        // Alloc a chunk by addr and insert it into the head.
        virt_mem = alloc_chunk(virt_mem, addr);
        chunk = virt_mem->head;
    }
    return chunk;
}

int *read_virtmem(VirtMem *virt_mem, size_t addr) {
    MemChunk *chunk = get_chunk(virt_mem, addr);
    return (chunk->mem) + (addr - chunk->start_addr);
}

void virtmem_cpy(VirtMem *virt_mem, int *dest, size_t src_addr, size_t n) {
    LOG_DEBUG("Calling virtmem_cpy", NULL);
    MemChunk *chunk = get_chunk(virt_mem, src_addr);
    size_t chunk_offset = src_addr - chunk->start_addr;
    for (size_t idx = 0; idx < n; idx++) {
        // Overflow chunk
        if (idx + chunk_offset > chunk->end_addr - chunk->start_addr) {
            LOG_DEBUG("Overflow chunk", NULL);
            chunk = get_chunk(virt_mem, chunk->end_addr + 1);
            // Recalculate chunk_offset
            chunk_offset = -idx;
        }
        LOG_DEBUG("Copying (chunk[%02x])%02x to dest[%02x]", chunk_offset + idx,
                  chunk->mem[chunk_offset + idx], idx);
        dest[idx] = chunk->mem[chunk_offset + idx];
    }
};

size_t merge_addr(int *mem_ptr) {
    // LOG_DEBUG("Bytes[0]:%02x", mem_ptr[0]);
    // LOG_DEBUG("Bytes[3]:%02x", mem_ptr[3]);
    size_t addr = (mem_ptr[0] << 24) + (mem_ptr[1] << 16) + (mem_ptr[2] << 8) +
                  mem_ptr[3];
    LOG_DEBUG("Addr:%08x", addr);
    return addr;
}

void log_mem(VirtMem *virt_mem, size_t begin, size_t end, LogLevel lvl) {
    int idx;
    for (idx = begin; idx < end && idx < virt_mem->max_size; idx++) {
        if (idx % 0x10 == 0) {
            logger_feat(1, 0, lvl, "0x%08zx|", idx);
        }
        logger_feat(1, 1, lvl, "%02x ", *read_virtmem(virt_mem, idx));
        if ((idx + 1) % 0x10 == 0) {
            logger_feat(1, 1, lvl, "\n", NULL);
        }
    }
    logger_feat(1, 1, lvl, "\n", NULL);
}

int *deref(VirtMem *virt_mem, size_t addr, int **mem_ref) {
    *mem_ref = read_virtmem(virt_mem, addr);
    return *mem_ref;
}

void deref_addrs(VirtMem *virt_mem, size_t *addr, int *mem_ref[], int count) {
    for (int i = 0; i < count; i++) {
        deref(virt_mem, addr[i], &mem_ref[i]);
    }
}

void execute_code(VirtMem *virt_mem, size_t cpu_clock, size_t src_size) {
    size_t byte_idx = 0;
    State state = Run;
    LOG_DEBUG("CPU Clock:%zu", cpu_clock);
    size_t microsec_per_cmd = cpu_clock == 0 ? 0 : 1000000 / cpu_clock;
    int data[16];
    int *mem_ref[16]; // Memory reference
    size_t addr[4];
    size_t cmd_cnt = 0;
    while (byte_idx < virt_mem->max_size && state != Halt) {
        ++cmd_cnt;
        if (cmd_cnt % 100 == 0) {
            log_mem(virt_mem, 0, src_size, DEBUG);
            log_mem(virt_mem, 0xf000, 0xf100, DEBUG);
        }
        if (cpu_clock != 0) {
            usleep(microsec_per_cmd);
        };
        virtmem_cpy(virt_mem, data, byte_idx, 16);
        for (int i = 0; i < 4; i++) {
            addr[i] = merge_addr(&data[1 + i * 4]);
        }
        switch (data[0]) {
        case Add:
            LOG_DEBUG("Add", NULL);
            deref_addrs(virt_mem, addr, mem_ref, 2);
            *mem_ref[0] += *mem_ref[1];
            LOG_DEBUG("Mem[%08x]:%08x", addr[0], *mem_ref[0]);
            byte_idx += ADDR_SIZE * 2;
            break;
        case Sub:
            LOG_DEBUG("Sub", NULL);
            deref_addrs(virt_mem, addr, mem_ref, 2);
            *mem_ref[0] -= *mem_ref[1];
            LOG_DEBUG("Mem[%08x]:%08x", addr[0], *mem_ref[0]);
            byte_idx += ADDR_SIZE * 2;
            break;
        case XAdd:
            LOG_DEBUG("XAdd", NULL);
            deref_addrs(virt_mem, addr, mem_ref, 2);
            *mem_ref[0] *= *mem_ref[1];
            LOG_DEBUG("Mem[%08x]:%08x", addr[0], *mem_ref[0]);
            byte_idx += ADDR_SIZE * 2;
            break;
        case XSub:
            LOG_DEBUG("XSub", NULL);
            deref_addrs(virt_mem, addr, mem_ref, 2);
            *mem_ref[0] /= *mem_ref[1];
            LOG_DEBUG("Mem[%08x]:%08x", addr[0], *mem_ref[0]);
            byte_idx += ADDR_SIZE * 2;
            break;
        case And:
            LOG_DEBUG("And", NULL);
            deref_addrs(virt_mem, addr, mem_ref, 3);
            *mem_ref[2] = *mem_ref[0] && *mem_ref[1];
            byte_idx += ADDR_SIZE * 3;
            break;
        case Or:
            LOG_DEBUG("Or", NULL);
            deref_addrs(virt_mem, addr, mem_ref, 3);
            *mem_ref[2] = *mem_ref[0] || *mem_ref[1];
            byte_idx += ADDR_SIZE * 3;
            break;
        case Not:
            LOG_DEBUG("Not", NULL);
            deref_addrs(virt_mem, addr, mem_ref, 2);
            *mem_ref[1] = !*mem_ref[0];
            byte_idx += ADDR_SIZE * 2;
            break;
        case Mov:
            LOG_DEBUG("Mov", NULL);
            deref_addrs(virt_mem, addr, mem_ref, 1);
            *mem_ref[0] = addr[1];
            byte_idx += ADDR_SIZE * 2;
            break;
        case Copy:
            LOG_DEBUG("Copy", NULL);
            deref_addrs(virt_mem, addr, mem_ref, 2);
            *mem_ref[0] = *mem_ref[1];
            byte_idx += ADDR_SIZE * 2;
            break;
        case Goto:
            LOG_DEBUG("Goto", NULL);
            deref_addrs(virt_mem, addr, mem_ref, 1);
            byte_idx = *mem_ref[0];
            byte_idx--;
            break;
        case Geta:
            LOG_DEBUG("Geta", NULL);
            byte_idx += ADDR_SIZE;
            deref_addrs(virt_mem, addr, mem_ref, 1);
            *mem_ref[0] = byte_idx + 1;
            LOG_DEBUG("Mem[%08x]:%08x", addr[0], *mem_ref[0]);
            break;
        case Gt:
            LOG_DEBUG("Gt", NULL);
            deref_addrs(virt_mem, addr, mem_ref, 3);
            if (*mem_ref[0] > *mem_ref[1]) {
                byte_idx = *mem_ref[2] - 1;
                LOG_DEBUG("%08x > %08x = True", *mem_ref[0], *mem_ref[1]);
            } else {
                byte_idx += ADDR_SIZE * 3;
            }
            break;
        case Lt:
            LOG_DEBUG("Lt", NULL);
            deref_addrs(virt_mem, addr, mem_ref, 3);
            if (*mem_ref[0] < *mem_ref[1]) {
                byte_idx = *mem_ref[2] - 1;
                LOG_DEBUG("%08x < %08x = True", *mem_ref[0], *mem_ref[1]);
            } else {
                byte_idx += ADDR_SIZE * 3;
            }
            break;
        case Eq:
            LOG_DEBUG("Eq", NULL);
            deref_addrs(virt_mem, addr, mem_ref, 3);
            if (*mem_ref[0] == *mem_ref[1]) {
                byte_idx = *mem_ref[2] - 1;
                LOG_DEBUG("%08x == %08x = True", *mem_ref[0], *mem_ref[1]);
            } else {
                byte_idx += ADDR_SIZE * 3;
            }
            break;
        case Lm:
            LOG_DEBUG("Lm", NULL);
            deref_addrs(virt_mem, addr, mem_ref, 2);
            deref(virt_mem, *mem_ref[0] - *mem_ref[1], &mem_ref[2]);
            *mem_ref[2] = *mem_ref[0];
            *mem_ref[0] = Void;
            byte_idx += ADDR_SIZE * 2;
            break;
        case Rm:
            LOG_DEBUG("Rm", NULL);
            deref_addrs(virt_mem, addr, mem_ref, 2);
            deref(virt_mem, *mem_ref[0] + *mem_ref[1], &mem_ref[2]);
            *mem_ref[2] = *mem_ref[0];
            *mem_ref[0] = Void;
            byte_idx += ADDR_SIZE * 2;
            break;
        case Exit:
            LOG_DEBUG("Halted", NULL);
            state = Halt;
            return;
            break;
        case Sto:
            LOG_DEBUG("Sto", NULL);
            while (*read_virtmem(virt_mem, ++byte_idx) != Sto)
                ;
            break;
        case Putc:
            LOG_DEBUG("Putc", NULL);
            deref_addrs(virt_mem, addr, mem_ref, 1);
            deref(virt_mem, *mem_ref[0], &mem_ref[1]);
            putchar(*mem_ref[1]);
            byte_idx += ADDR_SIZE;
            break;
        case Putn:
            LOG_DEBUG("Putn", NULL);
            deref_addrs(virt_mem, addr, mem_ref, 1);
            deref(virt_mem, *mem_ref[0], &mem_ref[1]);
            printf("%d", *mem_ref[1]);
            byte_idx += ADDR_SIZE;
            break;
        case Puth:
            LOG_DEBUG("Puth", NULL);
            deref_addrs(virt_mem, addr, mem_ref, 1);
            deref(virt_mem, *mem_ref[0], &mem_ref[1]);
            printf("%x", *mem_ref[1]);
            byte_idx += ADDR_SIZE;
            break;
        case Getc:
            LOG_DEBUG("Getc", NULL);
            deref_addrs(virt_mem, addr, mem_ref, 1);
            *mem_ref[0] = getch();
            byte_idx += ADDR_SIZE;
            break;
        case Getn:
            LOG_DEBUG("Getn", NULL);
            deref_addrs(virt_mem, addr, mem_ref, 1);
            scanf("%d", mem_ref[0]);
            byte_idx += ADDR_SIZE;
            break;
        case Geth:
            LOG_DEBUG("Geth", NULL);
            deref_addrs(virt_mem, addr, mem_ref, 1);
            scanf("%x", mem_ref[0]);
            byte_idx += ADDR_SIZE;
            break;
        case BAnd:
            LOG_DEBUG("BAnd", NULL);
            deref_addrs(virt_mem, addr, mem_ref, 2);
            *mem_ref[0] = *mem_ref[0] & *mem_ref[1];
            byte_idx += ADDR_SIZE * 2;
            break;
        case BOr:
            LOG_DEBUG("BOr", NULL);
            deref_addrs(virt_mem, addr, mem_ref, 2);
            *mem_ref[0] = *mem_ref[0] | *mem_ref[1];
            byte_idx += ADDR_SIZE * 2;
            break;
        case XOr:
            LOG_DEBUG("XOr", NULL);
            deref_addrs(virt_mem, addr, mem_ref, 2);
            *mem_ref[0] = *mem_ref[0] ^ *mem_ref[1];
            byte_idx += ADDR_SIZE * 2;
            break;
        case BNot:
            LOG_DEBUG("BNot", NULL);
            deref_addrs(virt_mem, addr, mem_ref, 1);
            *mem_ref[0] = ~*mem_ref[0];
            byte_idx += ADDR_SIZE;
            break;
        case Shl:
            LOG_DEBUG("Shl", NULL);
            deref_addrs(virt_mem, addr, mem_ref, 2);
            *mem_ref[0] <<= *mem_ref[1];
            byte_idx += ADDR_SIZE * 2;
            break;
        case Shr:
            LOG_DEBUG("Shr", NULL);
            deref_addrs(virt_mem, addr, mem_ref, 2);
            *mem_ref[0] >>= *mem_ref[1];
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

void run_file(VirtMem *virt_mem, FILE *src_file, size_t cpu_clock) {
    size_t byte_idx;
    for (byte_idx = 0;
         byte_idx < virt_mem->max_size &&
         (*read_virtmem(virt_mem, byte_idx) = fgetc(src_file)) != EOF;
         byte_idx++)
        ;

    size_t src_size;
    int *first_byte = read_virtmem(virt_mem, 0);
    int *last_byte = read_virtmem(virt_mem, byte_idx);
    LOG_INFO("FirstByte: %02x", *first_byte);
    LOG_INFO("LastByte: %02x", *last_byte);
    if (*last_byte == EOF) {
        src_size = byte_idx;
        *last_byte = Void;
    } else {
        LOG_FATAL("Memory not enough to load the source file", NULL);
    }

    // DEBUG MEM CONTENT
    LOG_INFO("Memory content", NULL);
    log_mem(virt_mem, 0, src_size, INFO);

    execute_code(virt_mem, cpu_clock, src_size);
}