//
// Created by braxtonn on 11/14/2019.
//

#ifndef SHARP_EXPRESSION_H
#define SHARP_EXPRESSION_H

#include "../../../stdimports.h"

class Utype;
class Ast;

enum expression_type
{
    exp_null,
    exp_var,
    exp_object,
    exp_class,
    exp_undefined
};

class Expression {
public:
    Expression()
    {
        init();
    }

    ~Expression() {
        free();
    }

    void init();
    void free();

    Utype* utype;
    expression_type type;
    bool lValue;
    Ast* ast;
};


#endif //SHARP_EXPRESSION_H
