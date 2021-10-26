//
// Created by BNunnally on 9/24/2021.
//

#include "primary_expression.h"
#include "../unary/not_expression.h"
#include "literal_expression.h"
#include "self_expression.h"
#include "dot_notation_call_expression.h"
#include "base_expression.h"
#include "null_expression.h"
#include "new_expression.h"
#include "get_expression.h"
#include "lambda_expression.h"
#include "sizeof_expression.h"
#include "paren_expression.h"
#include "array_expression.h"
#include "cast_expression.h"
#include "is_expression.h"

void compile_primary_expression(expression *e, Ast *ast) {
    if(ast->hasSubAst(ast_not_e))
        compile_not_expression(e, ast->getSubAst(ast_not_e));
    else if(ast->hasSubAst(ast_literal_e))
        compile_literal_expression(e, ast->getSubAst(ast_literal_e)->getSubAst(ast_literal));
    else if(ast->hasSubAst(ast_self_e))
        compile_self_expression(e, ast->getSubAst(ast_self_e));
    else if(ast->hasSubAst(ast_dot_not_e))
        compile_dot_notation_call_expression(e, NULL, ast->getSubAst(ast_dot_not_e));
    else if(ast->hasSubAst(ast_base_e))
        compile_base_expression(e, ast->getSubAst(ast_base_e));
    else if(ast->hasSubAst(ast_null_e))
        compile_null_expression(e, ast->getSubAst(ast_null_e));
    else if(ast->hasSubAst(ast_new_e))
        compile_new_expression(e, ast->getSubAst(ast_new_e));
    else if(ast->hasSubAst(ast_get_component))
        compile_get_expression(e, ast->getSubAst(ast_get_component));
    else if(ast->hasSubAst(ast_lambda_function))
        compile_lambda_expression(e, ast->getSubAst(ast_lambda_function));
    else if(ast->hasSubAst(ast_sizeof_e))
        compile_sizeof_expression(e, ast->getSubAst(ast_sizeof_e));
    else if(ast->getSubAst(ast_paren_e))
        compile_paren_expression(e, ast->getSubAst(ast_paren_e));
    else if(ast->getSubAst(ast_cast_e))
        compile_cast_expression(e, ast->getSubAst(ast_cast_e));
    else if(ast->getSubAst(ast_is_e))
        compile_is_expression(e, ast->getSubAst(ast_is_e));
}