//
// Created by bknun on 2/18/2017.
//

#include "Array.h"
#include "../internal/Environment.h"
#include "../oo/Object.h"

ArrayObject::ArrayObject(int64_t max, int type) {
    len = max;
    this->arry = NULL;//new gc_object[max];
    for(int64_t i = 0; i < max; i++) {
        this->arry[i] = gc_object((Type)type);
    }

}

void ArrayObject::free() {
    if(len != 0) {
        for(int64_t i = 0; i < len; i++) {
            this->arry[i].free();
        }

        len = 0;
        std::free (this->arry); arry = NULL;
    }
}

gc_object* ArrayObject::get(int64_t x) {
    if(x >= len) {
        stringstream ss;
        ss << "Invalid index " << x << ", size is " << len;
        throw Exception(&Environment::IndexOutOfBoundsException, ss.str());
    }
    return &this->arry[x];
}

ArrayObject::ArrayObject(int64_t max) {
    len = max;
    this->arry = NULL;//new gc_object[max];
    for(int64_t i = 0; i < max; i++) {
        this->arry[i] = gc_object();
    }
}
