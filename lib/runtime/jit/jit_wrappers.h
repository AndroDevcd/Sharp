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
void jit_gcSafepoint();

// Object creation and manipulation wrapper functions
void* jit_createObject(sharp_class* sc);
void jit_copyObject1(object* dest, sharp_object* src);  // copy_object(object *to, sharp_object *from)
void jit_copyObject2(object* dest, object* src);       // copy_object(object *to, object *from)
int jit_getObjectType(sharp_object* obj);               // Get obj->type (handles bit-field access)

// Function call wrapper functions
void jit_prepareMethod(int address);

// Wrappers for opcodes whose interpreter code is easier to share than to re-emit.
// Like every wrapper they never let a C++ exception escape: a vm_exception is
// turned into the thread's exception flag, which the compiled code checks.
void jit_prepareMethodDynamic(int inRegister);            // CALLD: range check + prepare_method((Int)regs[inRegister])
void jit_invokeDelegate(int address, int argSize, bool staticCall); // INVOKE_DELEGATE
bool jit_execInterrupt(int interrupt);                    // _INT: exec_interrupt, returns true if the vm terminated
void jit_checkLen(int inRegister);                        // CHECKLEN slow path: throws the bounds exception
bool jit_lockObject(sharp_object* o);                     // LOCK: true = could not lock, context switch requested
void jit_unlockObject(sharp_object* o);                   // ULOCK

// One wrapper per opcode that is rare or inherently slow (allocation, casts, io,
// integer remainder...). Each runs the interpreter's code for that opcode. The
// int parameters are the decoded instruction operands (register numbers,
// addresses), not values.
void jit_newArray(int inRegister, int ntype);             // NEWARRAY
void jit_newObjArray(int inRegister);                     // NEWOBJARRAY
void jit_newClassArray(int inRegister, int classAddress); // NEWCLASSARRAY
void jit_newString(int address);                          // NEWSTRING
void jit_cast(int classAddress);                          // CAST
void jit_varCast(int varType);                            // VARCAST
void jit_is(int outRegister, int type);                   // IS
void jit_itest(int outRegister);                          // ITEST
void jit_swap();                                          // SWAP
void jit_put(int inRegister);                             // PUT
void jit_putc(int inRegister);                            // PUTC
void jit_get(int outRegister);                            // GET
void jit_sleep(int inRegister);                           // SLEEP
void jit_exp(int outRegister, int leftRegister, int rightRegister); // EXP
void jit_movu64(int outRegister, int registerToCast);     // MOVU64
void jit_and(int leftRegister, int rightRegister);        // AND
void jit_mod(int outRegister, int leftRegister, int rightRegister); // MOD
void jit_imod(int outRegister, int value);                // IMOD
void jit_modl(int inRegister, int relFrameAddress);       // MODL
void jit_imodl(int value, int relFrameAddress);           // IMODL

// Debug wrapper functions
void jit_instructionStart(opcode_instr opcode, int pc);

#endif //SHARP_JIT_WRAPPERS_H