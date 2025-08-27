//
// JIT Wrapper Functions
// Static C-style wrapper functions for calling C++ instance methods from JIT code
//

#ifndef SHARP_JIT_WRAPPERS_H
#define SHARP_JIT_WRAPPERS_H

#include "../../../stdimports.h"

// Forward declarations
class fiber;
class sharp_class;
struct sharp_thread;

// Fiber/Task wrapper functions
void jit_growStack(fiber* task, Int requiredSize);

// Exception wrapper functions
void jit_throwException(sharp_class* exceptionClass, const char* message);
bool jit_catchException();
void jit_enableExceptionFlag(sharp_thread* thread, bool enable);

// Thread control wrapper functions
void jit_suspendSelf();

#endif //SHARP_JIT_WRAPPERS_H