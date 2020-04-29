//
// Created by BraxtonN on 2/12/2018.
//

#include "Exception.h"
#include "../Environment.h"
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
    if(thread_self != NULL && throwable.native) {
        if(throwable.handlingClass == vm.OutOfMemoryExcept) {
            /*
             * If there is no memory we exit
             */
            std::lock_guard<std::mutex> gd(thread_self->mutex);
            thread_self->state = THREAD_KILLED;
            sendSignal(thread_self->signal, tsig_kill, 1);
            return;
        }

        thread_self->exceptionObject
                = GarbageCollector::self->newObject(throwable.handlingClass);

        GarbageCollector::self->createStringArray(
                vm.resolveField("message", &thread_self->exceptionObject),
                   thread_self->throwable.message);
    }
}

Exception::Exception(Throwable &throwable)
        :
        runtime_error("")
{
    this->throwable.init();
    this->throwable = throwable;
}