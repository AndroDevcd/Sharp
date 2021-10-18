//
// Created by BNunnally on 10/17/2021.
//

#include "lambda_expression.h"
#include "../../../types/sharp_field.h"
#include "../../../astparser/ast_parser.h"

void compile_lambda_expression(expression *e, Ast *ast) {

    List<sharp_field*> params;
    if(ast->hasSubAst(ast_lambda_arg_list)) {
        parse_lambda_arg_list(params, ast->getSubAst(ast_lambda_arg_list));


        for(Int i = 0; i < params.size(); i++) {
            if(params.get(i)->type.type != type_any) {
                sharp_type resolvedType;
                resolve(params.get(i)->type, resolvedType,
                        false, resolve_hard_type, params.get(i)->ast);

                params.get(i)->type.copy(resolvedType);
            }
        }
    }



}
