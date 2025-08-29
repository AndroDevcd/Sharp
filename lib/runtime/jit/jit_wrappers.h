//
// JIT Wrapper Functions
// Static C-style wrapper functions for calling C++ instance methods from JIT code
//

#ifndef SHARP_JIT_WRAPPERS_H
#define SHARP_JIT_WRAPPERS_H

#include "../../../stdimports.h"
#include "../multitasking/thread/sharp_thread.h"
#include "../../core/opcode/opcode.h"

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

// Object creation and manipulation wrapper functions
void* jit_createObject(sharp_class* sc);
void jit_copyObject1(object* dest, sharp_object* src);  // copy_object(object *to, sharp_object *from)
void jit_copyObject2(object* dest, object* src);       // copy_object(object *to, object *from)
int jit_getObjectType(sharp_object* obj);               // Get obj->type (handles bit-field access)

// Function call wrapper functions
void jit_prepareMethod(int address);

// Debug wrapper functions
void jit_instructionStart(opcode_instr opcode, int pc);

#endif //SHARP_JIT_WRAPPERS_H