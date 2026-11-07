#ifndef ERRORS_H
#define ERRORS_H

#include <stdbool.h>

typedef struct {
  size_t line;
  size_t col;
  const char *path;
} Location;

typedef struct {
  Location location;
  const char *message;
  bool set;
} Error;

void print_error(const Error *error);

#endif // ERRORS_H
