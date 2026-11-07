#include <stdio.h>
#include <stdlib.h>

#include "tokeniser.h"
#include "preprocessor.h"

const char *program = "test.cvm";

void check_error(Error *error) {
  if (error->set) {
    print_error(error);
    exit(1);
  }
}

char *read_file(const char *path) {
  FILE *file = fopen(path, "rb");

  if (file == NULL) {
    return NULL;
  }

  fseek(file, 0, SEEK_END);
  long size = ftell(file);
  rewind(file);

  if (size < 0) {
    fclose(file);
    return NULL;
  }

  char *contents = malloc((size_t)size + 1);

  if (contents == NULL) {
    fclose(file);
    return NULL;
  }

  size_t bytes_read = fread(contents, 1, (size_t)size, file);

  fclose(file);

  if (bytes_read != (size_t)size) {
    free(contents);
    return NULL;
  }

  contents[bytes_read] = '\0';

  // remove terminating newline
  if (bytes_read > 0 && contents[bytes_read - 1] == '\n') {
    contents[--bytes_read] = '\0';
  }

  return contents;
}

int main(void) {
  char *f = read_file(program);
  if (f == NULL) {
    perror("s6cvm");
    return 1;
  }

  Tokens toks = create_tokens();
  Error error = {0};

  lexer(f, program, &toks, &error);
  check_error(&error);

  preprocessor(&toks, &error);
  check_error(&error);

  for (size_t i = 0; i < toks.count; i++) {
    Token tok = toks.items[i];
    printf("%s %.*s\n", token_kind_name(tok.kind), (int)tok.length, tok.start);
  }

  free(f);

  return 0;
}
