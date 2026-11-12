#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#include "tokeniser.h"
#include "parser.h"

bool append_symbol(Symbol sym, Symbols *syms) {
  if (syms->count < syms->capacity) {
    syms->items[syms->count++] = sym;
    return true;
  }

  size_t new_capa = syms->capacity;
  new_capa = new_capa > 0 ? new_capa*2 : 1;
  Symbol *new = realloc(syms->items, sizeof(Symbol) * new_capa);
  if (new == NULL) return false;

  syms->items = new;
  syms->capacity = new_capa;
  syms->items[syms->count++] = sym;

  return true;
}

bool append_token_symbol(Symbols *syms, Token *tok, long value) {
  return append_symbol((Symbol) {
    .name   = tok->start,
    .length = tok->length,
    .value  = value,
  }, syms);
}

bool lookup_symbol(const char *s, size_t length, Symbols *syms, long *value) {
  for (size_t i = 0; i < syms->count; i++) {
    Symbol sym = syms->items[i];

    if (length != sym.length) continue;
    if (strncmp(s, sym.name, length) != 0) continue;

    *value = sym.value;
    return true;
  }

  return false;
}

long read_number(const Token *tok, Error *error) {
  char buf[256];

  snprintf(buf, sizeof(buf), "%.*s", (int)tok->length, tok->start);
  char *end;
  long value = strtol(buf, &end, 10);

  if (end != buf + tok->length) {
    *error = (Error) {
      .location = tok->location,
      .message = "expected a number",
      .set = true,
    };
    return -1;
  }

  if (errno == ERANGE) {
    *error = (Error) {
      .location = tok->location,
      .message = "number out of range",
      .set = true,
    };
    return -1;
  }
  
  return value;
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

