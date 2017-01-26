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

    parser_state(ast* pAst, int64_t c, int64_t ac)
            :
            rAst(pAst),
            rCursor(c),
            rAstcursor(ac)
    {
    }

    ast* rAst;
    int64_t  rCursor;
    int64_t rAstcursor;
};

#endif //SHARP_PARSERSTATE_H
