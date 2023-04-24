//
// Created by bknun on 9/16/2022.
//
#include "stack_limits.h"
#include "../thread/thread_controller.h"
#include "../thread/sharp_thread.h"
#include "../../memory/memory_helpers.h"
#include "../../virtual_machine.h"
#include "../../multitasking/fiber/fiber.h"
#include "../../reflect/reflect_helpers.h"

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
    object *argsObject = &(++main->task->sp)->obj;
    Int iter=0;

    stringstream ss;
    ss << vm.manifest.target;
    string str = ss.str();

    copy_object(argsObject, create_object(size));

    copy_object(argsObject->o->node + iter, create_object(vm.manifest.application.size(), type_int8));
    assign_string_field((argsObject->o->node + iter++)->o, vm.manifest.application);
    copy_object(argsObject->o->node + iter, create_object(vm.manifest.version.size(), type_int8));
    assign_string_field((argsObject->o->node + iter++)->o, vm.manifest.version);
    copy_object(argsObject->o->node + iter, create_object(str.size(), type_int8));
    assign_string_field((argsObject->o->node + iter++)->o, str);


#ifdef WIN32_
    str = ("win");
#endif
#ifdef POSIX_
    str = ("posix");
#endif
    copy_object(argsObject->o->node + iter, create_object(str.size(), type_int8));
    assign_string_field((argsObject->o->node + iter++)->o, str);

    copy_object(argsObject->o->node + iter++, create_object(vm.thread_class));
    sharp_object *mainThread = argsObject->o->node[iter-1].o;
    main->currentThread.o = mainThread;
    inc_ref(mainThread)

    assign_numeric_field(
       resolve_field("native_handle", mainThread)->o,
       0,
       0
    );

    auto nameField = resolve_field("name", mainThread);
    copy_object(nameField, create_object(vm.string_class));

    str = ("main");
    auto data = resolve_field("data", nameField->o);
    copy_object(data, create_object(str.size(), type_int8));
    assign_string_field(data->o, str);


    assign_numeric_field(
            resolve_field("started", mainThread)->o,
            0,
            true
    );

    assign_numeric_field(
            resolve_field("stack_size", mainThread)->o,
            0,
            threadStackSize
    );

    /*
     * Assign program args to be passed to main
     */
    for(auto arg : appArgs) {
        copy_object(argsObject->o->node + iter, create_object(arg.size(), type_int8));
        assign_string_field((argsObject->o->node + iter++)->o, arg);
    }

    appArgs.clear();
}
