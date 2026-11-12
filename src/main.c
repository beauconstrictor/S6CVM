#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <limits.h>

#include "parser.h"
#include "tokeniser.h"
#include "assembler.h"
#include "simulator.h"
#include "preprocessor.h"

#define USAGE "Usage: %s [OPTION]... [MODE] FILE \n" \
              "Try passing '-h' for more information.\n"

#define HELP "Usage: %s [OPTION]... FILE \n" \
             "Compile S6CVM programs.\n" \
             "\n" \
             "Options:\n" \
             "\t-h  Show this message and exit\n" \
             "\t-v  Show version number and license information\n" \
             "\t-t  Format IR as raw tokens\n" \
             "\t-s  File to write a table of function addresses to\n" \
             "\t-o  File to write binary or IR to\n" \
             "\n" \
             "Modes:\n" \
             "\t-l  Stop compilation after lexing\n" \
             "\t-p  Stop compilation after preprocessing\n" \
             "\t-c  Fully compile program (default)\n" \
             "\t-i  Immediately interpret program\n" \
             "\n" \
             "Use '-' in place of paths to use the standard output.\n" \

#define VERSION "%s (S6 Compact Virtual Machine) version 0.1.0\n" \
                "Licensed under the FOSS MIT license.\n" \
                "\n" \
                "Written by Beau Constrictor; see\n" \
                "<https://github.com/beauconstrictor/s6cvm>\n"

typedef enum {
  MODE_LEXER,
  MODE_PREPROCESS,
  MODE_COMPILE,
  MODE_INTERPRET,
} CompilationMode;

void show_help(const char *binary) {
  printf(HELP, binary);
}

void show_version(const char *binary) {
  printf(VERSION, binary);
}

void show_ir(Tokens *toks, CompilationMode mode, bool raw_tokens,
    bool dump_syms, const char *binary) {
  if (dump_syms) {
    if (mode == MODE_LEXER)
      cli_error("cannot generate a symbol table in lexing mode", binary);
    if (mode == MODE_PREPROCESS)
      cli_error("cannot generate a symbol table in preprocessing mode", binary);
  } else if (raw_tokens) {
    for (size_t i = 0; i < toks->count; i++) {
      Token tok = toks->items[i];
      printf("%s %.*s\n", token_kind_name(tok.kind), (int)tok.length, tok.start);
    }

  } else {
    delexer(toks, stdout);
    printf("\n");
  }
}

FILE *get_handle(const char *path, const char *binary, bool binmode) {
  if (strcmp(path, "-") == 0) return stdout;

  FILE *handle = fopen(path, binmode ? "wb" : "w");
  if (handle == NULL) {
    char buf[PATH_MAX];
    snprintf(buf, sizeof(buf), "%s: %s", binary, path);
    perror(buf);
    exit(1);
  }
  return handle;
}

void dump_symbols(Symbols *syms, FILE *f) {
  for (size_t i = 0; i < syms->count; i++) {
    Symbol sym = syms->items[i];
    if (strncmp(sym.name, ANON_PREFIX, sizeof(ANON_PREFIX)-1) == 0)
      continue;
    fprintf(f, "%.*s\t%ld\n", (int)sym.length, sym.name, sym.value);
  }
}

int main(int argc, char **argv) {
  int opt;
  char output[PATH_MAX] = "a.out";
  char symtable[PATH_MAX];

  CompilationMode mode = MODE_COMPILE;

  bool raw_tokens = false;
  bool dump_syms  = false;

  while ((opt = getopt(argc, argv, "hvo:tplcis:")) != -1) {
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
      case 'l': {
        mode = MODE_LEXER;
      } break;
      case 'p': {
        mode = MODE_PREPROCESS;
      } break;
      case 'c': {
        mode = MODE_COMPILE;
      } break;
      case 'i': {
        mode = MODE_INTERPRET;
      } break;
      case 't': {
        raw_tokens = true;
      } break;
      case 's': {
        dump_syms = true;
        snprintf(symtable, sizeof(symtable), "%s", optarg);
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
  if (mode == MODE_LEXER) {
    show_ir(&toks, mode, raw_tokens, dump_syms, argv[0]);
    return 0;
  }
  Symbols vars = {0};
  if (!preprocessor(&toks, &vars, &error)) check_error(&error);
  if (mode == MODE_PREPROCESS) {
    show_ir(&toks, mode, raw_tokens, dump_syms, argv[0]);
    return 0;
  }

  if (!verify_syntax(&toks, &error)) check_error(&error);

  Symbols syms = {0};
  FILE *out = get_handle(output, argv[0], false);

  if (!assemble(out, &syms, &toks, &error)) check_error(&error);

  if (dump_syms) {
    FILE *syms_out = get_handle(symtable, argv[0], false);
    if (syms_out == out) fprintf(out, "\n");
    dump_symbols(&vars, syms_out);
    fprintf(syms_out, "\n");
    dump_symbols(&syms, syms_out);
  }
  
  if (out != stdout) fclose(out);

  if (mode == MODE_INTERPRET) {
    char *tape = read_file(output);
    if (tape == NULL) cli_error("no such file", argv[0]);
    const char *error = simulate(tape);
    if (error) cli_error(error, argv[0]);
  }

  return 0;
}
