//
// Created by BNunnally on 9/5/2020.
//
#include "fiber.h"
#include "../../memory/garbage_collector.h"
#include "task_controller.h"
#include "../../memory/sharp_object.h"
#include "../thread/sharp_thread.h"
#include "../../memory/memory_helpers.h"
#include "../thread/stack_limits.h"
#include "../../virtual_machine.h"
#include "../../telemetry/telemetry.h"

void fiber::free() {
    release_all_mutexes(this);
    if(held != NULL) {
        std::free(held); held = NULL;
    }
    heldCount = 0;
    heldCapacity = 0;

    if(stack != NULL) {
        release_bytes(sizeof(stack_item) * stackSize);
        std::free(this->stack); stack = NULL;
    }

    TC(if(slotTags != NULL) { std::free(slotTags); slotTags = NULL; })

    if(registers != NULL) {
        release_bytes(sizeof(vm_cell) * REGISTER_SIZE);
        std::free(registers); registers = NULL;
    }

    if(frames != NULL) {
        release_bytes(sizeof(frame) * frameSize);
        std::free(frames); frames = NULL;
    }

    copy_object(&fiberObject, (sharp_object*)nullptr);
    copy_object(&exceptionObject, (sharp_object*)nullptr);
    release_bytes(sizeof(fiber));
    fp = NULL;
    sp = NULL;
    name.clear();
    id = -1;
}

void fiber::growFrame() {
    TELEMETRY_COUNT(tm_frame_grows)
    guard_mutex(mut)

    // The list of call frames DOUBLES (at least FRAME_GROW_SIZE more), like the
    // stack in growStack below and for the same reason: with a ceiling of a
    // million frames, growing by a fixed 150 would copy the list thousands of
    // times on the way up. Doubling copies it about a dozen times.
    //      C++ comparison: how std::vector grows on push_back.
    uInt more = frameSize > FRAME_GROW_SIZE ? frameSize : FRAME_GROW_SIZE;

    if(frameSize + more < frameLimit) {
        frames = realloc_mem<frame>(
                frames, sizeof(frame) * (frameSize + more),
                sizeof(frame) * frameSize
        );

        frameSize += more;
    }
    else if(frameSize != frameLimit) {
        frames = realloc_mem<frame>(
                frames, sizeof(frame) * (frameLimit),
                sizeof(frame) * frameSize
        );

        frameSize = frameLimit;
    }
    
}

void init_struct(fiber *fib) {
    fib = new (fib)fiber();
    fib->main = nullptr;
    fib->pc = 0;
    fib->id = -1;
    fib->rom = 0;
    fib->state = FIB_CREATED;
    fib->exitVal = 0;
    fib->delayTime = -1;
    fib->wakeable = true;
    fib->finished = false;
    fib->attachedThread = NULL;
    fib->boundThread = NULL;
    fib->exceptionObject.o = NULL;
    fib->fiberObject.o = NULL;
    fib->stack = NULL;
    fib->registers = NULL;
    fib->frames = NULL;
    fib->f_lock = NULL;
    fib->calls = 0;
    fib->callFramePtr = -1;
    fib->rebuildDepth = 0;
    fib->rebuilding = false;
    fib->held = NULL;
    fib->heldCount = 0;
    fib->heldCapacity = 0;
    fib->current = NULL;
    fib->ptr = NULL;
    fib->ptrOwner = NULL;
    fib->readyNext = NULL;
    fib->queued = false;
    fib->lockWaiting = false;
    fib->timerArmed = -1;
    fib->readyNext = NULL;
    fib->queued = false;
    fib->lockWaiting = false;
    fib->timerArmed = -1;
    fib->readyNext = NULL;
    fib->queued = false;
    fib->lockWaiting = false;
    fib->timerArmed = -1;
    fib->readyNext = NULL;
    fib->queued = false;
    fib->lockWaiting = false;
    fib->timerArmed = -1;
    fib->readyNext = NULL;
    fib->queued = false;
    fib->lockWaiting = false;
    fib->timerArmed = -1;
    fib->stackLimit = virtualStackSize;
    fib->registers = malloc_mem<vm_cell>(REGISTER_SIZE * sizeof(vm_cell));
    new (&fib->mut) std::recursive_mutex();

    if(virtualStackSize < INITIAL_STACK_SIZE) {
        fib->stackSize = virtualStackSize;
    }
    else if((vm.mf.threadLocals + 50) < INITIAL_STACK_SIZE) {
        fib->stackSize = INITIAL_STACK_SIZE;
    }
    else {
        fib->stackSize = vm.mf.threadLocals + INITIAL_STACK_SIZE;
    }

    fib->stack = malloc_struct<stack_item>(sizeof(stack_item), fib->stackSize);
    fib->growAt = fib->stack + fib->stackSize - 1;
    TC(fib->slotTags = (unsigned char*)calloc(fib->stackSize, 1); memset(fib->regTags, 0, sizeof(fib->regTags));)
    if(virtualStackSize - vm.mf.threadLocals < INITIAL_FRAME_SIZE) {
        fib->frameSize = virtualStackSize - vm.mf.threadLocals;
    }
    else {
        fib->frameSize = INITIAL_FRAME_SIZE;
    }

    fib->frames = malloc_mem<frame>(fib->frameSize * sizeof(frame));
    fib->frameLimit = virtualStackSize - vm.mf.threadLocals;
    fib->fp = &fib->stack[vm.mf.threadLocals];
    fib->sp = (&fib->stack[vm.mf.threadLocals]) - 1;
}

void fiber::growStack(Int requiredSize) {
    TELEMETRY_COUNT(tm_stack_grows)
    guard_mutex(mut)
    TC(uInt tagsBefore = stackSize;)
    if(requiredSize < STACK_GROW_SIZE)
      requiredSize = STACK_GROW_SIZE;
    else requiredSize += STACK_GROW_SIZE;

    // The stack DOUBLES: it grows by at least as many slots as it has. With
    // the ceiling at a million slots (INTERNAL_STACK_SIZE), growing 512 slots
    // at a time would copy the whole stack two thousand times on the way up -
    // each copy longer than the last. Doubling gets from 200 slots to the
    // ceiling in thirteen copies, and a fiber that never recurses deeply still
    // never grows at all.
    //      C++ comparison: how std::vector grows on push_back; Go grows a
    //      goroutine's stack the same way.
    if(requiredSize < stackSize)
      requiredSize = stackSize;

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
                stack, sizeof(stack_item) * (stackSize + requiredSize),
                sizeof(stack_item) * stackSize
        );

        stack_item *stackItem = stack+stackSize;
        for(Int i = stackSize; i < stackSize+requiredSize; i++) {
            stackItem->obj.o = NULL;
            stackItem->var.i=0;
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
                stack, sizeof(stack_item) * (stackLimit),
                sizeof(stack_item) * stackSize
        );

        stack_item *stackItem = stack+stackSize;
        for(Int i = stackSize; i < stackLimit; i++) {
            stackItem->obj.o = NULL;
            stackItem->var.i=0;
            stackItem++;
        }

        if(ptrIdx >= 0) ptr = &stack[ptrIdx].obj;
        sp = stack+spIdx;
        fp = stack+fpIdx;
        stackSize = stackLimit;
    }

    growAt = stack + stackSize - 1;     // the stack may have moved and is longer

    TC(if(stackSize != tagsBefore) {
        slotTags = (unsigned char*)realloc(slotTags, stackSize);
        memset(slotTags + tagsBefore, 0, stackSize - tagsBefore);
    })
}

