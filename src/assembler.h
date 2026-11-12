#ifndef ASSEMBLER_H
#define ASSEMBLER_H

#include <stdio.h>

#include "errors.h"
#include "parser.h"
#include "tokeniser.h"

#define OPCODE_I    0x00
#define OPCODE_PUSH 0x01
#define OPCODE_RET  0x02

#define OPCODE_IF   0x03
#define OPCODE_WHIL 0x04

#define OPCODE_ADD  0x05
#define OPCODE_SUB  0x06

#define OPCODE_GT   0x07

#define OPCODE_DUP  0x09
#define OPCODE_DROP 0x0a
#define OPCODE_SWAP 0x0b
#define OPCODE_OVER 0x0c

#define INSTRUCTIONS        \
  X("i",       OPCODE_I)    \
  X("if-else", OPCODE_IF)   \
  X("while",   OPCODE_WHIL) \
                            \
  X("add",     OPCODE_ADD)  \
  X("sub",     OPCODE_SUB)  \
                            \
  X("gt",      OPCODE_GT)   \
                            \
  X("dup",     OPCODE_DUP)  \
  X("drop",    OPCODE_DROP) \
  X("swap",    OPCODE_SWAP) \
  X("over",    OPCODE_OVER) \

bool verify_syntax(Tokens *toks, Error *error);
// make sure to verify syntax before calling!
bool assemble(FILE *f, Symbols *syms, Tokens *toks, Error *error);

#endif // ASSEMBLER_H
