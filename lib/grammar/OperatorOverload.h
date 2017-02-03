//
// Created by BraxtonN on 2/3/2017.
//

#ifndef SHARP_OPERATOROVERLOAD_H
#define SHARP_OPERATOROVERLOAD_H

#include "Method.h"

enum _operator {
    op_PLUS,
    op_MINUS,
    op_MULT,
    op_DIV,
    op_MOD,
    op_INC,
    op_DEC,
    op_EQUALS,
    op_EQUALS_EQ,
    op_PLUS_EQ,
    op_MIN_EQ,
    op_MULT_EQ,
    op_DIV_EQ,
    op_AND_EQ,
    op_OR_EQ,
    op_NOT_EQ,
    op_MOD_EQ,

    op_NO
};

class OperatorOverload : public Method {
public:
    OperatorOverload(ClassObject *klass, const list<Param> &params,
                     const list<AccessModifier> &modifiers, NativeField rtype, _operator op) : Method("$operator", klass, params, modifiers,
                                                                                        rtype) {
        this->op = op;
    }

    OperatorOverload(ClassObject* klass, list<Param> params, list<AccessModifier> modifiers,
                     ClassObject* rtype, _operator op) : Method("$operator", klass, params, modifiers, rtype) {
        this->op = op;
    }


    OperatorOverload(ClassObject* klass, list<Param> params, list<AccessModifier> modifiers,
                     string tmplName, _operator op) : Method("$operator", klass, params, modifiers, tmplName) {
        this->op = op;
    }

    _operator getOperator() { return op; }

private:
    _operator op;
};

#endif //SHARP_OPERATOROVERLOAD_H
