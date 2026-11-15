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
    ss << vm.mf.target;
    string str = ss.str();

    copy_object(argsObject, create_object(size));

    copy_object(argsObject->o->node + iter, create_object(vm.mf.application.size(), type_int8));
    assign_string_field((argsObject->o->node + iter++)->o, vm.mf.application);
    copy_object(argsObject->o->node + iter, create_object(vm.mf.version.size(), type_int8));
    assign_string_field((argsObject->o->node + iter++)->o, vm.mf.version);
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

    set_number_field(mainThread, "native_handle", 0);

    auto nameField = resolve_field("name", mainThread);
    copy_object(nameField, create_object(vm.string_class));

    str = ("main");
    auto data = resolve_field("data", nameField->o);
    copy_object(data, create_object(str.size(), type_int8));
    assign_string_field(data->o, str);


    set_number_field(mainThread, "started", true);

    set_number_field(mainThread, "stack_size", threadStackSize);

    /*
     * Assign program args to be passed to main
     */
    for(auto arg : appArgs) {
        /*
         * An empty argument (`sharp prog a "" b`): create_object() makes no
         * array of size 0 - it gives null - and assign_string_field() then
         * threw "string field is null!" before main ran; the process died
         * with SIGSEGV. Null is no answer either: platform.srt_init casts each
         * item `as _int8[]`, which raises on null. So the item is an array of
         * length 0 (create_empty_array), and main sees the string "".
         *
         * C++ comparison: like `std::string("")` - a valid, empty value, not an error.
         */
        if(arg.empty()) {
            copy_object(argsObject->o->node + iter++, create_empty_array(type_int8));
            continue;
        }
        copy_object(argsObject->o->node + iter, create_object(arg.size(), type_int8));
        assign_string_field((argsObject->o->node + iter++)->o, arg);
    }

    appArgs.clear();
}
