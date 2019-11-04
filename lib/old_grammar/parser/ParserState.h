//
// Created by BraxtonN on 1/22/2018.
//

#ifndef SHARP_PARSERSTATE_H
#define SHARP_PARSERSTATE_H

#include "Ast.h"

class ParserState {
public:
    ParserState()
            :
            ast(NULL),
            cursor(0),
            astCursor(0)
    {
    }

    ParserState(Ast* pAst, int64_t cursor, int64_t astCursor)
    :
    copy(pAst->getParent(), pAst),
    ast(pAst),
    cursor(cursor),
    astCursor(astCursor)
    {
    }

    Ast *ast, copy;
    int64_t  cursor;
    int64_t astCursor;
};


#endif //SHARP_PARSERSTATE_H
