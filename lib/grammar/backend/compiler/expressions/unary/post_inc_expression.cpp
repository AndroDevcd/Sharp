//
// Created by BNunnally on 10/21/2021.
//

#include "post_inc_expression.h"
#include "../expression.h"
#include "unary_expression.h"
#include "../primary/post_ast_expression.h"

void compile_post_inc_expression(expression *e, Ast *ast, bool compileExpression) {

    if(compileExpression)
        compile_expression(*e, ast->getSubAst(ast_expression));

    string op = ast->getToken(0).getValue();
    compile_unary_postfix_expression(e, op, ast);

    compile_post_ast_expression(e, ast);
}
