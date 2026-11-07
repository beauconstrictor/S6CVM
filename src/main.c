#include <stdio.h>

#include "tokeniser.h"

const char *program = "{:negate <x : x -x>> (sub 234 over) \"Hello, world!\"}";
int main(void) {
  Tokens toks = create_tokens();
  Error error;
  lexer(program, "<unknown>", &toks, &error);

  if (error.set) {
    print_error(&error);
    return 1;
  }

  for (size_t i = 0; i < toks.count; i++) {
    Token tok = toks.items[i];
    printf("%s %.*s\n", token_kind_name(tok.kind), (int)tok.length, tok.start);
  }

  return 0;
}
