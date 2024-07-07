#ifndef CODEGEN_H
#define CODEGEN_H

#include "parser.h"

void generate_code(AST *node, FILE *output_file);

#endif
