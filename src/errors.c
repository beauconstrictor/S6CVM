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

typedef struct {
    const char *red;
    const char *bold;
    const char *yellow;
    const char *dim;
    const char *reset;
} Colours;

Colours get_colours(void) {
    bool colour = colour_allowed();
    return (Colours) {
        .red    = colour ? "\033[31m" : "",
        .bold   = colour ? "\033[1m"  : "",
        .yellow = colour ? "\033[33m" : "",
        .dim    = colour ? "\033[2m"  : "",
        .reset  = colour ? "\033[0m"  : "",
    };
}

void print_error(const Error *error) {
  Colours cols = get_colours();

  fprintf(stderr, "%s%s:%zu:%zu: %serror: %s%s\n", cols.bold,
      error->location.path, error->location.line,
      error->location.col, cols.red, cols.reset, error->message);

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

  fprintf(stderr, "%5zu %s|%s %s", error->location.line, cols.dim,
      cols.reset, line);
  fprintf(stderr, "%s%5s | %s", cols.dim, "", cols.reset);
  for (size_t i = 0; i < error->location.col-1; i++)
    fputc(' ', stderr);
  fprintf(stderr, "%s^%s\n\n", cols.yellow, cols.reset);

  fclose(f);
}

void check_error(const Error *error) {
  if (error->set) {
    print_error(error);
    exit(1);
  }
}

void cli_error(const char *s, const char *binary) {
  Colours cols = get_colours();

  fprintf(stderr, "%ss6cvm: %s", cols.red, cols.reset);
  fprintf(stderr, s, binary);
  fprintf(stderr, "\n");
  exit(1);
}

