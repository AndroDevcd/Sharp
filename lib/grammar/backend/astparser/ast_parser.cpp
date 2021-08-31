//
// Created by BNunnally on 8/31/2021.
//

#include "ast_parser.h"

string concat_tokens(Ast *ast) {
    string data;
    for(long i = 0; i < ast->getTokenCount(); i++)
        data += ast->getToken(i).getValue();

    return data;
}