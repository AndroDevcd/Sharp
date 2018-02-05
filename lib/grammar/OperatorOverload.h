//
// Created by BraxtonN on 2/5/2018.
//

#ifndef SHARP_OPERATOROVERLOAD_H
#define SHARP_OPERATOROVERLOAD_H


#include "Method.h"

enum Operator {
    oper_PLUS,
    oper_MINUS,
    oper_MULT,
    oper_DIV,
    oper_MOD,
    oper_INC,
    oper_DEC,
    oper_EQUALS,
    oper_EQUALS_EQ,
    oper_PLUS_EQ,
    oper_MIN_EQ,
    oper_MULT_EQ,
    oper_DIV_EQ,
    oper_AND_EQ,
    oper_OR_EQ,
    oper_NOT_EQ,
    oper_MOD_EQ,
    oper_NOT,
    oper_SHL,
    oper_SHR,
    oper_LESSTHAN,
    oper_GREATERTHAN,
    oper_LTEQ,
    oper_GTEQ,

    oper_UNDEFINED
};

class OperatorOverload : public Method {
public:
    OperatorOverload(RuntimeNote note, ClassObject *klass, List<Param> &params,
                     List<AccessModifier> &modifiers, FieldType rtype, Operator op, long long sourceFile
            , string oper) : Method("$operator"+oper, "",klass, params, modifiers,
                                    rtype, note, sourceFile) {
        this->op = op;
    }

    OperatorOverload(RuntimeNote note, ClassObject* klass, List<Param> &params, List<AccessModifier> &modifiers,
                     ClassObject* rtype, Operator op, long long sourceFile, string oper) : Method("$operator"+oper, "", klass, params, modifiers, rtype, note, sourceFile) {
        this->op = op;
    }

    OperatorOverload()
            : Method() {
        op = oper_UNDEFINED;
    }

    Operator getOperator() { return op; }

private:
    Operator op;
};

#endif //SHARP_OPERATOROVERLOAD_H
