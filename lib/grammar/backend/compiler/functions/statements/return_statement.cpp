//
// Created by bknun on 6/2/2022.
//

#include "return_statement.h"
#include "../../../../compiler_info.h"
#include "../../expressions/expression.h"
#include "../../../types/types.h"
#include "../../compiler.h"

void compile_constructor_call(Ast *ast, sharp_function *constructor, expression &e, operation_scheme *scheme);

void compile_return_statement(Ast *ast, bool *controlPaths) {
    current_context.blockInfo.reachable = false;

    sharp_function *function;
    uInt match_result;
    expression returnVal;
    operation_scheme *scheme = new operation_scheme();
    List<operation_scheme*> lockSchemes;

    scheme->schemeType = scheme_return;
    if(ast->getSubAst(ast_expression)) {
        compile_expression(returnVal, ast->getSubAst(ast_expression));
    }
    else {
        returnVal.type.type = type_nil;
    }

    retrieve_lock_schemes(&current_context.blockInfo, lockSchemes);
    for(Int i = 0; i < lockSchemes.size(); i++) {
        operation_scheme* tmp = new operation_scheme();
        create_unlock_operation(tmp, lockSchemes.get(i));
        add_scheme_operation(scheme, tmp);
    }

    match_result = is_implicit_type_match(current_context.functionCxt->returnType, returnVal.type, constructor_only, function);
    if(match_result == match_constructor) {
        operation_scheme* tmp = new operation_scheme();
        compile_constructor_call(ast, function, returnVal, tmp);
        create_object_return_operation(scheme, tmp);
    } else if(match_result == match_normal) {
        if(is_numeric_type(returnVal.type) && !returnVal.type.isArray)
            create_numeric_return_operation(scheme, &returnVal.scheme);
        else if(returnVal.type.type == type_nil)
            create_return_operation(scheme);
        else create_object_return_operation(scheme, &returnVal.scheme);
    }else {
        current_file->errors->createNewError(GENERIC, ast->line, ast->col, "returning `" + type_to_str(returnVal.type) + "` from a function returning `"
                      + type_to_str(current_context.functionCxt->returnType) + "`.");
    }

    if(inside_block(&current_context.blockInfo, finally_block)) {
        current_file->errors->createNewError(GENERIC, ast->line, ast->col, "control cannot leave body of finally clause");
    }

    add_scheme_operation(current_context.functionCxt->scheme, scheme);
    controlPaths[MAIN_CONTROL_PATH] = true;
}
