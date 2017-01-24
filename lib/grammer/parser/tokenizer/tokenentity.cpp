//
// Created by bknun on 1/6/2017.
//
#include "tokenentity.h"


token_entity_id token_entity::getid() {
    return id;
}

void token_entity::setid(token_entity_id id) {
    this->id = id;
}

string token_entity::gettoken() {
    return tok;
}

int token_entity::getcolumn() {
    return col;
}

int token_entity::getline() {
    return line;
}

bool token_entity::issingle()
{
    return id == SINGLE;
}

token_type token_entity::gettokentype() {
    return type;
}
