#ifndef TOKENISER_H
#define TOKENISER_H

#include <stdio.h>
#include <stdlib.h>

#include "errors.h"

#define CONST_WORD "const"
#define VAR_WORD "var"

#define TOKEN_TYPES \
  X(DEFINITION) \
  X(LSQUARE) \
  X(RSQUARE) \
  X(NUMBER) \
  X(STRING) \
  X(LBRACE) \
  X(ASSIGN) \
  X(RBRACE) \
  X(LPARA) \
  X(DEREF) \
  X(CONST) \
  X(RPARA) \
  X(WORD) \
  X(BYTE) \
  X(VAR) \

#define X(name) TOKEN_##name,

typedef enum {
  TOKEN_TYPES
} TokenKind;

#undef X

typedef struct {
  TokenKind kind;
  const char *start;
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
bool lexer(const char *s, const char *path, Tokens *tokens, Error *error);
void delexer(Tokens *toks, FILE *f);

#endif // TOKENISER_H
