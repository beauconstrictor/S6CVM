#include <string.h>
#include <stdint.h>
#include <stdbool.h>

#include "assembler.h"

int hex_nibble(char c) {
  if (c >= '0' && c <= '9')
      return c - '0';
  if (c >= 'A' && c <= 'F')
      return c - 'A' + 10;
  if (c >= 'a' && c <= 'f')
      return c - 'a' + 10;
  return -1;
}

uint8_t read_hex(const char *s) {
  // we don't actually check it's valid hex because this it always is
  // as it comes directly from the assembler
  int val = hex_nibble(*s++) << 4;
  val |= hex_nibble(*s);
  return val; 
}

uint16_t read_hex_addr(const char *s) {
  // we don't actually check it's valid hex because this it always is
  // as it comes directly from the assembler
  int val = read_hex(s) << 8;
  s += 2;
  return val | read_hex(s);
}

typedef struct {
  uint16_t data[1024];
  size_t head;
} DStack;

bool dstack_push(DStack *dstack, uint16_t val) {
  if (dstack->head >= 1024) return false;
  dstack->data[dstack->head++] = val;
  return true;
}

bool dstack_pop(DStack *dstack, uint16_t *val) {
  if (dstack->head == 0) return false;
  *val = dstack->data[--dstack->head];
  return true;
}

const char *execute(DStack *dstack, uint16_t addr, uint8_t *memory,
    int memsize) {
  static char err_msg[256];

  #define POP(var) \
        if (!dstack_pop(dstack, &var)) return "stack underflow";
  #define PUSH(var) \
        if (!dstack_push(dstack, var)) return "stack overflow";

  #define BINARY_OPERATOR(opcode, operation) \
        case OPCODE_##opcode: { \
          uint16_t b; \
          POP(b); \
          uint16_t a; \
          POP(a); \
          PUSH(operation); \
        } break;

  #define CALL(addr) \
    do { \
        const char *error = execute(dstack, addr, memory, \
            memsize); \
        if (error) return error; \
    } while (false)


  for (;;) {
    if (addr >= memsize) return "segmentation fault";
    uint8_t opcode = memory[addr];

    printf("<%d (0x%04x) '%s", addr, addr, mnemonics[opcode]);

    if (opcode == OPCODE_PUSH)
      printf(" %d (0x%04x)", memory[addr+1], memory[addr+1]);
    printf("'>");

    if (dstack->head > 0)
      printf(" ~ ");
    for (size_t i = 0; i < dstack->head; i++) {
      printf("%d ", dstack->data[i]);
    }
    printf("\n");

    switch (opcode) {
      case OPCODE_I: {
        uint16_t subroutine;
        POP(subroutine);
        CALL(subroutine);
      } break;

      case OPCODE_PUSH: {
        if (addr >= memsize - 2) return "segmentation fault";
        uint16_t val = (memory[addr+1] << 8) | memory[addr + 2];
        addr += 2;
        PUSH(val);
      } break;

      case OPCODE_RET: {
        return NULL;
      } break;

      BINARY_OPERATOR(ADD, a + b)
      BINARY_OPERATOR(SUB, a - b)

      BINARY_OPERATOR(GT,  a > b)
      BINARY_OPERATOR(EQ,  a == b)

      case OPCODE_DUP: {
        uint16_t val;
        POP(val);
        PUSH(val);
        PUSH(val);
      } break;

      case OPCODE_DROP: {
        uint16_t val;
        POP(val);
      } break;

      case OPCODE_OVER: {
        uint16_t b;
        POP(b);
        uint16_t a;
        POP(a);
        PUSH(a);
        PUSH(b);
        PUSH(a);
      } break;

      case OPCODE_SWAP: {
        uint16_t b;
        POP(b);
        uint16_t a;
        POP(a);
        PUSH(b);
        PUSH(a);
      } break;

      case OPCODE_PUTI: {
        uint16_t val;
        POP(val);
        printf("%u\n", val);
      } break;

      case OPCODE_IF: {
        uint16_t false_block;
        POP(false_block);
        uint16_t true_block;
        POP(true_block);
        uint16_t condition;
        POP(condition);

        if (condition == 0)
          CALL(false_block);
        else
          CALL(true_block);
      } break;

      default: {
        snprintf(err_msg, sizeof(err_msg),
            "unsupported opcode: 0x%02x @ %d (0x%04x)", opcode,
            addr, addr);
        return err_msg;
      } break;
    }

    addr++;
  }
}

const char *simulate(const char *s) {
  int addr = -1;

  uint8_t memory[65536] = {0};

  DStack dstack = {0};

  printf("%s\n", s);

  while (*s) {
    char c = *s++;

    if (strchr("\n T", c)) continue;

    if (c == ';') {
      addr = -1;
    } else if (addr >= 0) {
      memory[addr++] = read_hex(s - 1);
      s += 1;
    } else if (c == ':') {
      addr = read_hex_addr(s);
      s += 4;
    } else if (c == '!') {
      const char *error = execute(&dstack, read_hex_addr(s), memory, sizeof(memory));
      if (error) return error;
    }
  }

  return NULL;
}
