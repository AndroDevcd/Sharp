//
// Created by bknun on 11/13/2017.
//

#include "token.h"

token_id Token::getId()
{
    return id;
}

void Token::setId(token_id id)
{
    this->id = id;
}

string Token::getToken()
{
    return tok;
}

int Token::getColumn()
{
    return col;
}

int Token::getLine()
{
    return line;
}

bool Token::isSingle()
{
    return id == SINGLE;
}

token_type Token::getType()
{
    return type;
}