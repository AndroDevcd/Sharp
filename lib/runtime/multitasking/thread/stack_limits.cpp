//
// Created by bknun on 9/16/2022.
//
#include "stack_limits.h"
#include "../thread/thread_controller.h"
#include "../thread/sharp_thread.h"
#include "../../memory/memory_helpers.h"
#include "../../virtual_machine.h"

size_t threadStackSize = STACK_SIZE;
size_t virtualStackSize = INTERNAL_STACK_SIZE;

bool valid_stack_size(uInt stackSize) {
    return stackSize >= STACK_MIN;
}

bool valid_internal_stack_size(uInt stackSize) {
    return stackSize >= INTERNAL_STACK_MIN;
}

void initialize_main_thread_stack(std::list<string>& appArgs) {
    sharp_thread *main = get_main_thread();
    const short MIN_ARGS = 5;
    Int size = MIN_ARGS+appArgs.size();
    object *argsObject = &(++main->scht->task->sp)->object;
    Int iter=0;

    stringstream ss;
    ss << vm.manifest.target;
    string str = ss.str();

    new_object_array(argsObject, size);

    new_string_array(argsObject->o->node + iter++, vm.manifest.application.c_str());
    new_string_array(argsObject->o->node + iter++, vm.manifest.version.c_str());
    new_string_array(argsObject->o->node + iter++, str.c_str());

#ifdef WIN32_
    str = ("win");
#endif
#ifdef POSIX_
    str = ("posix");
#endif
    new_string_array(argsObject->o->node + iter++, str.c_str());
    new_class(argsObject->o->node + iter++, vm.thread_class);
    sharp_object *mainThread = argsObject->o->node[iter-1].o;
    main->currentThread.o = mainThread;
    inc_ref(mainThread)

    assign_number_field(
       resolve_field("native_handle", mainThread),
       0
    );

    auto nameField = resolve_field("name", mainThread);
    new_class(
       nameField,
       vm.string_class
    );

    str = ("main");
    new_string_array(
       resolve_field("data", nameField),
       str.c_str()
    );

    assign_number_field(
            resolve_field("started", mainThread),
            true
    );

    assign_number_field(
            resolve_field("stack_size", mainThread),
            threadStackSize
    );

    /*
     * Assign program args to be passed to main
     */
    for(auto str : appArgs) {
        new_string_array(argsObject->o->node + iter++, str.c_str());
    }

    appArgs.clear();
}
