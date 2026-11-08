#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <limits.h>

#include "tokeniser.h"
#include "preprocessor.h"

#define USAGE "Usage: %s [OPTION]... FILE \n" \
              "Try passing '-h' for more information.\n"

#define HELP "Usage: %s [OPTION]... FILE \n" \
             "Compile S6CVM programs.\n" \
             "\n" \
             "Options:\n" \
             "\t-h  Show this message and exit\n" \
             "\t-v  Show version number and license information\n" \
             "\t-p  Stop compilation after preprocessing\n" \

#define VERSION "%s (S6 Compact Virtual Machine) version 0.1.0\n" \
                "Licensed under the FOSS MIT license.\n" \
                "\n" \
                "Written by Beau Constrictor; see\n" \
                "<https://github.com/beauconstrictor/s6cvm>\n"

void check_error(Error *error) {
  if (error->set) {
    print_error(error);
    exit(1);
  }
}

void cli_error(const char *s, const char *binary) {
  fprintf(stderr, s, binary);
  exit(1);
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

void show_help(const char *binary) {
  printf(HELP, binary);
}

void show_version(const char *binary) {
  printf(VERSION, binary);
}

int main(int argc, char **argv) {
  int opt;
  char output[PATH_MAX] = "a.out";

  while ((opt = getopt(argc, argv, "hvo:")) != -1) {
    switch (opt) {
      case 'h': {
        show_help(argv[0]);
        return 0;
      } break;
      case 'v': {
        show_version(argv[0]);
        return 0;
      } break;
      case 'o': {
        snprintf(output, sizeof(output), "%s", optarg);
      } break;
    }
  }

  if (argc - optind != 1)
    cli_error(USAGE, argv[0]);
  char *filename = argv[optind];
  char *source = read_file(filename);

  if (source == NULL) cli_error(strerror(errno), argv[0]);

  Tokens toks = create_tokens();
  Error error = {0};

  if (!lexer(source, filename, &toks, &error)) check_error(&error);
  if (!preprocessor(&toks, &error)) check_error(&error);

  delexer(&toks, stdout);
  printf("\n");

  return 0;
}
