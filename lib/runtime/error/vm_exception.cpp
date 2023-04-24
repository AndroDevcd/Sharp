//
// Created by bknun on 9/18/2022.
//

#include "vm_exception.h"
#include "../virtual_machine.h"
#include "../multitasking/thread/sharp_thread.h"
#include "../memory/garbage_collector.h"
#include "../multitasking/fiber/fiber.h"
#include "../reflect/reflect_helpers.h"
#include "../../core/thread_state.h"


vm_exception::vm_exception(const char *msg, bool native)
        :
        runtime_error(msg)
{
    err.init(vm.runtime_except, msg, native);
    push_exception();
}

vm_exception::vm_exception(const std::string &__arg, bool native)
        :
        err(vm.runtime_except, __arg, native),
        runtime_error(__arg)
{
    push_exception();
}

vm_exception::vm_exception(sharp_class* throwable, const std::string &__arg, bool native)
        :
        err(throwable, __arg, native),
        runtime_error(__arg)
{
    push_exception();
}

vm_exception::vm_exception(vm_err &err)
        :
        runtime_error("")
{
    this->err.init();
    this->err = err;
}

void fill_stacktrace(string &output) {
    sharp_thread *thread = thread_self;
    if(thread->scht->task->frames)
    {
        uInt functions = 0;
        fiber *task = thread->scht->task;
        frame *frames = task->frames;

        if(task->calls + 1 < EXCEPTION_PRINT_MAX)
        {
            for(Int i = 0; i < task->calls; i++) {
                if(functions++ >= EXCEPTION_PRINT_MAX) break;
                output += get_info(&vm.methods[frames[i].returnAddress], frames[i].pc-task->rom) + "\n";
            }
        } else {
            for(Int i = (task->calls + 1) - EXCEPTION_PRINT_MAX; i < task->calls + 1; i++) {
                if(functions++ >= EXCEPTION_PRINT_MAX) break;
                output += get_info(&vm.methods[frames[i].returnAddress], frames[i].pc-task->rom) + "\n";
            }
        }
    }
}

void vm_exception::push_exception() {
    sharp_thread *thread = thread_self;
    if(thread != NULL && !err.internal) {

        if(err.handlingClass == vm.out_of_memory_except) {

            if(thread->task != NULL && thread->task != NULL) {
                /*
                 * If there is no memory we exit
                 */
                guard_mutex(thread->mut);
                copy_object(&thread->task->exceptionObject, &vm.memoryExcept);

                string str;
                fill_stacktrace(str);
                object *field = resolve_field("stack_trace", thread->task->exceptionObject.o);

                if (field) {
                    copy_object(field, create_object(vm.string_class, true));
                    object *dataField = resolve_field("data", field->o);

                    if (dataField != NULL) {
                        copy_object(dataField, create_object(str.size(), type_int8, true));

                        for (Int i = 0; i < str.size(); i++) {
                            dataField->o->HEAD[i] = str[i];
                        }
                    }
                }
            }

            sendSignal(thread->signal, tsig_kill, 1);
            return;
        }

        copy_object(&thread->task->exceptionObject, create_object(err.handlingClass));
        object *msg = resolve_field("message", thread->task->exceptionObject.o);
        object *info = resolve_field("frameInfo", thread->task->exceptionObject.o);
        copy_object(msg, create_object(err.message.size(), type_int8));

        for (Int i = 0; i < err.message.size(); i++) {
            msg->o->HEAD[i] = err.message[i];
        }

        auto frameInfo = get_frame_info(thread->task);
        copy_object(info, create_object(vm.stack_sate));

        object *pc = resolve_field("pc", info->o);
        object *methods = resolve_field("methods", info->o);
        copy_object(pc, create_object(frameInfo.size(), type_int64));
        copy_object(methods, create_object(frameInfo.size(), type_int64));

        Int i = 0; // here we assign frame info to the fields
        for(auto frame : frameInfo) {
            methods->o->HEAD[i] = frame.key;
            pc->o->HEAD[i] = frame.value;

            i++;
        }
    }
}

void print_thrown_exception() {
    // todo: impl
}
