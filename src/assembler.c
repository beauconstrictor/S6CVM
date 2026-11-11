#include <stdint.h>
#include <assert.h>

#include "tokeniser.h"
#include "parser.h"

#define PC_MIN 0x0800
#define PC_MAX 0x3b7f

#define GIVE_ERROR(msg, tok) \
  do { \
    *error = (Error) { \
      .location = tok.location, \
      .message = msg, \
      .set = true, \
    }; \
    return false; \
  } while (false) \

#define GIVE_ERROR_WITH_LEXEME(msg, tok) \
  do { \
    snprintf(buf, sizeof(buf), msg, (int)tok.length, tok.start); \
    *error = (Error) { \
      .location = tok.location, \
      .message = buf, \
      .set = true, \
    }; \
    return false; \
  } while (false) \

bool verify_syntax(Tokens *toks, Error *error) {
  static char buf[256];

  int nesting = 0;

  for (size_t i = 0; i < toks->count; i++) {
    Token tok = toks->items[i];

    Token next;
    bool has_next = i < toks->count - 1;
    if (has_next) next = toks->items[i+1];

    if (tok.kind != TOKEN_LBRACE && nesting < 1)
      GIVE_ERROR_WITH_LEXEME("unexpected '%.*s' outside of definition", tok);

    switch (tok.kind) {
      case TOKEN_LSQUARE:
      case TOKEN_RSQUARE:
      case TOKEN_STRING:
      case TOKEN_ASSIGN:
      case TOKEN_CONST:
      case TOKEN_LPARA:
      case TOKEN_RPARA:
      case TOKEN_VAR: {
        GIVE_ERROR_WITH_LEXEME("unexpected '%.*s'", tok);
      } break;

      case TOKEN_LBRACE: {
        nesting++;

        if (has_next && next.kind != TOKEN_DEFINITION)
          GIVE_ERROR("expected definition name after '{'", next);
        else if (!has_next) 
          GIVE_ERROR("expected definition name after '{'", tok);
      } break;

      case TOKEN_RBRACE: {
        nesting--;
      } break;

      default: break;
    }

    if (nesting > 1)
      GIVE_ERROR("nested defintions are not allowed", tok);
  }

  return true;
}

bool arrange_definitions(Symbols *syms, Tokens *toks, Error *error) {
  uint16_t pc = PC_MIN;

  for (size_t i = 0; i < toks->count; i++) {
    Token tok = toks->items[i];

    if (pc > PC_MAX) {
      if (i + 1 >= toks->count) {
        GIVE_ERROR("insufficient space for code allocation", tok);
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
  if (!verify_syntax(toks, error)) return false;
  if (!arrange_definitions(syms, toks, error)) return false;

  return true;
}
