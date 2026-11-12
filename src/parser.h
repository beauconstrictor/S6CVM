#ifndef PARSER_H
#define PARSER_H

#include <stdlib.h>

#include "tokeniser.h"

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

typedef struct {
  const char *name;
  size_t length;
  long value;
} Symbol;

typedef struct {
  Symbol *items;
  size_t count;
  size_t capacity;
} Symbols;


bool append_symbol(Symbol sym, Symbols *syms);
// uses the token's lexeme as the name
bool append_token_symbol(Symbols *syms, Token *tok, long value);
bool lookup_symbol(const char *s, size_t length, Symbols *syms, long *value);

long read_number(const Token *tok, Error *error);

char *read_file(const char *path);

#endif // PARSER_H
