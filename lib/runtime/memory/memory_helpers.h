//
// Created by bknun on 9/18/2022.
//

#ifndef SHARP_MEMORY_HELPERS_H
#define SHARP_MEMORY_HELPERS_H

#include "../../../stdimports.h"

template<class T>
T* malloc_struct(uInt bytes, uInt size, bool unsafe = false);
template<class T>
T* malloc_mem(uInt bytes, bool unsafe = false);


#endif //SHARP_MEMORY_HELPERS_H
