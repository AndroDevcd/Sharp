//
// Created by bknun on 9/16/2022.
//

#include "vm_initializer.h"
#include "exe/exe_processor.h"
#include "multitasking/thread/stack_limits.h"
#include "virtual_machine.h"
#include "multitasking/thread/thread_controller.h"
#include "reflect/reflect_helpers.h"
#include "multitasking/scheduler/idle_scheduler.h"
#include "multitasking/thread/sharp_thread.h"
#include "../util/time.h"
#include "main.h"
#include "jit/jit_compiler.h"
#include "memory/tracing_collector.h"
#include "memory/function_value.h"
#include "console_helper.h"
#include "../debugger/hooks.h"        // what the debugger build (sdb) adds; empty in the plain VM

int initialize_virtual_machine()
{
    console_output_start();     // before anything is printed (console_helper.h)
    KeyPair<int, string> result;
    if((result = process_exe()).key != 0) {
        if(!result.value.empty())
            fprintf(stderr, "%s\n", result.value.c_str());
        return result.key;
    }

    if((virtualStackSize - vm.mf.threadLocals) <= 1)
        return 2;


    create_main_thread();
    gc_startup();
    idle_handler_startup();
    init_jit_compiler();

    /**
     * Resolve Frequently Used classes
     */
    vm.throwable = locate_class("std#throwable");
    vm.runtime_except = locate_class("std#runtime_exception");
    vm.stack_overflow_except = locate_class("std#stack_overflow_exception");
    vm.thread_stack_except = locate_class("std#thread_stack_exception");
    vm.bounds_except = locate_class("std#out_of_bounds_exception");
    vm.nullptr_except = locate_class("std#nullptr_exception");
    vm.class_cast_except = locate_class("std#class_cast_exception");
    vm.out_of_memory_except = locate_class("std#out_of_memory_exception");
    vm.invalid_operation_except = locate_class("std#invalid_operation_exception");
    vm.unsatisfied_link_except = locate_class("std#unsatisfied_link_error");
    vm.ill_state_except = locate_class("std#illegal_state_exception");
    vm.incompatible_class_except = locate_class("std#incompatible_class_exception");
    vm.object_import_error = locate_class("std#object_import_error");
    vm.string_class = locate_class("std#string");
    vm.int_class = locate_class("std#int");
    vm.char_class = locate_class("std#char");
    vm.short_class = locate_class("std#short");
    vm.byte_class = locate_class("std#byte");
    vm.uint_class = locate_class("std#uint");
    vm.uchar_class = locate_class("std#uchar");
    vm.ushort_class = locate_class("std#ushort");
    vm.long_class = locate_class("std#long");
    vm.ulong_class = locate_class("std#ulong");
    vm.double_class = locate_class("std#double");      // (reflection hands a decimal field out in one)
    vm.stack_sate = locate_class("platform.kernel#stack_state");
    vm.thread_class = locate_class("std.io#thread");
    vm.exception_class = locate_class("std#exception");
    vm.error_class = locate_class("std#error");
    vm.char_array_class = locate_class("std#char_array");
    vm.fiber_class = locate_class("std.io.fiber#fiber");
    cout.precision(16);

    copy_object(&vm.memoryExcept, create_object(vm.out_of_memory_except));

    string outOfMemoryMsg = "out of memory";
    copy_object(resolve_field("message",  vm.memoryExcept.o), create_object(outOfMemoryMsg.size(), type_int8));
    assign_string_field(resolve_field("message",  vm.memoryExcept.o)->o, outOfMemoryMsg);

    /**
     * Initialize all classes to be used for static access
     */
    setup_function_values();    // the slots of platform.kernel#function_value
    build_number_slot_maps();   // which slots of each class hold numbers: needed before the first object exists
    for(unsigned long i = 0; i < vm.mf.classes; i++) {
        copy_object(vm.staticHeap + i, create_static_object(vm.classes + i));
        tgc_make_permanent(vm.staticHeap[i].o);
    }

    return 0;
}

recursive_mutex shutdownMut;
void shutdown() {
    // (SDB_BEFORE_PROGRAM_END: under the debugger the end of the program waits while the program is stopped)
    SDB_BEFORE_PROGRAM_END(thread_self);
    bool destroySystem = false;
    {
        guard_mutex(shutdownMut) // prevent multiple threads from shutting down the vm

        if (vm.state == VM_RUNNING) {
            vm.state = VM_SHUTTING_DOWN;
            destroySystem = true;
        }
    }

    if(destroySystem) {
        destroy();
        vm.state = VM_TERMINATED;
    }
    console_output_flush();     // what the program printed is out before the process ends
}

void destroy() {
    if(vm.state == VM_CREATED)
        return;

    if(thread_self != NULL) {
        vm.exitVal = thread_self->task->exitVal;
    } else
        vm.exitVal = 1;

    if(thread_self) shutdown_thread(thread_self);
    kill_all_threads();
    shutdown_jit_compiler();
}
