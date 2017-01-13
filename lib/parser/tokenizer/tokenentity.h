//
// Created by bknun on 1/6/2017.
//

#ifndef SHARP_TOKENENTITY_H
#define SHARP_TOKENENTITY_H

#include "../../../stdimports.h"
#include "tokentype.h"

enum token_entity_id
{
    IDENTIFIER,
    NATIVE_TYPE,
    TYPE_IDENTIFIER,
    NUMBER_LITERAL,
    MODULENAME,
    STRING_LITERAL,
    CHAR_LITERAL,
    LITERAL,
    BOOLEAN_LITERAL,
    VALUE,
    ACCESSTYPE,
    SINGLELINE_COMMENT,
    RETURN_STATEMENT,

    SINGLE, // single token
    NOENTITY // not an entity
};

/**
 * Token entity is a class that represents a single entity (not a group of token types).
 * For instance a token entity could be an identifier or native type but not a module name.
 */
class token_entity
{
public:
    token_entity(string val, token_entity_id tokenid, int col, int line, token_type type = NONE)
    :
            tok(val),
            type(type),
            id(tokenid),
            col(col),
            line(line)
    {
    }

    bool operator!=(const token_entity_id& id)
    {
        return this->id == id;
    }

    bool operator!=(const token_type& ty)
    {
        return this->type == ty;
    }

    token_entity_id getid();
    void setid(token_entity_id id);
    string gettoken()const;
    int getcolumn()const;
    int getline()const;
    bool issingle();
    token_type gettokentype()const;

private:
    const int line;
    const int col;
    token_entity_id id;
    const token_type type;
    const string tok;
};

class entity_group
{

};

#endif //SHARP_TOKENENTITY_H
