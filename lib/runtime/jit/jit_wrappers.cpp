//
// JIT Wrapper Functions Implementation
// Static C-style wrapper functions for calling C++ instance methods from JIT code
//

#include "jit_wrappers.h"
#include "../multitasking/fiber/fiber.h"

// Fiber/Task wrapper functions

void jit_growStack(fiber* task, Int requiredSize) {
    task->growStack(requiredSize);
}