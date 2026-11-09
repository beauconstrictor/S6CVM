// 1. deparenthesisation
//   -> keep a stack of the first word after parentheses, then
//      move it to the closing paren
//
// 2. destringifying
//   -> convert string literals to anonymous blocks
//
// 3. desymbolification
//   -> replace constants with their values
//
// 4. deanonymification
//   -> move anonymous blocks to global named blocks, with internal
//      names like ___anon_0

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#include "tokeniser.h"
#include "errors.h"

#define min(a, b) ((a) < (b) ? (a) : (b))

bool deparenthesise(Tokens *toks, Error *error) {
  Tokens prefix_words = create_tokens();

  for (size_t i = 0; i < toks->count; i++) {
    Token tok = toks->items[i];

    if (tok.kind == TOKEN_LPARA) {
      if (i + 1 >= toks->count) {
        *error = (Error) {
          .location = tok.location,
          .message = "expected word after '('",
          .set = true,
        };
        return false;
      }

      append_token(toks->items[i+1], &prefix_words);
      remove_token(toks, i);
      remove_token(toks, i);
      i -= 2;
    } else if (tok.kind == TOKEN_RPARA) {
      remove_token(toks, i--);
      if (prefix_words.count == 0) {
        *error = (Error) {
          .location = tok.location,
          .message = "unexpected ')'",
          .set = true,
        };
        return false;
      }
      Token word = prefix_words.items[prefix_words.count - 1];
      if (word.kind != TOKEN_WORD) {
        *error = (Error) {
          .location = word.location,
          .message = "expected word after '('",
          .set = true,
        };
        return false;
      }
      remove_token(&prefix_words, prefix_words.count - 1);
      insert_token(toks, i+1, word);
      i++;
    }
  }

  if (prefix_words.count > 0) {
      *error = (Error) {
        .location = prefix_words.items[prefix_words.count - 1].location,
        .message = "unterminated parenthesis",
        .set = true,
      };
      return false;
  }

  return true;
}

bool destringify(Tokens *toks, Error *error) {
  (void) error; // doesn't actually produce any errors yet, but will

  for (size_t i = 0; i < toks->count; i++) {
    Token tok = toks->items[i];
    if (tok.kind != TOKEN_STRING) continue;
  
    insert_token(toks, i++, (Token) {
        .kind = TOKEN_LBRACE,
        .start = "{",
        .length = 1,
        .location = tok.location,
    });

    remove_token(toks, i--);

    // TODO: this leaks memory if tokens are removed. that's not
    // really a problem though for a short-lived compiler like this,
    // so i think it's fine. maybe revisit?

    const char *s = tok.start;
    while (s < tok.start + tok.length) {
      char *num = malloc(4);
      if (num == NULL) return false;
      int len = snprintf(num, 4, "%d", *s);

      insert_token(toks, ++i, (Token) {
          .kind = TOKEN_BYTE,
          .start = num,
          .length = len,
          .location = tok.location,
      });
      s++;
    }

    insert_token(toks, ++i, (Token) {
        .kind = TOKEN_RBRACE,
        .start = "}",
        .length = 1,
        .location = tok.location,
    } );
  }

  return true;
}

struct Symbol {
  const char *name;
  size_t length;
  long value;
};

struct Symbols {
  struct Symbol *items;
  size_t count;
  size_t capacity;
};

bool append_symbol(struct Symbol sym, struct Symbols *syms) {
  if (syms->count < syms->capacity) {
    syms->items[syms->count++] = sym;
    return true;
  }

  size_t new_capa = syms->capacity;
  new_capa = new_capa > 0 ? new_capa*2 : 1;
  struct Symbol *new = realloc(syms->items, sizeof(struct Symbol) * new_capa);
  if (new == NULL) return false;

  syms->items = new;
  syms->capacity = new_capa;
  syms->items[syms->count++] = sym;

  return true;
}

bool lookup_symbol(const char *s, size_t length, struct Symbols *syms, long *value) {
  for (size_t i = 0; i < syms->count; i++) {
    struct Symbol sym = syms->items[i];

    if (length != sym.length) continue;
    if (strncmp(s, sym.name, length) != 0) continue;

    *value = sym.value;
    return true;
  }

  return false;
}

bool desymbolise(Tokens *toks, Error *error) {
  struct Symbols syms = {0};

  char buf[256];

  for (size_t i = 0; i < toks->count; i++) {
    Token tok = toks->items[i];
    if (tok.kind != TOKEN_CONST) continue;

    bool has_assignment = true;

    if (toks->count < i + 4) has_assignment = false;
    if (toks->items[i+1].kind != TOKEN_WORD) has_assignment = false;
    if (toks->items[i+2].kind != TOKEN_ASSIGN) has_assignment = false;
    // TODO: you can't currently assign in terms of OTHER symbols
    if (toks->items[i+3].kind != TOKEN_NUMBER) has_assignment = false;

    if (!has_assignment) {
      *error = (Error) {
        .location = tok.location,
        .message = "expected assignment after " CONST_WORD,
        .set = true,
      };
      return false;
    }

    Token lval  = toks->items[i+1];
    Token rval = toks->items[i+3];

    snprintf(buf, sizeof(buf), "%.*s", (int)rval.length, rval.start);
    char *end;
    long value = strtol(rval.start, &end, 10);

    if (end != rval.start + rval.length) {
      *error = (Error) {
        .location = rval.location,
        .message = "expected a number after assignment" CONST_WORD,
        .set = true,
      };
      return false;
    }

    struct Symbol sym = (struct Symbol) {
      .name   = lval.start,
      .length = lval.length,
      .value  = value,
    };

    // we have to make sure to delete them _after_ any errors.
    // otherwise their locations won't exist anymore to print them.
    remove_token(toks, i);
    remove_token(toks, i);
    remove_token(toks, i);
    remove_token(toks, i);
    i -= 4;

    append_symbol(sym, &syms);
  }

  // TODO: right here, sort symbols lexicographically so we can do a
  //       binary search lookup and detect duplicates. use strncmp!

  for (size_t i = 0; i < toks->count; i++) {
    Token tok = toks->items[i];
    if (tok.kind != TOKEN_WORD) continue;

    long value;
    if (!lookup_symbol(tok.start, tok.length, &syms, &value)) continue;

    int len = snprintf(NULL, 0, "%ld", value);
    char *s = malloc(len + 1);
    if (s == NULL) return false;

    snprintf(s, len + 1, "%ld", value);

    remove_token(toks, i);
    insert_token(toks, i-1, (Token) {
        .kind = TOKEN_NUMBER,
        .start = s,
        .length = len,
        .location = tok.location,
    });
  }
  
  return true;
}

bool deanonymify(Tokens *toks, Error *error) {
  (void) toks;
  (void) error;

  // TODO!

  return false;
}

bool preprocessor(Tokens *toks, Error *error) {
  (void)toks;
  (void)error;

  if (!deparenthesise(toks, error)) return false;
  if (!destringify(toks, error))    return false;
  delexer(toks, stdout);
  printf("\n\n--\n\n");
  if (!desymbolise(toks, error))    return false;
  if (!deanonymify(toks, error))    return false;

  return true;
}
