#ifndef PREPROCESSOR_H
#define PREPROCESSOR_H

#include "tokeniser.h"
#include "errors.h"

bool deparenthesise(Tokens *toks, Error *error);

bool preprocessor(Tokens *toks, Error *error);

#endif // PREPROCESSOR_H
