#ifndef PREPROCESSOR_H
#define PREPROCESSOR_H

#include "tokeniser.h"
#include "parser.h"
#include "errors.h"

#define ANON_PREFIX "___anon_"

bool preprocessor(Tokens *toks, Symbols *syms, Error *error);

#endif // PREPROCESSOR_H
