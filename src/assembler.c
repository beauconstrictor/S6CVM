#include <stdint.h>
#include <assert.h>

#include "tokeniser.h"
#include "parser.h"

#define PC_MIN 0x0800
#define PC_MAX 0x3b7f

bool arrange_definitions(Symbols *syms, Tokens *toks, Error *error) {
  uint16_t pc = PC_MIN;

  for (size_t i = 0; i < toks->count; i++) {
    Token tok = toks->items[i];

    if (pc > PC_MAX) {
      if (i + 1 >= toks->count) {
        *error = (Error) {
          .location = tok.location,
          .message = "insufficient space for code allocation",
          .set = true,
        };
        return false;
      }
    }

    assert(tok.kind != TOKEN_LPARA);
    assert(tok.kind != TOKEN_RPARA);
    assert(tok.kind != TOKEN_LSQUARE);
    assert(tok.kind != TOKEN_RSQUARE);

    bool is_addr_of = false;

    switch (tok.kind) {
      // all eliminated by the preprocessor
      case TOKEN_LSQUARE:
      case TOKEN_RSQUARE:
      case TOKEN_LBRACE:
      case TOKEN_RBRACE:
      case TOKEN_STRING:
      case TOKEN_ASSIGN:
      case TOKEN_CONST:
      case TOKEN_LPARA:
      case TOKEN_RPARA:
      case TOKEN_VAR:
        break;

      case TOKEN_DEFINITION: {
        if (!append_token_symbol(syms, &tok, pc)) return false;
      } break;

      case TOKEN_NUMBER: {
        // 1. push opcode - one byte
        // 2. (16-bit) operand - two bytes
        pc += 3;
      } break;

      case TOKEN_WORD: {
        // 1. push opcode - one byte
        // 2. (16-bit) operand - two bytes
        pc += 3;
        if (!is_addr_of) {
          // 3. call opcode - one byte
          pc++;
          is_addr_of = false;
        }
      } break;

      case TOKEN_ADDR: {
        is_addr_of = true;
      } break;

      case TOKEN_BYTE: {
        // inserts RAW bytes, not to be interpreted as opcodes
        pc++;
      } break;
    }
  }

  return true;
}

bool assemble(Symbols *syms, Tokens *toks, Error *error) {
  return arrange_definitions(syms, toks, error);
}
