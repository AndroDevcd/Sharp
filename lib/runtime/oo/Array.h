//
// Created by bknun on 2/18/2017.
//

#ifndef SHARP_ARRAY_H
#define SHARP_ARRAY_H

#include <cstdint>

class gc_object;

class ArrayObject {
public:
    int64_t len;
    gc_object* arry;
};


#endif //SHARP_ARRAY_H
