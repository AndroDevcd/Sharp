//
// Created by bknun on 4/20/2023.
//
#include "vm_stack.h"
#include "../multitasking/fiber/fiber.h"
#include "../error/vm_exception.h"

void init_struct(stack_item *item) {
    item->obj.o = NULL;
    item->var=0;
}

void init_struct(frame *f, int32_t raddr, opcode_instr *ip, stack_item* sp, stack_item* fp) {
    f->returnAddress=raddr;
    f->pc=ip;
    f->sp=sp;
    f->fp=fp;
}

std::list<KeyPair<Int, Int>> get_frame_info(fiber *task) {
    std::list<KeyPair<Int, Int>> info;

    if((task->calls + 1) < EXCEPTION_PRINT_MAX) {
        for (Int i = 1; i <= task->calls; i++) {
            if(i >= EXCEPTION_PRINT_MAX) break;

            info.emplace_back(task->frames[i].returnAddress, task->frames[i].pc - task->rom);
        }
    } else {

        for (Int i = (task->calls + 1) - EXCEPTION_PRINT_MAX; i <= task->calls; i++) {
            if(i >= EXCEPTION_PRINT_MAX) break;
            else if(i == 0) continue;

            info.emplace_back(task->frames[i].returnAddress, task->frames[i].pc - task->rom);
        }
    }

    return info;
}
