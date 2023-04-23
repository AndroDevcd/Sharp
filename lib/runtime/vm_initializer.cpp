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

int initialize_virtual_machine()
{
    KeyPair<int, string> result;
    if((result = process_exe()).key != 0) {
        if(!result.value.empty())
            fprintf(stderr, "%s\n", result.value.c_str());
        return result.key;
    }

    if((internalStackSize - vm.manifest.threadLocals) <= 1)
        return 2;


    create_main_thread();
    gc_startup();
    idle_handler_startup();

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
    vm.stack_sate = locate_class("platform.kernel#stack_state");
    vm.thread_class = locate_class("std.io#thread");
    vm.exception_class = locate_class("std#exception");
    vm.error_class = locate_class("std#error");
    vm.fiber_class = locate_class("std.io.fiber#fiber"); // todo: check if any of these classes are null
    cout.precision(16);

    new_class(&vm.memoryExcept, vm.out_of_memory_except);
    new_string_array(resolve_field("message",  vm.memoryExcept.o), "out of memory");

    /**
     * Initialize all classes to be used for static access
     */
    for(unsigned long i = 0; i < vm.manifest.classes; i++) {
        new_class(vm.staticHeap + i, vm.classes + i, true);
        SET_GENERATION(vm.staticHeap[i].object->info, gc_perm);
    }

    return 0;
}

void shutdown() {

}
