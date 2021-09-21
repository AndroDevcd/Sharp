//
// Created by BNunnally on 9/9/2021.
//
#include "unresolved_type.h"
#include "sharp_type.h"
#include "sharp_function.h"
#include "../operation/operation.h"

void unresolved_item::free() {
    typeSpecifiers.free();
    operations.free();
    delete returnType;
}

void unresolved_item::copy(List<sharp_type> &types) {
    for(Int i = 0; i < types.size(); i++) {
        typeSpecifiers.add(new sharp_type(types.get(i)));
    }
}

void unresolved_item::copy(const unresolved_item &item) {
    type = item.type;
    name = item.name;

    for(Int i = 0; i < item.typeSpecifiers.size(); i++) {
        typeSpecifiers.add(new sharp_type(*item.typeSpecifiers.get(i)));
    }

    operations.addAll(item.operations);
    accessType = item.accessType;
    ast = item.ast;
    if(item.returnType) {
        returnType = new sharp_type(*item.returnType);
    }
}

string access_type_to_str(reference_access_type type) {
    switch(type) {
        case access_normal:
            return ".";
        case access_safe:
            return "?.";
        case access_forced:
            return "!!.";
        default:
            return "?";
    }
}

