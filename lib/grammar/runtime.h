//
// Created by BraxtonN on 1/30/2017.
//

#ifndef SHARP_RUNTIME_H
#define SHARP_RUNTIME_H

#include "../../stdimports.h"
#include "parser/parser.h"

class runtime
{
public:
    runtime(string out, list<parser*> parsers)
    :
            parsers(parsers),
            out(out)
    {
        interpret();
    }

private:
    list<parser*> parsers;
    string out;

    void interpret();

    bool preprocess();
};

#endif //SHARP_RUNTIME_H
