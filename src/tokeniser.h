#ifndef TOKENISER_H
#define TOKENISER_H

#include <stdlib.h>

#define TOKEN_TYPES \
  X(NUMBER) \
  X(STRING) \
  X(DEFINITION) \
  X(LPARA) \
  X(RPARA) \
  X(LBRACE) \
  X(RBRACE) \
  X(WORD)

#define X(name) TOKEN_##name,

typedef enum {
  TOKEN_TYPES
} TokenKind;

#undef X


typedef struct {
  TokenKind kind;
  const char *start;
  size_t length;
  int line;
} Token;

typedef struct {
  Token *items;
  size_t count;
  size_t capacity;
} Tokens;

const char *token_kind_name(TokenKind kind);
Tokens lexer(const char *s);

#endif // TOKENISER_H
