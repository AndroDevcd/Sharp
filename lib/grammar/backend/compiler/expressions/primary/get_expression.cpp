//
// Created by BNunnally on 10/5/2021.
//

#include "get_expression.h"
#include "../expression.h"
#include "../../../context/context.h"
#include "../../../../taskdelegator/task_delegator.h"
#include "../../../../compiler_info.h"
#include "../../../../settings/settings.h"

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

    if(get_primary_component(&currThread->currTask->file->ctx) == NULL
        && (!ast->hasSubAst(ast_utype) && (typeName.empty() || componentName.empty()))) {
        create_new_error(GENERIC, ast->line, ast->col,
                    "cannot call `get()` outside of a component block without explicitly declaring type. Try `get<my_type>()`.");
    }

    if(ast->getSubAst(ast_utype)) {
        sharp_type requiredType = resolve(ast->getSubAst(ast_utype));
        if(requiredType.type < type_untyped) {
            type_definition *typeDefinition = NULL;

            if(typeName.empty() && componentName.empty()) {
                typeDefinition
                    = get_type_definition(componentManager, requiredType, ast);

                if(typeDefinition != NULL) {
                    e->type.copy(*typeDefinition->type);
                    e->scheme.copy(*typeDefinition->scheme);
                    return;
                } else {
                    create_new_error(GENERIC, ast->line, ast->col,
                         "No defined type found of `" + type_to_str(requiredType) + "`.");
                }
            } else {
                if(!typeName.empty() && !componentName.empty()) {
                    create_new_warning(GENERIC, __w_dep, ast->getSubAst(ast_utype)->line, ast->getSubAst(ast_utype)->col,
                            "unnecessary explicit type declaration for `get()`");

                    typeDefinition
                            = get_type_definition(componentManager, typeName, componentName);
                } else {
                    if(!componentName.empty()) {
                        typeDefinition
                                = get_type_definition(componentManager, requiredType, componentName);
                    } else {
                        typeDefinition
                            = get_type_definition(componentManager, typeName, requiredType, ast);
                    }
                }

                if(typeDefinition != NULL) {
                    if (is_explicit_type_match(requiredType, *typeDefinition->type)) {
                        e->type.copy(*typeDefinition->type);
                        e->scheme.copy(*typeDefinition->scheme);
                        return;
                    } else {
                        create_new_error(GENERIC, ast->line, ast->col,
                            "Type mismatch on type definition. Type was found to be `" + type_to_str(*typeDefinition->type)
                                + "`, but `" + type_to_str(requiredType) + "` was expected.");
                    }
                } else {
                    create_new_error(GENERIC, ast->line, ast->col,
                             "Could not find type definition for type `" + type_to_str(requiredType) + "`.");
                }
            }
        }
    }

    e->type.componentRequest = new get_component_request(typeName, componentName);

    type_definition *typeDefinition = NULL;

    // pre-load with type for better error reporting
    if(!typeName.empty()) {
        if(!componentName.empty()) {
            typeDefinition = get_type_definition(
                    componentManager, typeName, componentName);
        } else {
            component *primary = get_primary_component(&currThread->currTask->file->ctx);

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