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

