//
// Created by braxtonn on 11/14/2019.
//

#include "Expression.h"
#include "data/Utype.h"

void Expression::init() {
    type = exp_undefined;
    utype = new Utype();
}

void Expression::free() {
    if(utype) {
        utype->softFree();
        utype = NULL;
    }
}

void Expression::freeInjectors() {
    utype->getCode().getInjector(ptrInjector).free();
    utype->getCode().getInjector(stackInjector).free();
    utype->getCode().getInjector(ebxInjector).free();
    utype->getCode().getInjector(getterInjector).free();
}

void Expression::copy(Expression *expr) {
    type = expr->type;
    ast = expr->ast;
    utype->copy(expr->utype);
}

void Expression::copyInjectors(Utype *utype) {
    this->utype->getCode().getInjector(stackInjector).free()
        .inject(utype->getCode().getInjector(stackInjector));
    this->utype->getCode().getInjector(ebxInjector).free()
            .inject(utype->getCode().getInjector(ebxInjector));
    this->utype->getCode().getInjector(ptrInjector).free()
            .inject(utype->getCode().getInjector(ptrInjector));
    this->utype->getCode().getInjector(getterInjector).free()
            .inject(utype->getCode().getInjector(getterInjector));
}
