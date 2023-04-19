//
// Created by bknun on 9/18/2022.
//

#ifndef SHARP_MEMORY_HELPERS_H
#define SHARP_MEMORY_HELPERS_H

#include "../../../stdimports.h"

template<class T>
T* malloc_struct(uInt bytes, uInt size);
void* _malloc(uInt bytes);

void new_class(object* o, sharp_class *sc, bool static_init = false);
void new_string_array(object *o, const char *str);
void new_object_array(object* o, Int size);



#endif //SHARP_MEMORY_HELPERS_H
