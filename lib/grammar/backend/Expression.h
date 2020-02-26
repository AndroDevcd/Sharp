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
    void freeInjectors();
    void copyInjectors(Utype *utype);
    void copy(Expression *expr);

    Utype* utype;
    expression_type type;
    Ast* ast;
};


#endif //SHARP_EXPRESSION_H
