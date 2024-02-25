//
// Created by bknun on 6/2/2022.
//

#ifndef SHARP_FUNCTION_COMPILER_H
#define SHARP_FUNCTION_COMPILER_H

#include "../../types/sharp_function.h"
#include "../../context/context.h"

void compile_function(sharp_class *with_class, function_type type, Ast *ast);
void compile_function(sharp_function *function, Ast *block);
bool compile_block(
        Ast *ast,
        operation_schema *scheme,
        block_type bt = normal_block,
        sharp_label *repeatLoopLabel = NULL,
        sharp_label *endLabel = NULL,
        Ast *lockExpression = NULL,
        sharp_label *finallyLabel = NULL);

void compile_function(
        sharp_class *with_class,
        function_type type,
        Ast *ast);

void compile_default_constructor(sharp_class *with_class);
void compile_class_functions(sharp_class *with_class, Ast *block);
void compile_class_mutations(sharp_class *with_class, Ast *block);
void compile_class_lambdas(sharp_class *with_class);

#endif //SHARP_FUNCTION_COMPILER_H
