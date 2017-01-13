//
// Created by bknun on 1/6/2017.
//

#ifndef SHARP_PARSEERRORS_H
#define SHARP_PARSEERRORS_H

#include <list>
#include "../../stdimports.h"
#include "../util/keypair.h"
#include "tokenizer/tokenentity.h"

enum p_errors
{
    UNEXPECTED_SYMBOL = 0,
    ILLEGAL_NUMBER_FORMAT = 1,
    UNEXPECTED_EOF = 2,
    EXPECTED_STRING_LITERAL_EOF = 3,
    ILLEGAL_STRING_FORMAT = 4,
    EXPECTED_CHAR_LITERAL_EOF = 5,
    ILLEGAL_CHAR_LITERAL_FORMAT = 6,
    GENERIC = 7,
    ILLEGAL_ACCESS_DECLARATION = 8,
    ILLEGAL_BRACKET_MISMATCH = 9,
};

static list<keypair<p_errors, string>> predefined_errs;
void int_errs();

struct parseerror
{
public:
    parseerror()
    {
    }

    parseerror(keypair<p_errors, string> err, int l, int c, string addon = "")
    {
        id = err.key;
        error = (err.value + addon);
        line = l;
        col = c;
    }

    parseerror(keypair<p_errors, string> err, token_entity token, string addon = "")
    {
        id = err.key;
        error = (err.value + addon);
        line = token.getline();
        col = token.getcolumn();
    }

    p_errors id;
    string error;
    int line;
    int col;
};

class Errors
{
public:
    Errors(list<string>* lines)
    :
            lines(lines)
    {
        errors = new list<parseerror>();
    }

    string getall_errors();
    void newerror(p_errors err, token_entity token, string xcmts = "");
    void newerror(p_errors err, int l, int c, string xcmts = "");
    bool _errs();

private:
    string getline(int line);
    keypair<p_errors, string> geterrorbyid(p_errors);

    list<string>* lines;
    list<parseerror>* errors;
    parseerror lasterr;
    bool _err;
};

#endif //SHARP_PARSEERRORS_H
