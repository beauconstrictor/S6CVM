#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stdbool.h>

#include "errors.h"

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

bool insert_token(Tokens *tokens, size_t index, Token tok) {
  if (index > tokens->count)
    return false;

if (tokens->count == tokens->capacity) {
    size_t new_capa = tokens->capacity ? tokens->capacity * 2 : 1;
    Token *new = realloc(tokens->items, sizeof(Token) * new_capa);
    if (!new) return false;

    tokens->items = new;
    tokens->capacity = new_capa;
  }

  memmove(&tokens->items[index + 1], &tokens->items[index],
    (tokens->count - index) * sizeof(Token));

  tokens->items[index] = tok;
  tokens->count++;

  return true;
}

bool remove_token(Tokens *tokens, size_t index) {
  if (index >= tokens->count)
    return false;

  memmove(&tokens->items[index], &tokens->items[index + 1],
    (tokens->count - index - 1) * sizeof(Token));

  tokens->count--;
  return true;
}

Tokens create_tokens(void) {
  return (Tokens) {
    .items = NULL,
    .count = 0,
    .capacity = 0,
  };
}

int lexer(const char *s, const char *path, Tokens *toks, Error *error) {
  const char *start = s;
  const char *linestart = s;
  int    line = 1;

  #define FLUSH_TOK(type) \
    do { \
      if (start < s)  { \
        Token tok = (Token) { \
          .kind = type, \
          .start = start, \
          .length = s - start, \
          .location = { \
            .path = path, \
            .line = line, \
            .col = start - linestart + 1, \
          } \
        }; \
        if (!append_token(tok, toks)) return 1; \
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
          .location = { \
            .path = path, \
            .line = line, \
            .col = s- linestart + 1, \
          } \
        }; \
      start = s + 1; \
      if (!append_token(tok, toks)) return 1; \
    } while (false)

  #define EXHAUST(func) \
    do { \
      while (func((unsigned char)*++s)) {} \
    } while (false)

  #define EXHAUST_UNTIL(chrs) \
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
      if (comment_level == 0) {
        *error = (Error) {
          .location = {
            .line = line,
            .col = s - linestart + 1,
            .path = path,
          },
          .message = "'>' encountered outside of comment",
          .set = true,
        };
        return 1;
      }
      comment_level--;
    }

    if (*s == '\n') {
      line++;
      linestart = s + 1;
    };

    if (comment_level > 0 || *s == '>') {
      s++;
      start = s;
      continue;
    };

    if (*s == '{') FLUSH_WITH_BRACK(LBRACE);
    if (*s == '}') FLUSH_WITH_BRACK(RBRACE);
    if (*s == '(') FLUSH_WITH_BRACK(LPARA);
    if (*s == ')') FLUSH_WITH_BRACK(RPARA);

    if (isspace((unsigned char)*s)) {
      FLUSH_TOK(TOKEN_WORD);
      start = s + 1;
    } else if ((isdigit((unsigned char)*s)
        || (*s == '-' && isdigit((unsigned char)s[1])))
        && start == s) {
      EXHAUST(isdigit);
      FLUSH_TOK(TOKEN_NUMBER);
      continue;
    } else if (*s == ':' && start == s) {
      EXHAUST(!isspace);
      start++;
      FLUSH_TOK(TOKEN_DEFINITION);
      continue;
    } else if (*s == '"' && start == s) {
      // TODO: these strings should probably be their own helper
      //       function

      while (true) {
        s++;

        if (*s == '\n') {
          line++;
          linestart = s;
        }

        if (s[-1] != '\\' && *s == '"') break;
        if (*s == '\0') {
          *error = (Error) {
            .location = {
              .line = line,
              .col = s - linestart + 1,
              .path = path,
            },
            .message = "unterminated string literal",
            .set = true,
          };
          return 1;
        }
      }

      start++;
      FLUSH_TOK(TOKEN_STRING);
      start++; // move past final quote
      s++;
      continue;
    }

    s++;
  }

  if (comment_level) {
    *error = (Error) {
      .location = {
        .line = line,
        .col = s - linestart + 1,
        .path = path,
      },
      .message = "unterminated comment",
      .set = true,
    };
    return 1;
  }

  if (start < s) FLUSH_TOK(TOKEN_WORD);

  #undef FLUSH_TOK
  #undef FLUSH_WITH_BRACK
  #undef EXHAUST
  #undef EXHAUST_UNTIL

  return 0;
}

