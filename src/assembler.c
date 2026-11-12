#include <stdint.h>
#include <assert.h>
#include <string.h>

#include "tokeniser.h"
#include "parser.h"

#include "assembler.h"

/*
 * TODO: don't emit unreferenced subroutines into the binary?
 * TODO: deduplicate subroutines with the same instructions
 */

#define PC_MIN 0x0800
#define PC_MAX 0x3b7f

#define OPCODE_CALL 0x00
#define OPCODE_PUSH 0x01
#define OPCODE_RET  0x02

#define ENTRY_POINT "main"

#define INSTRUCTIONS \
  X("i",       0x03) \
  X("if-else", 0x04) \
  X("while",   0x05) \
                     \
  X("add",     0x06) \
  X("sub",     0x07) \
                     \
  X("gt",      0x08) \
  X("sub",     0x09) \
                     \
  X("dup",     0x0a) \
  X("drop",    0x0b) \
  X("swap",    0x0c) \
  X("over",    0x0d) \
                     \
  X("puti",    0x0e) \

bool verify_syntax(Tokens *toks, Error *error) {
  static char buf[256];

  int nesting = 0;
  Token definition_tok;

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

        if (!has_next) 
          GIVE_ERROR("expected definition name after '{'", tok);
        else if (has_next && next.kind != TOKEN_DEFINITION)
          GIVE_ERROR("expected definition name after '{'", next);
      } break;

      case TOKEN_DEFINITION: {
        definition_tok = tok;
      } break;

      case TOKEN_RBRACE: {
        nesting--;
      } break;

      case TOKEN_ADDR: {
         if (!has_next)
           GIVE_ERROR_WITH_LEXEME("expected word after '%.*s'", tok);
         else if (has_next && next.kind != TOKEN_WORD)
           GIVE_ERROR_WITH_LEXEME("expected word after '%.*s'", next);
      } break;

      default: break;
    }

    if (nesting > 1)
      GIVE_ERROR("nested defintions are not allowed", tok);
  }

  if (nesting > 0)
    GIVE_ERROR("unterminated definition", definition_tok);

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

    bool is_addr_of = false;

    switch (tok.kind) {
      // all eliminated by the preprocessor
      case TOKEN_LSQUARE:
      case TOKEN_RSQUARE:
      case TOKEN_IMPORT:
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

int get_opcode(Token *tok) {
  char buf[256];
  snprintf(buf, sizeof(buf), "%.*s", (int)tok->length, tok->start);

  // prevent duplicate opcodes at compile time (i'm stupid, so i need
  // this)
  #define X(mnemonic, opcode) \
    [opcode] = mnemonic,
  const char *instrs[] = { INSTRUCTIONS };
  (void)instrs;
  #undef X

  #define X(mnemonic, opcode) \
      if (strcmp(buf, mnemonic) == 0) return opcode;
  INSTRUCTIONS
  #undef X

  return -1;
}

bool assemble(FILE *f, Symbols *syms, Tokens *toks, Error *error) {
  if (!arrange_definitions(syms, toks, error)) return false;

  #define EMIT_BYTE(byte) fprintf(f,"%02x", byte &   0xff);
  #define EMIT_WORD(word) fprintf(f,"%04x", word & 0xffff);

  static char buf[256];
  long pc;

  fprintf(f, "T\n");

  for (size_t i = 0; i < toks->count; i++) {
    Token tok = toks->items[i];

    Token next;
    bool has_next = i < toks->count - 1;
    if (has_next) next = toks->items[i+1];

    switch (tok.kind) {
      case TOKEN_DEFINITION: {
        if (!lookup_symbol(tok.start, tok.length, syms, &pc))
          GIVE_ERROR_WITH_LEXEME("undefined symbol: '%.*s'", tok);
        fprintf(f, ":%ld ", pc);
      } break;

      case TOKEN_RBRACE: {
        fprintf(f, "%02x ;\n", OPCODE_RET);
      } break;

      case TOKEN_BYTE: {
        long byte = read_number(&tok, error);
        if (error->set) return false;
        EMIT_BYTE((int)byte);
      } break;

      case TOKEN_ADDR: {
        long addr;
        if (!lookup_symbol(next.start, next.length, syms, &addr))
          GIVE_ERROR_WITH_LEXEME("undefined symbol: '%.*s'", tok);
        if (error->set) return false;
        EMIT_BYTE(OPCODE_PUSH);
        EMIT_WORD((int)addr);
        i++; // skip past the following word
      } break;

      case TOKEN_NUMBER: {
        long word = read_number(&tok, error);
        EMIT_BYTE(OPCODE_PUSH);
        EMIT_WORD((int)word);
      } break;

      case TOKEN_WORD: {
        int opcode = get_opcode(&tok);
        if (opcode >= 0) {
          EMIT_BYTE(opcode);
          break;
        }

        long addr;
        if (!lookup_symbol(tok.start, tok.length, syms, &addr))
          GIVE_ERROR_WITH_LEXEME("undefined symbol: '%.*s'", tok);
        if (error->set) return false;
        EMIT_BYTE(OPCODE_PUSH);
        EMIT_WORD((int)addr);
        EMIT_BYTE(OPCODE_CALL);
      } break;

      // TODO: add base words

      default: break;
    }
  }
  
  long main_addr;
  if (lookup_symbol(ENTRY_POINT, sizeof(ENTRY_POINT)-1, syms, &main_addr))
    fprintf(f, "!%04x", (unsigned)main_addr);

  fprintf(f, "\nT\n");

  #undef EMIT_BYTE
  #undef EMIT_WORD

  return true;
}
