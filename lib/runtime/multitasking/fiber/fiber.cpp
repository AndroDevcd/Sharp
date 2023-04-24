//
// Created by BNunnally on 9/5/2020.
//
#include "fiber.h"
#include "../../memory/garbage_collector.h"
#include "task_controller.h"
#include "../../memory/sharp_object.h"
#include "../thread/sharp_thread.h"
#include "../../memory/memory_helpers.h"

void fiber::free() {
    release_all_mutexes(this);

    if(stack != NULL) {
        release_bytes(sizeof(stack_item) * stackSize);
        stack_item *p = stack;
        for(size_t i = 0; i < stackSize; i++)
        {
            if(p->obj.o) {
                dec_ref(p->obj.o);
                p->obj.o=NULL;
            }
            p++;
        }

        std::free(this->stack); stack = NULL;
    }

    if(registers != NULL) {
        release_bytes(sizeof(double) * REGISTER_SIZE);
        std::free(registers); registers = NULL;
    }

    if(frames != NULL) {
        release_bytes(sizeof(frame) * frameSize);
        std::free(frames); frames = NULL;
    }

    copy_object(&fiberObject, (sharp_object*)nullptr);
    copy_object(&exceptionObject, (sharp_object*)nullptr);
    fp = NULL;
    sp = NULL;
    name.clear();
    id = -1;
}

void fiber::growFrame() {
    GUARD(mut)

    if(frameSize + FRAME_GROW_SIZE < frameLimit) {
        frames = realloc_mem<frame>(
                frames, sizeof(frame) * (frameSize + FRAME_GROW_SIZE),
                sizeof(frame) * frameSize
        );

        frameSize += FRAME_GROW_SIZE;
    }
    else if(frameSize != frameLimit) {
        frames = realloc_mem<frame>(
                frames, sizeof(frame) * (frameLimit),
                sizeof(frame) * frameSize
        );

        frameSize = frameLimit;
    }
    
}

void fiber::growStack(Int requiredSize) {
    GUARD(mut)
    if(requiredSize < STACK_GROW_SIZE)
      requiredSize = STACK_GROW_SIZE;
    else requiredSize += STACK_GROW_SIZE;

    if(stackSize + requiredSize < stackLimit) {
        Int spIdx = sp-stack, fpIdx = fp-stack, ptrIdx = -1;
        if(ptr) {
            for(Int i = 0; i < stackSize; i++) {
                if(ptr == &stack[i].obj)
                {
                    ptrIdx = i;
                    break;
                }
            }
        }

        stack = realloc_mem<stack_item>(
                frames, sizeof(stack_item) * (stackSize + requiredSize),
                sizeof(stack_item) * stackSize
        );

        stack_item *stackItem = stack+stackSize;
        for(Int i = stackSize; i < stackSize+requiredSize; i++) {
            stackItem->obj.o = NULL;
            stackItem->var=0;
            stackItem++;
        }

        if(ptrIdx >= 0) ptr = &stack[ptrIdx].obj;
        sp = stack+spIdx;
        fp = stack+fpIdx;
        stackSize += requiredSize;
    }
    else if(stackSize != stackLimit) {
        Int spIdx = sp-stack, fpIdx = fp-stack, ptrIdx = -1;
        if(ptr) {
            for(Int i = 0; i < stackSize; i++) {
                if(ptr == &stack[i].obj)
                {
                    ptrIdx = i;
                    break;
                }
            }
        }

        stack = realloc_mem<stack_item>(
                frames, sizeof(stack_item) * (stackLimit),
                sizeof(stack_item) * stackSize
        );

        stack_item *stackItem = stack+stackSize;
        for(Int i = stackSize; i < stackLimit; i++) {
            stackItem->obj.o = NULL;
            stackItem->var=0;
            stackItem++;
        }

        if(ptrIdx >= 0) ptr = &stack[ptrIdx].o;
        sp = stack+spIdx;
        fp = stack+fpIdx;
        stackSize = stackLimit;
    }

}

