//
// Created by BraxtonN on 11/5/2019.
//

#ifndef SHARP_REFRENCEPOINTER_H
#define SHARP_REFRENCEPOINTER_H

#include "../../../stdimports.h"
#include "../List.h"

class ReferencePointer {
public:
    ReferencePointer()
    :
        mod("")
    {
        classes.init();
    }

    void free() {
        classes.free();
    }

    string mod;
    List<string> classes;
};

enum utype_rules {
    ut_self,
    ut_base,
    ut_fields,
    ut_methods,
    ut_classes,
};


#endif //SHARP_REFRENCEPOINTER_H
