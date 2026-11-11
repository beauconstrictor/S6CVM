#ifndef ASSEMBLER_H
#define ASSEMBLER_H

#include <stdio.h>

#include "errors.h"
#include "parser.h"
#include "tokeniser.h"

bool verify_syntax(Tokens *toks, Error *error);
// make sure to verify syntax before calling!
bool assemble(FILE *f, Symbols *syms, Tokens *toks, Error *error);

#endif // ASSEMBLER_H
