//
// Created by bknun on 8/11/2022.
//

#include "register_allocator.h"
#include "code_context.h"
#include "../../../types/types.h"
#include "../generator.h"

void setup_register_alloc() {
    // compiler allocated register
    cc.registers.add(new internal_register(-1, ADX, normal_register, status_busy));

    cc.registers.add(new internal_register(-1, CX, normal_register, status_free));

    // compiler allocated register
    cc.registers.add(new internal_register(-1, CMT, normal_register, status_busy));

    // compiler allocated register
    cc.registers.add(new internal_register(-1, EBX, normal_register, status_busy));

    cc.registers.add(new internal_register(-1, ECX, normal_register, status_free));
    cc.registers.add(new internal_register(-1, ECF, normal_register, status_free));
    cc.registers.add(new internal_register(-1, EDF, normal_register, status_free));
    cc.registers.add(new internal_register(-1, EHF, normal_register, status_free));

    // compiler allocated register
    cc.registers.add(new internal_register(-1, BMR, normal_register, status_busy));

    // compiler allocated register
    cc.registers.add(new internal_register(-1, EGX, normal_register, status_busy));
}

internal_register* allocate_register(Int id) {
    for(Int i = 0; i < cc.registers.size(); i++) {
        internal_register *ir = cc.registers.get(i);

        if(ir->status == status_free) {
            ir->id = id;
            ir->status = status_busy;
            return ir;
        }
    }

    cc.registers.add(new internal_register( // todo: fix values looks like it needs a +1 value added for instance functions
            id, (check_flag(cc.container->flags, flag_static) ? 0 : 1) +
            cc.container->locals.size() + (cc.registers.size() - MIN_REGISTERS),
            stack_register, status_busy));

    return cc.registers.last();
}

void release_register(internal_register *r) {
    if(r != NULL) {
        r->status = status_free;
        r->id = -1;
    } else {
        generation_error("attempt to release unknown register");
    }
}

internal_register* find_register(Int id) {
    for(Int i = 0; i < cc.registers.size(); i++) {
        internal_register *ir = cc.registers.get(i);

        if(ir->id == id) {
            return ir;
        }
    }

    return NULL;
}

internal_register* get_register(Int address) {
    return cc.registers.get(address);
}
