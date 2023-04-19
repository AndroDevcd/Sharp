//
// Created by bknun on 9/18/2022.
//

#include "memory_helpers.h"
#include "../../../stdimports.h"
#include "garbage_collector.h"
#include "../virtual_machine.h"

void* _malloc(uInt bytes)
{
    void* ptr =nullptr;
    reserve_bytes(bytes);

    ptr=malloc(bytes);
    if(ptr == nullptr) {
        throw vm_exception(vm.out_of_memory_exception, "out of memory");
    }

    return ptr;
}

template<class T>
T* malloc_struct(uInt bytes, uInt size) {
    T* data = (T*)_malloc(bytes);

    for(uInt i = 0; i < size; i++)
        init_struct(&data[i]);
    return data;
}
