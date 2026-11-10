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
//
// 5. deimplication
//   -> turn all implicit calls like 'fib' into '&fib call'

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#include "tokeniser.h"
#include "parser.h"
#include "errors.h"

#define ALLOC_START 0x38b0
#define ALLOC_MAX   0x6fff
#define ANON_PREFIX "___anon_"

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

bool desymbolise(Tokens *toks, Error *error) {
  Symbols syms = {0};

  size_t allocator = ALLOC_START;

  size_t i = 0;

  while (i < toks->count) {
    Token tok = toks->items[i];

    if (tok.kind == TOKEN_CONST) {
      if (toks->count < i + 4
        || toks->items[i+1].kind != TOKEN_WORD
        || toks->items[i+2].kind != TOKEN_ASSIGN
        || toks->items[i+3].kind != TOKEN_NUMBER) {
        *error = (Error) {
          .location = tok.location,
          .message = "expected assignment after " CONST_WORD,
          .set = true,
        };
        return false;
      }

      Token lval  = toks->items[i+1];
      Token rval = toks->items[i+3];

      long value = read_number(&rval, error);
      if (error->set) return false;

      if (!append_token_symbol(&syms, &lval, value)) return false;

      // we have to make sure to delete them _after_ any errors.
      // otherwise their locations won't exist anymore to print them.
      for (int x = 0; x < 4; x++)
        remove_token(toks, i);

      continue;

    } else if (tok.kind == TOKEN_VAR) {
      if (toks->count < i + 5
        || toks->items[i+1].kind != TOKEN_WORD
        || toks->items[i+2].kind != TOKEN_LSQUARE
        || toks->items[i+3].kind != TOKEN_NUMBER
        || toks->items[i+4].kind != TOKEN_RSQUARE) {
        *error = (Error) {
          .location = tok.location,
          .message = "expected declaration after " VAR_WORD,
          .set = true,
        };
        return false;
      }

      Token lval = toks->items[i+1];
      Token rval = toks->items[i+3];

      long size = read_number(&rval, error);
      if (error->set) return false;

      if (allocator + size > ALLOC_MAX) {
        *error = (Error) {
          .location = tok.location,
          .message = "insufficient memory for variable allocation" VAR_WORD,
          .set = true,
        };
        return false;
      }

      if (!append_token_symbol(&syms, &lval, allocator)) return false;
      allocator += size;

      for (int x = 0; x < 5; x++)
        remove_token(toks, i);
 
      continue;
    }

    i++;
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
    insert_token(toks, i, (Token) {
        .kind = TOKEN_NUMBER,
        .start = s,
        .length = len,
        .location = tok.location,
    });
  }
  
  return true;
}

struct Blocks {
  Tokens *items;
  size_t count;
  size_t capacity;
};

// TODO: maybe move all these dynamic arrays into a macro?
bool append_block(Tokens block, struct Blocks *blocks) {
  if (blocks->count < blocks->capacity) {
    blocks->items[blocks->count++] = block;
    return true;
  }

  size_t new_capa = blocks->capacity;
  new_capa = new_capa > 0 ? new_capa*2 : 1;
  Tokens *new = realloc(blocks->items, sizeof(Tokens) * new_capa);
  if (new == NULL) return false;

  blocks->items = new;
  blocks->capacity = new_capa;
  blocks->items[blocks->count++] = block;

  return true;
}

static bool deanonymify_inner(Tokens *toks, struct Blocks *blocks,
    size_t *anon_count, Error *error) {
  size_t i = 0;

  while (i < toks->count) {
    if (toks->items[i].kind != TOKEN_LBRACE) {
      i++;
      continue;
    }

    Token opening = toks->items[i];

    size_t depth = 1;
    size_t end = i + 1;

    while (end < toks->count && depth > 0) {
      if (toks->items[end].kind == TOKEN_LBRACE)
        depth++;
      else if (toks->items[end].kind == TOKEN_RBRACE)
        depth--;

      end++;
    }

    if (depth != 0) {
      *error = (Error) {
        .location = opening.location,
        .message = "unterminated block",
        .set = true,
      };
      return false;
    }

    Tokens block = create_tokens();

    for (size_t x = i + 1; x < end - 1; x++) {
      if (!append_token(toks->items[x], &block)) return false;
    }

    if (!deanonymify_inner(&block, blocks, anon_count, error))
      return false;

    bool named =
      block.count > 0 &&
      block.items[0].kind == TOKEN_DEFINITION;

    if (named) {
      for (size_t x = i + 1; x < end - 1; x++)
        remove_token(toks, i + 1);

      for (size_t x = 0; x < block.count; x++)
        insert_token(toks, i + 1 + x, block.items[x]);

      i += block.count + 2;
      continue;
    }

    int len = snprintf(
      NULL, 0, ANON_PREFIX "%zu", (*anon_count)
    );

    char *name = malloc(len + 1);
    if (name == NULL)
      return false;

    snprintf(name, len + 1, ANON_PREFIX "%zu", (*anon_count)++);
    if (!append_block(block, blocks))
      return false;

    for (size_t x = i; x < end; x++)
      remove_token(toks, i);

    insert_token(toks, i, (Token) {
      .kind = TOKEN_WORD,
      .start = name,
      .length = len,
      .location = opening.location,
    });

    i++;
  }

  return true;
}

bool deanonymify(Tokens *toks, Error *error) {
  struct Blocks blocks = {0};
  size_t anon_count = 0;

  if (!deanonymify_inner(toks, &blocks, &anon_count, error))
    return false;

  // add back the anonymous blocks the the end of the main
  // token stream
  for (size_t i = 0; i < blocks.count; i++) {
    int len = snprintf(NULL, 0, ANON_PREFIX "%zu", i);

    char *name = malloc(len + 1);
    if (name == NULL)
      return false;

    snprintf(name, len + 1, ANON_PREFIX "%zu", i);

    append_token((Token) {
      .kind = TOKEN_LBRACE,
      .start = "{",
      .length = 1,
    }, toks);

    append_token((Token) {
      .kind = TOKEN_DEFINITION,
      .start = name,
      .length = len,
    }, toks);

    for (size_t j = 0; j < blocks.items[i].count; j++)
      append_token(blocks.items[i].items[j], toks);

    append_token((Token) {
      .kind = TOKEN_RBRACE,
      .start = "}",
      .length = 1,
    }, toks);
  }

  return true;
}

bool preprocessor(Tokens *toks, Error *error) {
  (void)toks;
  (void)error;

  if (!deparenthesise(toks, error)) return false;
  if (!destringify(toks, error))    return false;
  if (!desymbolise(toks, error))    return false;
  if (!deanonymify(toks, error))    return false;

  return true;
}
