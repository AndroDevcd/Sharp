//
// Created by bknun on 9/16/2022.
//

#ifndef SHARP_VM_STACK_H
#define SHARP_VM_STACK_H

#include "../../core/opcode/opcode.h"
#include "../../util/KeyPair.h"

struct sharp_object;
struct fiber;

struct stack_item {
    double var;
    sharp_object *obj;
};

struct frame {
    int32_t returnAddress;
    opcode_instr *pc;
    int32_t sp;
    int32_t fp;
};

void init_struct(
        frame *f,
        int32_t raddr,
        opcode_instr *ip,
        int32_t sp,
        int32_t fp
);

std::list<KeyPair<Int, Int>> get_frame_info(fiber *task);

#endif //SHARP_VM_STACK_H
