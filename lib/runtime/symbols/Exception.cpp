//
// Created by BraxtonN on 2/12/2018.
//

#include "Exception.h"
#include "ClassObject.h"
#include "../Thread.h"
#include "../register.h"
#include "../memory/GarbageCollector.h"
#include "../VirtualMachine.h"

void Throwable::drop() {
    this->handlingClass = NULL;
    this->message.free();
    stackTrace.free();
}

Exception::Exception(const char *msg, bool native)
        :
        runtime_error(msg)
{
    throwable.init(vm.RuntimeExcept, msg, native);
    pushException();
}

Exception::Exception(const std::string &__arg, bool native)
        :
        throwable(vm.RuntimeExcept, __arg, native),
        runtime_error(__arg)
{
    pushException();
}

extern void printRegs();
Exception::Exception(ClassObject* throwable, const std::string &__arg, bool native)
        :
        throwable(throwable, __arg, native),
        runtime_error(__arg)
{
    pushException();
}

Exception::~Exception()
{
    throwable.drop();
}

void Exception::pushException() {
    Thread *thread = thread_self;
    if(thread != NULL && throwable.native) {

        if(throwable.handlingClass == vm.OutOfMemoryExcept) {
            /*
             * If there is no memory we exit
             */
            GUARD(thread->mutex);
            thread->state = THREAD_KILLED;
            thread->exceptionObject = vm.outOfMemoryExcept;

            native_string str;
            vm.fillStackTrace(str);
            Object *stackTrace = vm.resolveField("stack_trace", vm.outOfMemoryExcept.object);
            if(stackTrace) {
                *stackTrace = gc.newObjectUnsafe(str.len);

                if(stackTrace->object) {
                    for (Int i = 0; i < str.len; i++) {
                        stackTrace->object->HEAD[i] = str.chars[i];
                    }
                }
            }
            gc.printClassRefStatus();
            sendSignal(thread->signal, tsig_kill, 1);
            return;
        }

        thread->exceptionObject
                = gc.newObject(throwable.handlingClass);

        gc.createStringArray(
                vm.resolveField("message", thread->exceptionObject.object),
                   throwable.message);
        vm.getFrameInfo(vm.resolveField("frame_info", thread->exceptionObject.object));
    }
}

Exception::Exception(Throwable &throwable)
        :
        runtime_error("")
{
    this->throwable.init();
    this->throwable = throwable;
}