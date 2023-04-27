//
// Created by bknun on 9/16/2022.
//

#ifndef SHARP_VM_STACK_H
#define SHARP_VM_STACK_H

#include "../../core/opcode/opcode.h"
#include "../../util/KeyPair.h"
#include "sharp_object.h"

struct fiber;

struct stack_item {
    double var;
    object obj;
};

struct frame {
    int32_t returnAddress;
    opcode_instr *pc;
    stack_item* sp;
    stack_item* fp;
};

CXX11_INLINE void init_struct(stack_item *item);
CXX11_INLINE void init_struct(
        frame *f,
        int32_t raddr,
        opcode_instr *ip,
        stack_item* sp,
        stack_item* fp
);

std::list<KeyPair<Int, Int>> get_frame_info(fiber *task);

#endif //SHARP_VM_STACK_H