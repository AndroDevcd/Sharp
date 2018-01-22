//
// Created by bknun on 11/13/2017.
//

#ifndef SHARP_TOKENIZER_H
#define SHARP_TOKENIZER_H

#include <list>
#include "../../../../stdimports.h"
#include "tokenentity.h"
#include "../ErrorManager.h"
#include "../../List.h"

class tokenizer
{
public:
    tokenizer(string tokens, string file)
            :
            toks(tokens),
            cursor(0),
            col(0),
            line(1),
            file(file)
    {
        entites.init();
        lines = new list<string>();

        parse();
    }

    unsigned long getEntityCount();
    List<token_entity>& getEntities();
    ErrorManager* getErrors();
    list<string>* getLines();

    string file;
    token_entity* EOF_token;
    void free();

private:
    void parse();
    void parse_lines();
    void scan();
    void invalidate_comments();
    void invalidate_whitespace();
    void scan_identifier();
    void scan_symbol();
    void scan_number();
    bool ismatch(char i, char current);
    void scan_stringliteral();
    void scan_characterliteral();
    string get_escaped_string(string msg) const;

    List<token_entity> entites;
    ErrorManager* errors;
    list<string> *lines;
    string toks;
public:
    const string &getData() const;

private:
    unsigned long cursor;

    int line;
    int col;

    void scan_hex();
};

#define iswhitespace(c) \
    (' '  == c) || ('\n' == c) || \
    ('\r' == c) || ('\t' == c) || \
    ('\b' == c) || ('\v' == c) || \
    ('\f' == c) \

#define current() \
    (cursor >= toks.length()) ? \
        toks.at(toks.length()-1) \
    : toks.at(cursor); \

#define newline() \
    col = 0, line++;

#define isend() \
    cursor>=toks.length()

#define peekend(forward) \
    (cursor+forward)>=toks.length()

#define peek(forward) \
    ((cursor+forward) >= toks.length() || (cursor+forward) < 0) ? \
        toks.at(toks.length()-1) : toks.at(cursor+forward)

#define issymbol(c) \
    ('+' == c) || ('-' == c) || \
    ('*' == c) || ('/' == c) || \
    ('^' == c) || ('<' == c) || \
    ('>' == c) || ('=' == c) || \
    (',' == c) || ('!' == c) || \
    ('(' == c) || (')' == c) || \
    ('[' == c) || (']' == c) || \
    ('{' == c) || ('}' == c) || \
    ('%' == c) || (':' == c) || \
    ('?' == c) || ('&' == c) || \
    ('|' == c) || (';' == c) || \
    ('!' == c) || ('.' == c) || \
    ('#' == c) || ('$' == c)

#define isnumber(c) \
    isdigit(c)

#define isletter(c) \
    isalpha(c)

#define ishexnum(c) \
    isdigit(c) || (c >= 65 && c <= 72) || \
        (c >= 97 && c <= 104)

#define advance() \
    col++; cursor++;

#define tokensLeft() \
    toks.length() - cursor

#define issign(s) \
    s == '+' || s == '-'

#endif //SHARP_TOKENIZER_H
