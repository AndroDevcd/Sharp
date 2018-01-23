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
        lines.init();

        parse();
    }

    unsigned long getEntityCount();
    List<token_entity>& getEntities();
    ErrorManager* getErrors();
    List<string>& getLines();

    string file;
    token_entity* EOF_token;
    void free();

private:
    void parse();
    void parse_lines();
    void scan();
    CXX11_INLINE void invalidate_comments();
    void invalidate_whitespace();
    CXX11_INLINE void scan_identifier();
    CXX11_INLINE void scan_symbol();
    CXX11_INLINE void scan_number();
    bool ismatch(char i, char current);
    CXX11_INLINE void scan_stringliteral();
    CXX11_INLINE void scan_characterliteral();
    string get_escaped_string(string msg) const;

    List<token_entity> entites;
    ErrorManager* errors;
    List<string> lines;
    string toks;
public:
    const string &getData() const;

private:
    unsigned long cursor;

    int line;
    int col;

    void scan_hex();
};

#endif //SHARP_TOKENIZER_H
