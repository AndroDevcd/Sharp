//
// Created by BNunnally on 10/5/2021.
//

#include "get_expression.h"
#include "../expression.h"

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

    e->type.componentRequest = new get_component_request(typeName, componentName);
}