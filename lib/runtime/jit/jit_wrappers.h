//
// JIT Wrapper Functions
// Static C-style wrapper functions for calling C++ instance methods from JIT code
//

#ifndef SHARP_JIT_WRAPPERS_H
#define SHARP_JIT_WRAPPERS_H

#include "../../stdimports.h"

// Forward declarations
class fiber;

// Fiber/Task wrapper functions
void jit_growStack(fiber* task, Int requiredSize);

#endif //SHARP_JIT_WRAPPERS_H