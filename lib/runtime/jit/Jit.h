//
// Created by braxtonn on 8/23/2019.
//

#ifndef SHARP_JIT_H
#define SHARP_JIT_H

#include "x64Assembler.h"
#include "../../../stdimports.h"
#include <mutex>
#ifdef BUILD_JIT

#define jit_error_compile  1             // error compiling the source
#define jit_error_mem      304           // not enough memory
#define jit_error_max_attm 305           // maximum compile attempts reached
#define jit_error_unsupp   306           // unsupported OS or processor
#define jit_error_size     307           // the function trying to be compiled is too small
#define jit_error_ok       0             // the result you want

class Jit {
public:
    Jit()
    :
        x64Asm(NULL),
        error(jit_error_ok)
    {
        messageQueue.init();

        if (ASMJIT_ARCH_BITS == 64) {
            // currently only running on windows
            if(OS_id==win) {
                x64Asm = new x64Assembler();
            } else
                error = jit_error_unsupp;
        }
        else {
            error = jit_error_unsupp;
        }
    }

    static Jit *self;
    static void startup();
    static void shutdown();
    static void tlsSetup();
    static
#ifdef WIN32_
    DWORD WINAPI
#endif
#ifdef POSIX_
    void*
#endif
    threadStart(void *);
    void run();

    /**
     * Messages are sendable by other threads to command the Jit compiler
     * to attempt to compile a certain function deemed to be called frequently.
     * The calling thread should do its due diligence to check if the function in question
     * qualifies to be compiled to not over saturate the message queue with the same function
     * @param message
     */
    static void sendMessage(Method* func);

    _BaseAssembler* getAssembler();
private:
    Thread *tSelf;
    recursive_mutex mutex;
    _List<int64_t> messageQueue;
    x64Assembler* x64Asm;
    int error;
};
#endif

#endif //SHARP_JIT_H
