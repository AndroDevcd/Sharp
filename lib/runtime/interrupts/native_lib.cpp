//
// System interrupts for native libraries (SNI): load a .so/.dll, free it, and
// link one of its C++ functions to a Sharp `native` declaration.
//
// Moved verbatim from the old switch in virtual_machine.cpp (OP_LOAD_LIBRARY,
// OP_FREE_LIBRARY, OP_LINK_FUNC). See interrupt_internal.h for what a handler is.
//
#include "interrupt_internal.h"

//
// load_library(path): load a native library beside the program (see sni/
// sni_runtime.cpp). The path is popped off the stack.
//
void op_load_library(Int interrupt) {
    sharp_object *libNameObj = (thread_self->task->sp--)->obj.o;
    if (libNameObj == NULL || libNameObj->type > type_var)
        throw vm_exception(vm.nullptr_except, "");
    string name;
    populate_string(name, libNameObj);
    sni_load_library(name);
}

//
// free_library(path): unload a native library that was loaded earlier. Result
// (did it unload) in EBX.
//
void op_free_library(Int interrupt) {
    sharp_object *libNameObj = (thread_self->task->sp--)->obj.o;

    if (libNameObj != NULL && libNameObj->type <= type_var) {
        string name;
        populate_string(name, libNameObj);
        registers[EBX].i = sni_free_library(name);
    }
}

//
// link_function(library, function): bind a native function in a loaded library
// to the Sharp `native` declaration named `function`. Both names are popped;
// the result (did it bind) goes in EBX.
//
void op_link_func(Int interrupt) {
    sharp_object *libNameObj = (thread_self->task->sp--)->obj.o;
    sharp_object *funcNameObj = (thread_self->task->sp--)->obj.o;

    if (libNameObj != NULL && libNameObj->type <= type_var
        && funcNameObj != NULL && funcNameObj->type <= type_var) {
        string libname, funcname;

        populate_string(libname, libNameObj);
        populate_string(funcname, funcNameObj);
        registers[EBX].i = sni_link_from(libname, locate_function(funcname.c_str()));
    }
}
