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
    tokenizer(const string tokens, const string file)
            :
            toks(""),
            empty(""),
            len(tokens.size()),
            cursor(0),
            col(0),
            line(1),
            file(file)
    {
        entites.init();
        lines.init();

        if(!tokens.empty()) {
            toks = tokens;
        } else {
            toks = "";
            len = 0;
        }

        parse();
    }

    unsigned long getEntityCount();
    List<token_entity>& getEntities();
    ErrorManager* getErrors();
    List<string>& getLines();
    string &getData();

    string file;
    token_entity* EOF_token;
    void free();

private:
    void parse();
    void parse_lines();
    bool ismatch(char i, char current);
    string get_escaped_string(string msg) const;

    List<token_entity> entites;
    ErrorManager* errors;
    List<string> lines;
    string toks, empty;
    unsigned long len;
    unsigned long cursor;
    int line;
    int col;
};

#endif //SHARP_TOKENIZER_H
