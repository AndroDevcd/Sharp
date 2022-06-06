//
// Created by bknun on 6/2/2022.
//

#ifndef SHARP_FUNCTION_COMPILER_H
#define SHARP_FUNCTION_COMPILER_H

#include "../../types/sharp_function.h"

void compile_function(sharp_function *function);
bool compile_block(Ast *ast, operation_schema *scheme);

#endif //SHARP_FUNCTION_COMPILER_H
