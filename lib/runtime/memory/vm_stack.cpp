//
// Created by bknun on 4/20/2023.
//
#include "vm_stack.h"
#include "../multitasking/fiber/fiber.h"
#include "../error/vm_exception.h"
#include "../types/sharp_function.h"
#include "../../core/opcode/opcode_macros.h"
#include "../virtual_machine.h"

void init_struct(stack_item *item) {
    item->obj.o = NULL;
    item->var=0;
}

void init_struct(frame *f, int32_t raddr, opcode_instr *ip, Int sp, Int fp) {
    f->returnAddress=raddr;
    f->pc=ip;
    f->sp=sp;
    f->fp=fp;
}

std::list<KeyPair<Int, Int>> get_frame_info(fiber *task) {
    std::list<KeyPair<Int, Int>> info;

    if((task->callFramePtr + 1) < EXCEPTION_PRINT_MAX) {
        for (Int i = 0; i <= task->callFramePtr; i++) {
            if(i >= EXCEPTION_PRINT_MAX) break;

            info.emplace_back(task->frames[i].returnAddress, task->frames[i].pc - vm.methods[task->frames[i].returnAddress].bytecode);
        }
    } else {

        for (Int i = (task->callFramePtr + 1) - EXCEPTION_PRINT_MAX; i < task->callFramePtr; i++) {
            if(info.size() >= EXCEPTION_PRINT_MAX) break;
            else if(i == 0) continue;

            info.emplace_back(task->frames[i].returnAddress, task->frames[i].pc -  vm.methods[task->frames[i].returnAddress].bytecode);
        }
    }

    info.emplace_back(task->current->address, current_pc);
    return info;
}
