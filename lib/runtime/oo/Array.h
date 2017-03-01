//
// Created by bknun on 2/18/2017.
//

#ifndef SHARP_ARRAY_H
#define SHARP_ARRAY_H

#include "../../../stdimports.h"

class gc_object;

class ArrayObject {
public:
    ArrayObject()
    :
            len(0),
            arry(NULL)
    {
    }

    ArrayObject(int64_t max, int type);
    ArrayObject(int64_t max);

    gc_object* get(int64_t);

    int64_t len;

    void free();

private:
    gc_object* arry;
};


#endif //SHARP_ARRAY_H
