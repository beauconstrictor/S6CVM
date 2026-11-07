// 1. deparenthesisation
//   -> keep a stack of the first word after parentheses, then
//      move it to the closing paren
//
// 2. destringifying
//   -> convert string literals to anonymous blocks
//
// 3. deanonymification
//   -> move anonymous blocks to global named blocks, with internal
//      names like ___anon_0

#include <stdio.h>
#include <stdbool.h>

#include "tokeniser.h"
#include "errors.h"

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
  (void)error;

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
    });

  }

  return true;
}

bool preprocessor(Tokens *toks, Error *error) {
  if (!deparenthesise(toks, error)) return false;
  if (!destringify(toks, error))    return false;

  return true;
}
