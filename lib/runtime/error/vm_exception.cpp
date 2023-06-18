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
#include "../../core/opcode/opcode_macros.h"


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

void fill_stack_trace(string &output) {
    sharp_thread *thread = thread_self;
    if(thread->task->frames)
    {
        uInt functions = 0;
        fiber *task = thread->task;
        frame *frames = task->frames;

        if(task->callFramePtr + 1 < EXCEPTION_PRINT_MAX)
        {
            for(Int i = 0; i < task->callFramePtr; i++) {
                if(functions++ >= EXCEPTION_PRINT_MAX) break;
                output += get_info(&vm.methods[frames[i].returnAddress], frames[i].pc-vm.methods[frames[i].returnAddress].bytecode) + "\n";
            }
        } else {
            for(Int i = (task->callFramePtr + 1) - EXCEPTION_PRINT_MAX; i < task->callFramePtr + 1; i++) {
                if(functions++ >= EXCEPTION_PRINT_MAX) break;
                output += get_info(&vm.methods[frames[i].returnAddress], frames[i].pc-vm.methods[frames[i].returnAddress].bytecode) + "\n";
            }
        }

        output += get_info(&vm.methods[task->current->address], current_pc) + "\n";
    }
}

void fill_stack_trace_from_frame_info() {
    auto task = thread_self->task;
    auto stackState = pop_stack_object.o;
    auto methodsField = resolve_field("methods", stackState);
    auto pcField = resolve_field("pc", stackState);

    if(methodsField->o && pcField->o) {
        std::list<KeyPair<Int, Int>> frameInfo;
        for (Int i = 0; i < methodsField->o->size; i++) {
            frameInfo.push_back(KeyPair<Int, Int>(methodsField->o->HEAD[i], pcField->o->HEAD[i]));
        }

        string str;
        fill_stack_trace(str, frameInfo);
        auto stacktrace = &push_stack_object;

        copy_object(stacktrace, create_object(vm.string_class));
        auto dataField = resolve_field("data", stacktrace->o);
        auto strObject = create_object(str.size(), type_int8);

        copy_object(dataField, strObject);

        if (str.size() > 0)
            assign_string_field(strObject, str);
    } else {
        copy_object(&push_stack_object, (sharp_object*) nullptr);
    }
}

void fill_stack_trace(string &output, std::list<KeyPair<Int, Int>> frameInfo) {
    sharp_thread *thread = thread_self;
    fiber *task = thread->task;
    frame *frames = task->frames;

    for(KeyPair<Int, Int> &info : frameInfo) {
        output += get_info(&vm.methods[info.key], info.value) + "\n";
    }
}

void vm_exception::push_exception() {
    sharp_thread *thread = thread_self;
    if(thread != NULL && !err.internal) {

        if(err.handlingClass == vm.out_of_memory_except) {

            print_memory_diagnostics();
            if(thread->task != NULL && thread->task != NULL) {
                /*
                 * If there is no memory we exit
                 */
                guard_mutex(thread->mut);
                copy_object(&thread->task->exceptionObject, &vm.memoryExcept);

                string str;
                fill_stack_trace(str);
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
        object *info = resolve_field("frame_info", thread->task->exceptionObject.o);
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

recursive_mutex exceptionMut;
void print_thrown_exception() {
    guard_mutex(exceptionMut)
    auto thread = thread_self;
    fiber *task = thread_self->task;

    if(hasSignal(thread->signal, tsig_except) && thread->task) {
        auto frameInfo = resolve_field("frame_info", task->exceptionObject.o);
        auto message = resolve_field("message", task->exceptionObject.o);
        auto stackTrace = resolve_field("stack_trace", task->exceptionObject.o);
        sharp_class *exceptionClass = NULL;
        if(task->exceptionObject.o != NULL)
            exceptionClass = &vm.classes[CLASS(task->exceptionObject.o->info)];


        cout << "Unhandled exception on thread " << thread->name << " (most recent call last):\n";
        if(stackTrace != NULL && stackTrace->o != NULL){
            auto data = resolve_field("data", stackTrace->o);

            if(data != NULL) {
                cout << read_string_value(data->o);
            }
        } else if(frameInfo && frameInfo->o) {
            if(((task->sp - task->stack) + 1) >= task->stackLimit) {
                task->sp = task->stack - 1;
            }

            if(exceptionClass != NULL && exceptionClass->guid != vm.out_of_memory_except->guid) {
                copy_object(&(++task->sp)->obj, frameInfo);
                fill_stack_trace_from_frame_info();

                auto exceptionString = pop_stack_object.o;
                auto data = resolve_field("data", exceptionString);
                if(data) cout << read_string_value(data->o);
            }
        }

        sendSignal(thread->signal, tsig_except, 0);
        cout << endl << (exceptionClass != NULL ? exceptionClass->name : "") << " ("
             << (message != NULL ? read_string_value(message->o) : "") << ")\n";
    }
}
