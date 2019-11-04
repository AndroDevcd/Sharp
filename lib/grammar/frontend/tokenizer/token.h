//
// Created by bknun on 11/13/2017.
//

#ifndef SHARP_TOK_H
#define SHARP_TOK_H


#include "../../../../stdimports.h"
#include "tokentype.h"

enum token_id
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
class Token
{
public:
    Token(string val, token_id tokenid, int col, int line, token_type type = NONE)
            :
            tok(val),
            type(type),
            id(tokenid),
            col(col),
            line(line)
    {
    }

    Token()
            :
            tok(""),
            type(NONE),
            id(NOENTITY),
            col(0),
            line(0)
    {
    }
    ~Token()
    {
        tok.clear();
    }

    bool operator!=(const token_id& id)
    {
        return this->id != id;
    }
    void operator=(const Token& token)
    {
        tok = token.tok;
        line = token.line;
        col = token.col;
        id = token.id;
        type = token.type;
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

    token_id getId();
    void setId(token_id id);
    string getToken();
    int getColumn();
    int getLine();
    bool isSingle();
    token_type getType();

private:
    int line;
    int col;
    token_id id;
    token_type type;
    string tok;
};


#endif //SHARP_TOKEN_H
