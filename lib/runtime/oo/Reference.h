//
// Created by BraxtonN on 2/28/2017.
//

#ifndef SHARP_REFRENCE_H
#define SHARP_REFRENCE_H

#include "Exception.h"
#include "Object.h"

class gc_object;

class Reference {

public:
    Reference()
    :
            object(NULL)
    {
    }

    void operator=(const Reference &r) {
        if(object != NULL) {
            object->inv_reference(this);
        }

        object = NULL;
        if(r.object != NULL) {
            add(r.object);
        }
    }

    void add(gc_object* obj);

    CXX11_INLINE
    void notify() {
        // reference invalid
        object = NULL;
    }

    CXX11_INLINE
    gc_object* get_unsafe() {
        return object;
    }

    gc_object* get() {
        if(object == NULL)
            throw Exception("access to null reference");
        return object;
    }

private:
    gc_object* object;
};


#endif //SHARP_REFRENCE_H
