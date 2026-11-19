//
// Created by bknun on 9/16/2022.
//

#ifndef SHARP_VM_STACK_H
#define SHARP_VM_STACK_H

#include "../../core/opcode/opcode.h"
#include "../../util/KeyPair.h"
#include "sharp_object.h"

struct fiber;

/**
 * One slot of a fiber's stack: a local variable, an argument, or a temporary.
 * A slot has both halves; a number uses `var`, a reference uses `obj`.
 *
 * The reference half is what the garbage collector scans (every slot up to sp,
 * memory/tracing_collector.cpp): it is always an object or null, which is why
 * the collector needs no map of "which slots hold references".
 */
struct stack_item {
    vm_cell var;    // a number: an integer or a decimal, see vm_cell in stdimports.h
    object obj;
};

struct frame {
    int32_t returnAddress;
    opcode_instr *pc;
    Int sp;
    Int fp;
    bool native;
    // todo: add bool for is call is native
};

void init_struct(stack_item *item);
void init_struct(
        frame *f,
        int32_t raddr,
        opcode_instr *ip,
        Int sp,
        Int fp,
        bool native
);

std::list<KeyPair<Int, Int>> get_frame_info(fiber *task);

#endif //SHARP_VM_STACK_H
