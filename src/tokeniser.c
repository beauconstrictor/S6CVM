#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stdbool.h>

#include "tokeniser.h"

const char *token_kind_name(TokenKind kind) {
    switch (kind) {
        #define X(name) case TOKEN_##name: return #name;
        TOKEN_TYPES
        #undef X

        default: return "UNKNOWN";
    }
}

bool append_token(Token tok, Tokens *tokens) {
  if (tokens->count < tokens->capacity) {
    tokens->items[tokens->count++] = tok;
    return true;
  }

  size_t new_capa = tokens->capacity;
  new_capa = new_capa > 0 ? new_capa*2 : 1;
  Token *new = realloc(tokens->items, sizeof(Token) * new_capa);
  if (new == NULL) return false;

  tokens->items = new;
  tokens->capacity = new_capa;
  tokens->items[tokens->count++] = tok;

  return true;
}

Tokens create_tokens(void) {
  return (Tokens) {
    .items = NULL,
    .count = 0,
    .capacity = 0,
  };
}

Tokens lexer(const char *s) {
  Tokens toks = create_tokens();
  
  const char *start = s;
  int    line = 0;

  #define FLUSH_TOK(type) \
    do { \
      if (start < s)  { \
        Token tok = (Token) { \
          .kind = type, \
          .start = start, \
          .length = s - start, \
          .line = line, \
        }; \
        append_token(tok, &toks); \
      } \
      start = s; \
    } while (false)
  #define FLUSH_WITH_BRACK(type) \
    do { \
      if (start < s) FLUSH_TOK(TOKEN_WORD); \
      Token tok = (Token) { \
        .kind = TOKEN_##type, \
        .start = s, \
        .length = 1, \
        .line = line, \
      }; \
      start = s + 1; \
      append_token(tok, &toks); \
    } while (false)

  #define EXHAUST(func) \
    do { \
      while (func(*++s)) {} \
    } while (false)

  #define EXHAUST_CHRS(chrs) \
    do { \
      while (strchr(chrs, *++s)) {} \
    } while (false)
  
  #define EXHAUST_NCHRS(chrs) \
    do { \
      while (!strchr(chrs, *++s)) {} \
    } while (false)

  int comment_level = 0;

  while (*s) {
    if (*s == '<') {
      if (comment_level == 0) FLUSH_TOK(TOKEN_WORD);
      comment_level++;
    }
    if (*s == '>') {
      assert(comment_level > 0); // '>' encountered outside of comment. TODO: actual error system
      comment_level--;
    }

    if (*s == '\n') line++;

    if (comment_level > 0 || *s == '>') {
      s++;
      start = s;
      continue;
    };

    if (*s == '{') FLUSH_WITH_BRACK(LBRACE);
    if (*s == '}') FLUSH_WITH_BRACK(RBRACE);
    if (*s == '(') FLUSH_WITH_BRACK(LPARA);
    if (*s == ')') FLUSH_WITH_BRACK(RPARA);

    if (isspace(*s)) {
      FLUSH_TOK(TOKEN_WORD);
      start = s + 1;
    } else if ((isdigit(*s) || *s == '-') && start == s) {
      EXHAUST(isdigit);
      FLUSH_TOK(TOKEN_NUMBER);
      continue;
    } else if (*s == ':' && start == s) {
      EXHAUST(!isspace);
      start++;
      FLUSH_TOK(TOKEN_DEFINITION);
      continue;
    } else if (*s == '"' && start == s) {
      EXHAUST_NCHRS("\"");
      start++;
      FLUSH_TOK(TOKEN_STRING);
      start++; // move past final quote
      s++;
      continue;
    }

    s++;
  }

  if (start < s) FLUSH_TOK(TOKEN_WORD);

  return toks;
}

