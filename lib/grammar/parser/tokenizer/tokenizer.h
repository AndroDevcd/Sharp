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
            len(tokens.size()),
            cursor(0),
            col(0),
            line(1),
            file(file)
    {
        entites.init();
        lines.init();
        EOF_token = new token_entity("", SINGLE, 0, line, _EOF);

        if(!tokens.empty()) {
            toks = new char[tokens.size()];
            strcpy(toks, tokens.c_str());
        } else {
            toks = NULL;
            len = 0;
        }

        parse();
    }

    unsigned long getEntityCount();
    List<token_entity>& getEntities();
    ErrorManager* getErrors();
    List<string>& getLines();
    const string getData() const;

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
    char* toks;
    unsigned long len;
    unsigned long cursor;
    int line;
    int col;
};

#endif //SHARP_TOKENIZER_H
