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
            errors(NULL),
            EOF_token(NULL),
            len(tokens.size()),
            cursor(0),
            col(0),
            line(1),
            file(file),
            dynamicString(false),
            brackets(0)
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

    ~tokenizer() {
        free();
    }

    unsigned long getEntityCount();
    List<Token>& getTokens();
    ErrorManager* getErrors();
    List<string>& getLines();
    string &getData();
    Int size() { return tokens.size(); }

    string file;
    Token* EOF_token;
    void free();

    static string tokenTypeToString(token_type);
    static string from_escaped_string(string msg);
    static string to_escaped_string(string msg);

private:
    void parse();
    void parse_lines();
    bool ismatch(char i, char current);

    List<Token> tokens;
    ErrorManager* errors;
    List<string> lines;
    string toks, empty;
    const char* data;
    uInt len;
    uInt cursor;
    int line;
    int col;
    bool dynamicString;
    long brackets;

    CXX11_INLINE void saveString(const stringstream &message, bool escaped_found);

    CXX11_INLINE void parseIdentifier();
};

#endif //SHARP_TOKENIZER_H
