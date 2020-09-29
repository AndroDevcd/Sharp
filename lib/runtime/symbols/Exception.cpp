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

}

void Exception::pushException() {
    Thread *thread = thread_self;
    if(thread != NULL && throwable.native) {

        if(throwable.handlingClass == vm.OutOfMemoryExcept) {

            if(thread->this_fiber) {
                /*
                 * If there is no memory we exit
                 */
                GUARD(thread->mutex);
                thread->this_fiber->exceptionObject = vm.outOfMemoryExcept;

                native_string str;
                vm.fillStackTrace(str);
                Object *stackTrace = vm.resolveField("stack_trace",
                                                     vm.outOfMemoryExcept.object); // seg faile error please fig stack_trace is now a string not a int8[]
                if (stackTrace) {
                    *stackTrace = gc.newObjectUnsafe(vm.StringClass);
                    Object *data = vm.resolveField("data", stackTrace->object);
                    if (data != NULL) {
                        *data = gc.newObjectUnsafe(str.len, _INT8);

                        if (data->object != NULL) {
                            for (Int i = 0; i < str.len; i++) {
                                data->object->HEAD[i] = str.chars[i];
                            }
                        }
                    }
                }
            }

            //gc.printClassRefStatus();
            sendSignal(thread->signal, tsig_kill, 1);
            return;
        }

        thread->this_fiber->exceptionObject
                = gc.newObject(throwable.handlingClass);

        gc.createStringArray(
                vm.resolveField("message", thread->this_fiber->exceptionObject.object),
                   throwable.message);
        vm.getFrameInfo(vm.resolveField("frame_info", thread->this_fiber->exceptionObject.object));
    }
}

Exception::Exception(Throwable &throwable)
        :
        runtime_error("")
{
    this->throwable.init();
    this->throwable = throwable;
}