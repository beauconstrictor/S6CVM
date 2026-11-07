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
  const char *bold   = colour ? "\033[1m"  : "";
  const char *yellow = colour ? "\033[33m" : "";
  const char *dim    = colour ? "\033[2m"  : "";
  const char *reset  = colour ? "\033[0m"  : "";

  fprintf(stderr, "%s%s:%zu:%zu: %serror: %s%s\n", bold,
      error->location.path, error->location.line,
      error->location.col, red, reset, error->message);

  FILE *f = fopen(error->location.path, "r");

  // if we can't show the error line for whatever reason, we just
  // don't bother
  if (f == NULL) {
    fprintf(stderr, "\n");
    return;
  }

  char line[4096];

  for (size_t i = 1; i <= error->location.line; i++) {
    if (fgets(line, sizeof(line), f) == NULL) {
      fclose(f);
      fprintf(stderr, "\n");
      return;
    }
  }

  // TODO: this looks fucked with tabs

  fprintf(stderr, "%5zu %s|%s %s", error->location.line, dim, reset,
      line);
  fprintf(stderr, "%s%5s | %s", dim, "", reset);
  for (size_t i = 0; i < error->location.col-1; i++)
    fputc(' ', stderr);
  fprintf(stderr, "%s^%s\n\n", yellow, reset);

  fclose(f);
}
