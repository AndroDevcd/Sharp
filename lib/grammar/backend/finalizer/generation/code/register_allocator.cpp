//
// Created by bknun on 8/11/2022.
//

#include "register_allocator.h"
#include "code_context.h"
#include "../../../types/types.h"

void setup_register_alloc() {
    cc.registers.add(new internal_register(ADX, normal_register, status_free));
    cc.registers.add(new internal_register(CX, normal_register, status_free));
    cc.registers.add(new internal_register(CMT, normal_register, status_free));

    // compiler allocated register
    cc.registers.add(new internal_register(EBX, normal_register, status_busy));

    cc.registers.add(new internal_register(ECX, normal_register, status_free));
    cc.registers.add(new internal_register(ECF, normal_register, status_free));
    cc.registers.add(new internal_register(EDF, normal_register, status_free));
    cc.registers.add(new internal_register(EHF, normal_register, status_free));
    cc.registers.add(new internal_register(BMR, normal_register, status_free));
    cc.registers.add(new internal_register(EGX, normal_register, status_busy));
}

internal_register* allocate_register() {
    for(Int i = 0; i < cc.registers.size(); i++) {
        internal_register *ir = cc.registers.get(i);

        if(ir->status == status_free) {
            ir->status = status_busy;
            return ir;
        }
    }

    cc.registers.add(new internal_register(
            (check_flag(cc.container->flags, flag_static) ? 0 : 1) +
            cc.container->locals.size() + (cc.registers.size() - MIN_REGISTERS),
            stack_register, status_busy));

    return cc.registers.last();
}

void release_register(internal_register *r) {
    r->status = status_free;
}

internal_register* get_register(Int address) {
    return cc.registers.get(address);
}
