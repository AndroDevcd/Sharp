//
// Created by bknun on 9/18/2022.
//

#ifndef SHARP_MEMORY_HELPERS_H
#define SHARP_MEMORY_HELPERS_H

#include "../../../stdimports.h"
#include "garbage_collector.h"
#include "../virtual_machine.h"
#include "../error/vm_exception.h"

template<class T>
T* malloc_mem(uInt bytes, bool unsafe = true)
{
    T* ptr =nullptr;
    reserve_bytes(bytes, unsafe);

    ptr=(T*)malloc(bytes);
    if(ptr == nullptr) {
        release_bytes(bytes);
        throw vm_exception(vm.out_of_memory_except, "out of memory");
    }

    return ptr;
}

template<class T>
T* malloc_struct(uInt bytes, uInt size, bool unsafe = true) {
    T* data = malloc_mem<T>(bytes, unsafe);

    for(uInt i = 0; i < size; i++)
        init_struct(&data[i]);
    return data;
}

template<class T>
T* realloc_mem(void *ptr, uInt bytes, uInt prevSize, bool unsafe = true)
{
    void* rmap =nullptr;
    Int size = bytes - prevSize;

    if(size < 0) {
        release_bytes(size);
    } else reserve_bytes(size, unsafe);

    rmap=realloc(ptr, bytes);
    if(rmap == nullptr) {
        if(size < 0) {
            reserve_bytes(size, unsafe);
        } else release_bytes(size);
        throw vm_exception(vm.out_of_memory_except, "out of memory");
    }

    return (T*)ptr;
}


#endif //SHARP_MEMORY_HELPERS_H
