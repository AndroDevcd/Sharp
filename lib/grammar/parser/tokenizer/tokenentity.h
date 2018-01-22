//
// Created by bknun on 11/13/2017.
//

#ifndef SHARP_TOKENENTITY_H
#define SHARP_TOKENENTITY_H


#include "../../../../stdimports.h"
#include "tokentype.h"

enum token_entity_id
{
    IDENTIFIER,
    NATIVE_TYPE,
    TYPE_IDENTIFIER,
    INTEGER_LITERAL,
    HEX_LITERAL,
    MODULENAME,
    STRING_LITERAL,
    CHAR_LITERAL,
    LITERAL,
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

    token_entity()
            :
            tok(""),
            type(NONE),
            id(NOENTITY),
            col(0),
            line(0)
    {
    }
    ~token_entity()
    {
        tok.clear();
    }

    bool operator!=(const token_entity_id& id)
    {
        return this->id == id;
    }
    void operator=(const token_entity& entity)
    {
        tok = entity.tok;
        line = entity.line;
        col = entity.col;
        id = entity.id;
        type = entity.type;
    }
    bool operator!=(const token_type& ty)
    {
        return this->type == ty;
    }
    bool operator==(const string& s)
    {
        return this->tok == s;
    }
    bool operator!=(const string& s)
    {
        return this->tok != s;
    }

    token_entity_id getId();
    void setId(token_entity_id id);
    string getToken();
    int getColumn();
    int getLine();
    bool isSingle();
    token_type getTokenType();

private:
    int line;
    int col;
    token_entity_id id;
    token_type type;
    string tok;
};


#endif //SHARP_TOKENENTITY_H
