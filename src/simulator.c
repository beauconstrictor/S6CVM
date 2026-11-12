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

const char *execute(DStack *dstack, uint16_t addr, uint8_t *memory, int memsize) {
  for (;;) {
    if (addr >= memsize) return "segmentation fault";
    uint8_t opcode = memory[addr++];

    printf("%02x, %02x\n", opcode, OPCODE_PUSH);

    switch (opcode) {
      case OPCODE_PUSH: {
        if (addr >= memsize - 2) return "segmentation fault";
        uint16_t opcode = (memory[addr] << 8) | memory[addr + 1];
        addr += 2;
        if (!dstack_push(dstack, opcode)) return "stack overflow";
      } break;

      default: {
        return "unsupported opcode";
      } break;
    }
  }

  return NULL;
}

const char *simulate(const char *s) {
  int addr = -1;

  uint8_t memory[65536] = {0xff};

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
