//
// Created by BNunnally on 10/5/2021.
//

#include "get_expression.h"
#include "../expression.h"
#include "../../../context/context.h"
#include "../../../../taskdelegator/task_delegator.h"
#include "../../../../compiler_info.h"

void compile_get_expression(expression *e, Ast *ast) {
    e->type.type = type_get_component_request;

    string typeName, componentName;
    Token *tok;

    if((tok = ast->getToken(STRING_LITERAL)) != NULL) {
       typeName = tok->getValue();
    }

    if((tok = ast->getToken(IDENTIFIER)) != NULL) {
        componentName = tok->getValue();
    }

    if(get_primary_component(&currThread->currTask->file->context) == NULL) {
        currThread->currTask->file->errors->createNewError(PREVIOUSLY_DEFINED, ast->line, ast->col,
                    "cannot call `get()` outside of a component block.");
    }

    e->type.componentRequest = new get_component_request(typeName, componentName);

    type_definition *typeDefinition = NULL;

    // pre-load with type for better error reporting
    if(!typeName.empty()) {
        if(!componentName.empty()) {
            typeDefinition = get_type_definition(
                    componentManager, typeName, componentName);
        } else {
            component *primary = get_primary_component(&currThread->currTask->file->context);

            if(primary != NULL) {
                typeDefinition = get_type_definition(typeName, primary);
            }

            if(typeDefinition == NULL) {
                typeDefinition = get_type_definition(
                        componentManager, typeName, NULL);
            }
        }
    }

    if(typeDefinition != NULL) {
        e->type.componentRequest->resolvedTypeDefinition = typeDefinition;
    }
}