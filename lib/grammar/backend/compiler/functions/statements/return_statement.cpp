//
// Created by bknun on 6/2/2022.
//

#include "return_statement.h"
#include "../../../../compiler_info.h"
#include "../../expressions/expression.h"
#include "../../../types/types.h"
#include "../../compiler.h"

void compile_return_statement(Ast *ast, operation_schema *scheme, bool *controlPaths) {
    current_context.blockInfo.reachable = false;

    sharp_function *function = NULL;
    uInt match_result;
    expression returnVal;
    List<operation_schema*> lockSchemes;
    operation_schema *subScheme = new operation_schema();
    subScheme->schemeType = scheme_return;

    if(ast->getSubAst(ast_expression)) {
        compile_expression(returnVal, ast->getSubAst(ast_expression));
    }
    else {
        returnVal.type.type = type_nil;
    }

    retrieve_lock_schemes(&current_context.blockInfo, lockSchemes);
    for(Int i = 0; i < lockSchemes.size(); i++) { // todo look into also processing finally blocks in the same way
        operation_schema* tmp = new operation_schema();
        create_unlock_operation(tmp, lockSchemes.get(i));
        add_scheme_operation(subScheme, tmp);
    }

    auto *f = current_context.functionCxt;

    match_result = is_implicit_type_match(current_context.functionCxt->returnType, returnVal.type, constructor_only, function);
    if(match_result == match_constructor) {
        operation_schema* tmp = new operation_schema();
        compile_initialization_call(ast, function, returnVal, tmp);
        create_object_return_operation(subScheme, tmp);
    } else if(is_match_normal(match_result)) {
        if(is_numeric_type(returnVal.type) && !returnVal.type.isArray)
            create_numeric_return_operation(subScheme, &returnVal.scheme);
        else if(returnVal.type.type == type_nil)
            create_return_operation(subScheme);
        else create_object_return_operation(subScheme, &returnVal.scheme);
    } else if(match_result == indirect_match_w_nullability_mismatch) {
        currThread->currTask->file->errors->createNewError(INCOMPATIBLE_TYPES, ast->line, ast->col,
                                                           " expressions are not compatible, assigning null to non nullable type of `" + type_to_str(current_context.functionCxt->returnType) + "`.");
    } else {
        current_file->errors->createNewError(GENERIC, ast->line, ast->col, "returning `" + type_to_str(returnVal.type) + "` from a function returning `"
                      + type_to_str(current_context.functionCxt->returnType) + "`.");
    }

    if(inside_block(&current_context.blockInfo, finally_block)) {
        current_file->errors->createNewError(GENERIC, ast->line, ast->col, "control cannot leave body of finally clause");
    }

    add_scheme_operation(scheme, subScheme);
    controlPaths[MAIN_CONTROL_PATH] = true;
}
