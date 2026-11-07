#ifndef TOKENISER_H
#define TOKENISER_H

#include <stdlib.h>

#include "errors.h"

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
  const char *path;
  size_t length;
  Location location;
} Token;

typedef struct {
  Token *items;
  size_t count;
  size_t capacity;
} Tokens;

Tokens create_tokens(void);

bool append_token(Token tok, Tokens *tokens);
bool insert_token(Tokens *tokens, size_t index, Token tok);
bool remove_token(Tokens *tokens, size_t index);

const char *token_kind_name(TokenKind kind);
int lexer(const char *s, const char *path, Tokens *tokens, Error *error);

#endif // TOKENISER_H
