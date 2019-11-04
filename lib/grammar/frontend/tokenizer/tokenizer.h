//
// Created by bknun on 11/13/2017.
//

#ifndef SHARP_TOKENIZER_H
#define SHARP_TOKENIZER_H

#include <list>
#include "../../../../stdimports.h"
#include "token.h"
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
        this->tokens.init();
        lines.init();

        if(!tokens.empty()) {
            toks = tokens;
            data = tokens.c_str();
        } else {
            toks = "";
            data = NULL;
            len = 0;
        }

        parse();
    }

    unsigned long getEntityCount();
    List<Token>& getTokens();
    ErrorManager* getErrors();
    List<string>& getLines();
    string &getData();
    long long size() { return tokens.size(); }

    string file;
    Token* EOF_token;
    void free();

private:
    void parse();
    void parse_lines();
    bool ismatch(char i, char current);
    string get_escaped_string(string msg) const;

    List<Token> tokens;
    ErrorManager* errors;
    List<string> lines;
    string toks, empty;
    const char* data;
    unsigned long len;
    unsigned long cursor;
    int line;
    int col;
};

#endif //SHARP_TOKENIZER_H
