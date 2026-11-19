//
// System interrupts for native libraries (SNI): load a .so/.dll, free it, and
// link one of its C++ functions to a Sharp `native` declaration.
//
// Moved verbatim from the old switch in virtual_machine.cpp (OP_LOAD_LIBRARY,
// OP_FREE_LIBRARY, OP_LINK_FUNC). See interrupt_internal.h for what a handler is.
//
#include "../cffi/c_bind.h"
#include "../cffi/c_callback.h"
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

//
// c_library(name, place): where the C library the program calls `name` is - a path or a file's
// name (cffi/c_bind.h). The place is popped, then the name.
//
void op_c_library(Int interrupt) {
    sharp_object *placeObj = (thread_self->task->sp--)->obj.o;
    sharp_object *nameObj = (thread_self->task->sp--)->obj.o;
    if (placeObj == NULL || placeObj->type > type_var || nameObj == NULL || nameObj->type > type_var)
        throw vm_exception(vm.nullptr_except, "");
    string name, place;
    populate_string(name, nameObj);
    populate_string(place, placeObj);
    c_set_library(name, place);
}

//
// c_link_all(): open every C library the program's extern functions name and find every one of
// those functions now. Pushes "" when all are there, otherwise what is missing.
//
void op_c_link_all(Int interrupt) {
    auto task = thread_self->task;
    string missing = c_link_all();
    auto strObject = create_object(missing.size(), type_int8);
    copy_object(&push_stack_object, strObject);
    if(strObject && missing.size() > 0) assign_string_field(strObject, missing);
}

//
// c_keep(function) / c_drop(function): a function C calls after the call it was handed to has
// returned keeps its C address until it is dropped (cffi/c_callback.h). The function is popped.
//
void op_c_keep(Int interrupt) {
    c_keep((thread_self->task->sp--)->obj.o);
}

void op_c_drop(Int interrupt) {
    c_drop((thread_self->task->sp--)->obj.o);
}

//
// c_serve_one(): the thread that serves the calls of C's own threads waits for one and runs it
// (cffi/c_callback.h). Pushes an array of one item to go on, of none when the program is ending.
//
void op_c_serve_one(Int interrupt) {
    bool more = c_serve_one();
    auto task = thread_self->task;
    copy_object(&push_stack_object, more ? create_object(1, type_int8) : create_empty_array(type_int8));
}

void op_c_serve_start(Int interrupt) {
    c_serve_start();
}
