//
// Created by BNunnally on 9/17/2021.
//

#include "expression.h"
#include "unary/minus_expression.h"
#include "../../dependency/dependancy.h"
#include "../../types/types.h"
#include "../../../taskdelegator/task_delegator.h"
#include "unary/pre_increment_expression.h"
#include "primary/primary_expression.h"

void compile_expression(expression &e, Ast *ast) {

    if(ast->hasSubAst(ast_minus_e))
        compile_minus_expression(&e, ast->getSubAst(ast_minus_e));
    else if(ast->hasSubAst(ast_pre_inc_e))
        compile_pre_increment_expression(&e, ast->getSubAst(ast_pre_inc_e));
    else if(ast->hasSubAst(ast_primary_expr))
        compile_primary_expression(&e, ast->getSubAst(ast_primary_expr));

}

void compile_expression_for_type(sharp_type &type, Ast *ast) {
    expression e;
    compile_expression(e, ast);

    type.copy(e.type);
}


void compile_class_function_overload(
        sharp_class *with_class,
        expression &e,
        List<sharp_field*> &params,
        List<operation_scheme> &paramOperations,
        string &op,
        Ast *ast) {
    sharp_function *fun;

    if((fun = resolve_function(
            "operator" + op,
            with_class,
            params,
            operator_function,
            match_initializer | match_operator_overload,
            ast,
            true,
            true)) != NULL) {

        compile_function_call(&e.scheme, params, paramOperations, fun, false, false);
        e.type.copy(fun->returnType);
    } else {
        currThread->currTask->file->errors->createNewError(GENERIC, ast,
                "use of operator `" + op + "` does not have any qualified overloads with class `" + with_class->fullName + "`");

    }
}

void compile_function_call(
        operation_scheme *scheme,
        List<sharp_field*> & params,
        List<operation_scheme> &paramOperations,
        sharp_function *callee,
        bool isStaticCall,
        bool isPrimaryClass) {
    if(scheme) {
        Int matchResult;
        List<operation_scheme> compiledParamOperations;
        for (Int i = 0; i < params.size(); i++) {
            sharp_type *asignee = &callee->parameters.get(i)->type;
            sharp_type *assigner = &params.get(i)->type;
            sharp_function *matchedConstructor = NULL;

            matchResult = is_explicit_type_match(*asignee, *assigner);
            if (matchResult == no_match_found) {
                matchResult = is_implicit_type_match(
                        *asignee, *asignee, 0,
                        matchedConstructor);
            }

            create_function_parameter_push_operation(
                    asignee,
                    matchResult,
                    matchedConstructor,
                    &paramOperations.get(i),
                    &compiledParamOperations.__new()
            );
        }

        if (isStaticCall) {
            create_static_function_call_operation(scheme, compiledParamOperations, callee);
        } else {
            if (isPrimaryClass)
                create_primary_class_function_call_operation(scheme, compiledParamOperations, callee);
            else {
                create_instance_function_call_operation(scheme, compiledParamOperations, callee);
            }
        }
    }
}
