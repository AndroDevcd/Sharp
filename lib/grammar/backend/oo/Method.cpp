//
// Created by BraxtonN on 10/22/2019.
//

#include "Method.h"
#include "../Compiler.h"

void Method::free() {
    release();
    Compiler::freeListPtr(params);
    if(utype) {
        utype->free();
        delete utype;
    }
}

string Method::toString() {
    stringstream ss;
    ss << "fn ";
    if(fnType == fn_delegate_impl || fnType == fn_delegate)
        ss << "delegate::";
    else if(fnType == fn_prototype)
        ss << "*";
    ss << fullName << paramsToString(params);

    if(utype) {
        ss << " : " << utype->toString();
    } else {
        ss << " : <untyped>";
    }
    return ss.str();
}

string Method::paramsToString(List<Field *> &params) {
    stringstream ss;
    ss << "(";
    for(long long i = 0; i < params.size(); i++) {
        ss << params.get(i)->toString();
        if((i+1) < params.size()) {
            ss << ", ";
        }
    }
    ss << ")";
    return ss.str();
}
