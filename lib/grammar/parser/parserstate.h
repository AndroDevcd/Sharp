//
// Created by BraxtonN on 1/25/2017.
//

#ifndef SHARP_PARSERSTATE_H
#define SHARP_PARSERSTATE_H

#include "ast.h"

struct parser_state
{
public:
    parser_state()
            :
            rAst(NULL),
            rCursor(0),
            rAstcursor(0)
    {
    }

    ast* rAst;
    int64_t  rCursor;
    int64_t rAstcursor;
};

#endif //SHARP_PARSERSTATE_H
