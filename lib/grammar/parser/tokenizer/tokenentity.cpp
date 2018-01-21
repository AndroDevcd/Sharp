//
// Created by bknun on 11/13/2017.
//

#include "tokenentity.h"

token_entity_id token_entity::getId()
{
    return id;
}

void token_entity::setId(token_entity_id id)
{
    this->id = id;
}

string token_entity::getToken()
{
    return tok;
}

int token_entity::getColumn()
{
    return col;
}

int token_entity::getLine()
{
    return line;
}

bool token_entity::isSingle()
{
    return id == SINGLE;
}

token_type token_entity::getTokenType()
{
    return type;
}