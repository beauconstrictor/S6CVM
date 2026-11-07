#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include "errors.h"

bool colour_allowed(void) {
  const char *no_color = getenv("NO_COLOR");

  if (no_color != NULL && no_color[0] != '\0')
      return false;
  if (!isatty(STDERR_FILENO))
      return 0;

  const char *term = getenv("TERM");

  if (term == NULL || strcmp(term, "dumb") == 0)
    return false;

  return true;
}

void print_error(const Error *error) {
  bool colour = colour_allowed();
  const char *red    = colour ? "\033[31m" : "";
  const char *yellow = colour ? "\033[33m" : "";
  const char *bold   = colour ? "\033[1m" : "";
  const char *reset  = colour ? "\033[0m" : "";

  fprintf(stderr, "%ss6cvm: %s%s%s:%zu:%zu:%s %s\n",
      red, yellow, bold, error->location.path, error->location.line,
      error->location.col, reset, error->message);
}
