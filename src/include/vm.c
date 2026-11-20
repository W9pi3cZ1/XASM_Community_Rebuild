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
  virt_mem->chunk_size = 0x100;
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
  memset(chunk->mem, Void, virt_mem->chunk_size * sizeof(int));
  chunk->next_chunk = virt_mem->head;
  virt_mem->head = chunk;
  return virt_mem;
}

void debug_indent(long indent, FILE *debug_file) {
  for (long i = 0; i < indent; i++) {
    fprintf(debug_file, "    ");
  }
}

void iprintf(long indent, FILE *debug_file, const char *format, ...) {
  debug_indent(indent, debug_file);
  va_list args;
  va_start(args, format);
  vprintf(format, args);
  va_end(args);
}

void debug_byte(int byte, FILE *debug_file) {
  switch (byte) {
  case Add:
    fprintf(debug_file, "Add");
    break;
  case Sub:
    fprintf(debug_file, "Sub");
    break;
  case XAdd:
    fprintf(debug_file, "XAdd");
    break;
  case XSub:
    fprintf(debug_file, "XSub");
    break;
  case And:
    fprintf(debug_file, "And");
    break;
  case Or:
    fprintf(debug_file, "Or");
    break;
  case Not:
    fprintf(debug_file, "Not");
    break;
  case Mov:
    fprintf(debug_file, "Mov");
    break;
  case Copy:
    fprintf(debug_file, "Copy");
    break;
  case Goto:
    fprintf(debug_file, "Goto");
    break;
  case Geta:
    fprintf(debug_file, "Geta");
    break;
  case Gt:
    fprintf(debug_file, "Gt");
    break;
  case Lt:
    fprintf(debug_file, "Lt");
    break;
  case Eq:
    fprintf(debug_file, "Eq");
    break;
  case Lm:
    fprintf(debug_file, "Lm");
    break;
  case Rm:
    fprintf(debug_file, "Rm");
    break;
  case Exit:
    fprintf(debug_file, "Exit");
    break;
  case Sto:
    fprintf(debug_file, "Sto");
    break;
  case Putc:
    fprintf(debug_file, "Putc");
    break;
  case Putn:
    fprintf(debug_file, "Putn");
    break;
  case Puth:
    fprintf(debug_file, "Puth");
    break;
  case Getc:
    fprintf(debug_file, "Getc");
    break;
  case Getn:
    fprintf(debug_file, "Getn");
    break;
  case Geth:
    fprintf(debug_file, "Geth");
    break;
  case BAnd:
    fprintf(debug_file, "BAnd");
    break;
  case BOr:
    fprintf(debug_file, "BOr");
    break;
  case XOr:
    fprintf(debug_file, "XOr");
    break;
  case BNot:
    fprintf(debug_file, "BNot");
    break;
  case Shl:
    fprintf(debug_file, "Shl");
    break;
  case Shr:
    fprintf(debug_file, "Shr");
    break;
  default:
    fprintf(debug_file, "%04x", byte);
  }
}

void debug_chunk(long indent, MemChunk *chunk, FILE *debug_file) {
  iprintf(0, debug_file, "Chunk {\n");
  indent++;
  iprintf(indent, debug_file, "start_addr: %zx,\n", chunk->start_addr);
  iprintf(indent, debug_file, "end_addr: %zx,\n", chunk->end_addr);
  iprintf(indent, debug_file, "mem: [");
  indent++;
  for (size_t i = 0; i < chunk->end_addr - chunk->start_addr; i++) {
    if (i % 0x10 == 0) {
      iprintf(0, debug_file, "\n");
      iprintf(indent, debug_file, "[0x%08zx] = ", i + chunk->start_addr,
              chunk->mem[i]);
    }
    // debug_byte(chunk->mem[i], debug_file);
    fprintf(debug_file, "%04x, ", chunk->mem[i]);
  }
  iprintf(0, debug_file, "\n");
  indent--;
  iprintf(indent, debug_file, "]\n");
  indent--;
  iprintf(indent, debug_file, "}");
}

void debug_virt_mem(VirtMem *virt_mem, FILE *debug_file) {
  MemChunk *chunk = virt_mem->head;
  long cnt = 0;
  long indent = 0;
  iprintf(indent, debug_file, "VirtMem {\n");
  indent++;
  while (chunk != NULL) {
    iprintf(indent, debug_file, "[%ld] = ", cnt);
    debug_chunk(indent, chunk, debug_file);
    iprintf(0, debug_file, ",\n");
    cnt++;
    chunk = chunk->next_chunk;
  }
  indent--;
  iprintf(indent, debug_file, "}\n");
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
  MemChunk *chunk = get_chunk(virt_mem, src_addr);
  size_t chunk_offset = src_addr - chunk->start_addr;
  for (size_t idx = 0; idx < n; idx++) {
    // Overflow chunk
    if (idx + chunk_offset > chunk->end_addr - chunk->start_addr) {
      chunk = get_chunk(virt_mem, chunk->end_addr + 1);
      // Recalculate chunk_offset
      chunk_offset = -idx;
    }
    dest[idx] = chunk->mem[chunk_offset + idx];
  }
};

size_t merge_addr(int *mem_ptr) {
  // LOG_DEBUG("Bytes[0]:%02x", mem_ptr[0]);
  // LOG_DEBUG("Bytes[3]:%02x", mem_ptr[3]);
  size_t addr =
      (mem_ptr[0] << 24) + (mem_ptr[1] << 16) + (mem_ptr[2] << 8) + mem_ptr[3];
  return addr;
}

void log_mem(VirtMem *virt_mem, size_t begin, size_t end, LogLevel lvl) {
  size_t idx;
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

State state = Run;

void halt() {
  LOG_DEBUG("Halted", NULL);
  state = Halt;
  return;
}

#if defined(__linux__) || defined(__APPLE__)
#include <signal.h>
void halt_handler(int signum) {
  LOG_DEBUG("Halt signal received: %d", signum);
  halt();
}

void set_sig_handler() {
  struct sigaction mact;
  mact.sa_flags = 0;
  sigemptyset(&mact.sa_mask);
  sigaddset(&mact.sa_mask, SIGQUIT);
  mact.sa_handler = halt_handler;
  sigaction(SIGINT, &mact, NULL);
}
#endif

void execute_code(VirtMem *virt_mem, size_t cpu_clock, size_t src_size) {
  (void)src_size;
  size_t byte_idx = 0;
  LOG_DEBUG("CPU Clock:%zu", cpu_clock);
  size_t microsec_per_cmd = cpu_clock == 0 ? 0 : 1000000 / cpu_clock;
  const size_t ADDR_BUF_SIZE = 4;
  const size_t DATA_BUF_SIZE = ADDR_BUF_SIZE * 4 + 1; // 1 for cmd
  int data[DATA_BUF_SIZE];
  int *mem_ref[DATA_BUF_SIZE]; // Memory chunk data reference
  size_t addr[ADDR_BUF_SIZE];
  size_t cmd_cnt = 0;
  int is_jumped = 0;
  while (byte_idx < virt_mem->max_size && state != Halt) {
    ++cmd_cnt;
    if (cpu_clock != 0) {
      usleep(microsec_per_cmd);
    };
    memset(data, Void, DATA_BUF_SIZE * sizeof(int));
    virtmem_cpy(virt_mem, data, byte_idx, DATA_BUF_SIZE);
    logger_feat(1, 0, DEBUG, "Addrs: ");
    for (size_t i = 0; i < ADDR_BUF_SIZE; i++) {
      addr[i] = merge_addr(&data[1 + i * 4]);
      logger_feat(1, 1, DEBUG, "[%d]=0x%08x ", i, addr[i]);
    }
    logger_feat(1, 1, DEBUG, "\n");
    is_jumped = 0;
    // Auto Deref
    switch (data[0]) {
    case Mov:
    case Goto:
    case Geta:
    case Putc:
    case Putn:
    case Puth:
    case Getc:
    case Getn:
    case Geth:
    case BNot:
      deref_addrs(virt_mem, addr, mem_ref, 1);
      break;
    case Add:
    case Sub:
    case XAdd:
    case XSub:
    case Copy:
    case Lm:
    case Rm:
    case BAnd:
    case BOr:
    case XOr:
    case Shl:
    case Shr:
      deref_addrs(virt_mem, addr, mem_ref, 2);
      break;
    case And:
    case Or:
    case Not:
    case Gt:
    case Lt:
    case Eq:
      deref_addrs(virt_mem, addr, mem_ref, 3);
      break;
    }

    switch (data[0]) {
    case Add:
      LOG_DEBUG("Add", NULL);
      *mem_ref[0] += *mem_ref[1];
      LOG_DEBUG("Res: %02x", *mem_ref[0]);
      break;
    case Sub:
      LOG_DEBUG("Sub", NULL);
      *mem_ref[0] -= *mem_ref[1];
      break;
    case XAdd:
      LOG_DEBUG("XAdd", NULL);
      *mem_ref[0] *= *mem_ref[1];
      break;
    case XSub:
      LOG_DEBUG("XSub", NULL);
      *mem_ref[0] /= *mem_ref[1];
      break;
    case And:
      LOG_DEBUG("And", NULL);
      *mem_ref[2] = *mem_ref[0] && *mem_ref[1];
      break;
    case Or:
      LOG_DEBUG("Or", NULL);
      *mem_ref[2] = *mem_ref[0] || *mem_ref[1];
      break;
    case Not:
      LOG_DEBUG("Not", NULL);
      /* Original XASM1: dest = (a != b), three 4-byte operands. */
      *mem_ref[2] = *mem_ref[0] != *mem_ref[1];
      break;
    case Mov:
      LOG_DEBUG("Mov", NULL);
      *mem_ref[0] = addr[1];
      LOG_DEBUG("Res: %02x", *mem_ref[0]);
      break;
    case Copy:
      LOG_DEBUG("Copy", NULL);
      *mem_ref[0] = *mem_ref[1];
      LOG_DEBUG("Res: %02x", *mem_ref[0]);
      break;
    case Goto:
      LOG_DEBUG("Goto", NULL);
      byte_idx = *mem_ref[0];
      byte_idx--;
      break;
    case Geta:
      LOG_DEBUG("Geta", NULL);
      *mem_ref[0] = byte_idx + ADDR_SIZE + 1;
      break;
    case Gt:
      LOG_DEBUG("Gt", NULL);
      is_jumped = *mem_ref[0] > *mem_ref[1];
      break;
    case Lt:
      LOG_DEBUG("Lt", NULL);
      is_jumped = *mem_ref[0] < *mem_ref[1];
      break;
    case Eq:
      LOG_DEBUG("Eq", NULL);
      is_jumped = *mem_ref[0] == *mem_ref[1];
      break;
    case Lm:
      LOG_DEBUG("Lm", NULL);
      /* Original XASM1 uses the address of operand 1, not its value. */
      deref(virt_mem, addr[0] - *mem_ref[1], &mem_ref[2]);
      *mem_ref[2] = *mem_ref[0];
      *mem_ref[0] = Void;
      break;
    case Rm:
      LOG_DEBUG("Rm", NULL);
      /* Original XASM1 uses the address of operand 1, not its value. */
      deref(virt_mem, addr[0] + *mem_ref[1], &mem_ref[2]);
      *mem_ref[2] = *mem_ref[0];
      *mem_ref[0] = Void;
      break;
    case Exit:
      halt();
      break;
    case Sto:
      LOG_DEBUG("Sto", NULL);
      while (*read_virtmem(virt_mem, ++byte_idx) != Sto)
        ;
      break;
    case Putc:
      LOG_DEBUG("Putc", NULL);
      deref(virt_mem, *mem_ref[0], &mem_ref[1]);
      putchar(*mem_ref[1]);
      break;
    case Putn:
      LOG_DEBUG("Putn", NULL);
      deref(virt_mem, *mem_ref[0], &mem_ref[1]);
      printf("%d", *mem_ref[1]);
      break;
    case Puth:
      LOG_DEBUG("Puth", NULL);
      deref(virt_mem, *mem_ref[0], &mem_ref[1]);
      printf("%x", *mem_ref[1]);
      break;
    case Getc:
      LOG_DEBUG("Getc", NULL);
      *mem_ref[0] = getch();
      break;
    case Getn:
      LOG_DEBUG("Getn", NULL);
      scanf("%d", mem_ref[0]);
      break;
    case Geth:
      LOG_DEBUG("Geth", NULL);
      scanf("%x", mem_ref[0]);
      break;
    case BAnd:
      LOG_DEBUG("BAnd", NULL);
      *mem_ref[0] = *mem_ref[0] & *mem_ref[1];
      break;
    case BOr:
      LOG_DEBUG("BOr", NULL);
      *mem_ref[0] = *mem_ref[0] | *mem_ref[1];
      break;
    case XOr:
      LOG_DEBUG("XOr", NULL);
      *mem_ref[0] = *mem_ref[0] ^ *mem_ref[1];
      break;
    case BNot:
      LOG_DEBUG("BNot", NULL);
      *mem_ref[0] = ~*mem_ref[0];
      break;
    case Shl:
      LOG_DEBUG("Shl", NULL);
      *mem_ref[0] <<= *mem_ref[1];
      break;
    case Shr:
      LOG_DEBUG("Shr", NULL);
      *mem_ref[0] >>= *mem_ref[1];
      break;
    case Void:
      LOG_WARN("Segfault", NULL);
      state = Halt;
      return;
    default:
      LOG_FATAL("Unknown ByteCode `%02x`", data[0], NULL);
      break;
    }
    // Auto move byte_idx
    // Auto Deref
    switch (data[0]) {
    case Geta:
    case Putc:
    case Putn:
    case Puth:
    case Getc:
    case Getn:
    case Geth:
    case BNot:
      byte_idx += ADDR_SIZE * 1;
      break;
    case Add:
    case Sub:
    case XAdd:
    case XSub:
    case Mov:
    case Copy:
    case Lm:
    case Rm:
    case BAnd:
    case BOr:
    case XOr:
    case Shl:
    case Shr:
      byte_idx += ADDR_SIZE * 2;
      break;
    case And:
    case Or:
    case Not:
      byte_idx += ADDR_SIZE * 3;
      break;
    case Gt:
    case Lt:
    case Eq:
      if (is_jumped) {
        LOG_DEBUG("Jumped to 0x%08x", *mem_ref[2]);
        byte_idx = *mem_ref[2];
        byte_idx--;
      } else {
        byte_idx += ADDR_SIZE * 3;
      }
      break;
    default:
      break;
    }
    fflush(stdout);
    ++byte_idx;
  }
  logger_feat(1, 1, INFO, "\n");
  LOG_INFO("Execute end.", NULL);
  LOG_INFO("Executed %d commands", cmd_cnt);
  debug_virt_mem(virt_mem, stdout);
}

void run_file(VirtMem *virt_mem, FILE *src_file, size_t cpu_clock) {
  size_t byte_idx;
  for (byte_idx = 0;
       byte_idx < virt_mem->max_size &&
       (*read_virtmem(virt_mem, byte_idx) = fgetc(src_file)) != EOF;
       byte_idx++)
    ;

  size_t src_size = byte_idx;
  int *first_byte = read_virtmem(virt_mem, 0);
  int *last_byte = read_virtmem(virt_mem, byte_idx);
  LOG_INFO("FirstByte: %02x", *first_byte);
  LOG_INFO("LastByte: %02x", *last_byte);
  if (*last_byte != EOF) {
    LOG_FATAL("Memory not enough to load the source file", NULL);
  }
  *last_byte = Void;

  // DEBUG MEM CONTENT
  LOG_INFO("Memory content", NULL);
  log_mem(virt_mem, 0, src_size, INFO);

  execute_code(virt_mem, cpu_clock, src_size);
}
