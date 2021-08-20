//
// Created by bnunnally on 8/18/21.
//

#include "thread_exception_controller.h"
#include "../VirtualMachine.h"

void print_exception(Thread *thread) {
    GUARD(thread->mutex)
    fiber *this_fiber = thread->this_fiber;

    if(hasSignal(thread->signal, tsig_except) && thread->this_fiber) {
        Object* frameInfo = vm.resolveField("frame_info", this_fiber->exceptionObject.object);
        Object* message = vm.resolveField("message", this_fiber->exceptionObject.object);
        Object* stackTrace = vm.resolveField("stack_trace", this_fiber->exceptionObject.object);
        ClassObject *exceptionClass = NULL;
        if(this_fiber->exceptionObject.object != NULL)
            exceptionClass = &vm.classes[CLASS(this_fiber->exceptionObject.object->info)];


        cout << "Unhandled exception on thread " << thread->name.str() << " (most recent call last):\n";
        if(stackTrace != NULL && stackTrace->object != NULL){
            Object* data = vm.resolveField("data", stackTrace->object);

            if(data != NULL) {
                cout << vm.stringValue(data->object);
            }
        } else if(frameInfo && frameInfo->object) {
            if(((this_fiber->sp - this_fiber->dataStack) + 1) >= this_fiber->stackLimit) {
                this_fiber->sp = this_fiber->dataStack - 1;
            }

            if(exceptionClass != NULL && exceptionClass->guid != vm.OutOfMemoryExcept->guid) {
                (++this_fiber->sp)->object = frameInfo;
                vm.getStackTrace();
                Object* data = vm.resolveField("data", this_fiber->sp->object.object);

                if(data != NULL) {
                    cout << vm.stringValue(data->object);
                }
            }
        }

        sendSignal(thread->signal, tsig_except, 0);
        cout << endl << (exceptionClass != NULL ? exceptionClass->name : "") << " ("
             << (message != NULL ? vm.stringValue(message->object) : "") << ")\n";
    }
}

