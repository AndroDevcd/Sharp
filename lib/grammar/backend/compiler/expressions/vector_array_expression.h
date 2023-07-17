//
// Created by BNunnally on 11/27/2021.
//

#ifndef SHARP_VECTOR_ARRAY_EXPRESSION_H
#define SHARP_VECTOR_ARRAY_EXPRESSION_H

#include "../../../frontend/parser/Ast.h"
#include "../../../../util/KeyPair.h"

struct expression;
struct sharp_type;
struct sharp_class;

void compile_vector_array_expression(expression *e, Ast *ast);
void compile_vector_item(Ast *ast, sharp_type *arrayType, List<expression*>& arrayItems);


#endif //SHARP_VECTOR_ARRAY_EXPRESSION_H
