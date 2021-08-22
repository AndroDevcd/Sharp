//
// Created by BNunnally on 9/5/2020.
//
#include "fiber.h"
#include "Thread.h"
#include "VirtualMachine.h"
#include "scheduler/scheduler.h"
#include "scheduler/task_controller.h"

void fiber::free() {
    gc.reconcileLocks(this);

    bind_task(this, NULL);
    if(dataStack != NULL) {
        gc.freeMemory(sizeof(StackElement) * stackSize);
        StackElement *p = dataStack;
        for(size_t i = 0; i < stackSize; i++)
        {
            if(p->object.object) {
                DEC_REF(p->object.object);
                p->object.object=NULL;
            }
            p++;
        }

        std::free(this->dataStack); dataStack = NULL;
    }

    if(registers != NULL) {
        gc.freeMemory(sizeof(double) * REGISTER_SIZE);
        std::free(registers); registers = NULL;
    }

    if(callStack != NULL) {
        gc.freeMemory(sizeof(Frame) * frameSize);
        std::free(callStack); callStack = NULL;
    }

    fiberObject=(SharpObject*)NULL;
    exceptionObject=(SharpObject*)NULL;
    fp = NULL;
    sp = NULL;
    name.clear();
    id = -1;
}

void fiber::growFrame() {
    GUARD(mut)

    if(frameSize + FRAME_GROW_SIZE < frameLimit) {
        callStack = (Frame *) __realloc(callStack, sizeof(Frame) * (frameSize + FRAME_GROW_SIZE),
                                        sizeof(Frame) * frameSize);
        frameSize += FRAME_GROW_SIZE;
        gc.addMemory(sizeof(Frame) * FRAME_GROW_SIZE);
    }
    else if(frameSize != frameLimit) {
        callStack = (Frame *) __realloc(callStack, sizeof(Frame) * (frameLimit), sizeof(Frame) * frameSize);
        gc.addMemory(sizeof(Frame) * (frameLimit - frameSize));
        frameSize = frameLimit;
    }
    
}

void fiber::growStack(Int requiredSize) {
    GUARD(mut)
    if(requiredSize < STACK_GROW_SIZE)
      requiredSize = STACK_GROW_SIZE;
    else requiredSize += STACK_GROW_SIZE;

    if(stackSize + requiredSize < stackLimit) {
        Int spIdx = sp-dataStack, fpIdx = fp-dataStack, ptrIdx = -1;
        if(ptr) {
            for(Int i = 0; i < stackSize; i++) {
                if(ptr == &dataStack[i].object)
                {
                    ptrIdx = i;
                    break;
                }
            }
        }

        dataStack = (StackElement *) __realloc(dataStack, sizeof(StackElement) * (stackSize + requiredSize),
                                        sizeof(StackElement) * stackSize);

        StackElement *stackItem = dataStack+stackSize;
        for(Int i = stackSize; i < stackSize+requiredSize; i++) {
            stackItem->object.object = NULL;
            stackItem->var=0;
            stackItem++;
        }

        if(ptrIdx >= 0) ptr = &dataStack[ptrIdx].object;
        sp = dataStack+spIdx;
        fp = dataStack+fpIdx;
        stackSize += requiredSize;
        gc.addMemory(sizeof(StackElement) * requiredSize);
    }
    else if(stackSize != stackLimit) {
        Int spIdx = sp-dataStack, fpIdx = fp-dataStack, ptrIdx = -1;
        if(ptr) {
            for(Int i = 0; i < stackSize; i++) {
                if(ptr == &dataStack[i].object)
                {
                    ptrIdx = i;
                    break;
                }
            }
        }

        dataStack = (StackElement *) __realloc(dataStack, sizeof(StackElement) * (stackLimit), sizeof(StackElement) * stackSize);
        gc.addMemory(sizeof(Frame) * (stackLimit - stackSize));

        StackElement *stackItem = dataStack+stackSize;
        for(Int i = stackSize; i < stackLimit; i++) {
            stackItem->object.object = NULL;
            stackItem->var=0;
            stackItem++;
        }

        if(ptrIdx >= 0) ptr = &dataStack[ptrIdx].object;
        sp = dataStack+spIdx;
        fp = dataStack+fpIdx;
        stackSize = stackLimit;
    }

}

