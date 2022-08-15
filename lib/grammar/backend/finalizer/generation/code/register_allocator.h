//
// Created by bknun on 8/11/2022.
//

#ifndef SHARP_REGISTER_ALLOCATOR_H
#define SHARP_REGISTER_ALLOCATOR_H

#include "../../../../../../stdimports.h"

#define MIN_REGISTERS 10

enum register_type {
    normal_register,
    stack_register
};

enum register_status {
    status_busy,
    status_free
};

struct internal_register {
    internal_register()
            :
            address(-1),
            type(normal_register),
            status(status_free)
    {}

    internal_register(int addr, register_type type, register_status status)
            :
            address(addr),
            type(type),
            status(status)
    {}

    internal_register(const internal_register &reg)
            :
            address(reg.address),
            type(reg.type),
            status(reg.status)
    {
    }

    ~internal_register() {
    }

    int address;
    register_type type;
    register_status status;
};

void setup_register_alloc();
internal_register* allocate_register();
void release_register(internal_register *);
internal_register* get_register(Int address);

#endif //SHARP_REGISTER_ALLOCATOR_H
