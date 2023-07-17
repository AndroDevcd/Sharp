//
// Created by bknun on 7/14/2023.
//

#ifndef SHARP_SNB_API_H
#define SHARP_SNB_API_H

#include <cstdint>
#include <iostream>
#include <cstdlib>
#include <sstream>
#include <stdexcept>
#include <string>
#include <cstdio>
#include <cstring>
#include <list>

#define SNB_API_VERS 0x001
#define LIB_PROCS_SIZE 27

// data types
#define TYPE_INT8 (0)
#define TYPE_INT16 (1)
#define TYPE_INT32 (2)
#define TYPE_INT64 (3)
#define TYPE_UINT8 (4)
#define TYPE_UINT16 (5)
#define TYPE_UINT32 (6)
#define TYPE_UINT64 (7)
#define TYPE_FUNCTION_PTR (8)
#define TYPE_VAR (9)
#define TYPE_OBJECT (10)
#define TYPE_CLASS (11)

// errors
#define INVALID_NUMBER -0xC0000005
#define INVALID_PROC_SIZE 1
#define INVALID_API_VERS 2
#define MISSING_API_PROC 3

// Define EXPORTED for any platform
#if defined _WIN32 || defined __CYGWIN__
#define EXPORTED __attribute__ ((dllexport))
#define NOT_EXPORTED
#elif defined(__GNUC__)
//  GCC
    #define EXPORTED __attribute__((visibility("default")))
    #define NOT_EXPORTED
#else
    //  do nothing and hope for the best?
    #define EXPORTED
    #define NOT_EXPORTED
    #pragma warning Unknown dynamic link import/export semantics.
#endif

#define CONCAT2(a, b) a ## b

// global scope
#define scope_1(module) \
    namespace CONCAT2(module, _global) {

// class scope
#define scope_2(module, name) \
    namespace CONCAT2(module, _ ## name) {

#define import_1(module) \
    using namespace CONCAT2(module, _global)

#define import_2(module, klass) \
    using namespace CONCAT2(module, _ ## klass)

#define import_3(a, b, c) static_assert(false, "Too many arguments provided, try import(std) or import(std, string) or import(std, someClass_subClass)");
#define import_4(a, b, c, d) static_assert(false, "Too many arguments provided, try import(std) or import(std, string) or import(std, someClass_subClass)");
#define import_5(a, b, c, d, e) static_assert(false, "Too many arguments provided, try import(std) or import(std, string) or import(std, someClass_subClass)");

#define GET_MACRO(_1,_2, _3, _4, _5, NAME,...) NAME
#define import(...) GET_MACRO(__VA_ARGS__, import_5, import_4, import_3, import_2, import_1)(__VA_ARGS__)

#define scope_3(a, b, c) static_assert(false, "Too many arguments provided, try scope_begin(std) or scope_begin(std, string) or scope_begin(std, someClass_subClass)");
#define scope_4(a, b, c, d) static_assert(false, "Too many arguments provided, try scope_begin(std) or scope_begin(std, string) or scope_begin(std, someClass_subClass)");
#define scope_5(a, b, c, d, e) static_assert(false, "Too many arguments provided, try scope_begin(std) or scope_begin(std, string) or scope_begin(std, someClass_subClass)");

#define scope_begin(...) GET_MACRO(__VA_ARGS__, scope_5, scope_4, scope_3, scope_2, scope_1)(__VA_ARGS__)

#define scope_end() }

// standardized exported bridge functions
#ifdef __cplusplus
extern "C" {
#endif

EXPORTED uint32_t snb_link_proc(const char*);
EXPORTED void snb_main(long);
EXPORTED int snb_initialize(void* lib_procs[], int, int);

#ifdef __cplusplus
}
#endif

// struct object {} equivalent
typedef void* SharpObject;

//internal system calls
typedef void (*_ref_increment)(SharpObject obj);
typedef void (*_ref_decrement)(SharpObject obj);
typedef double* (*_get_local_number_at)(int32_t address);
typedef SharpObject (*_get_local_object_at)(int32_t address);
typedef SharpObject (*_get_field)(SharpObject obj, const char* name);
typedef long double* (*_get_raw_number_data)(SharpObject obj);
typedef double (*_pop_number)();
typedef SharpObject (*_pop_object)();
typedef uint32_t (*_get_size)(SharpObject obj);
typedef void (*_assign_object)(SharpObject dest, SharpObject src);
typedef SharpObject (*_get_static_class)(const char *name);
typedef void (*_new_array)(int32_t size, int8_t type);
typedef void (*_new_class)(const char *name);
typedef void (*_new_object_array)(int32_t size);
typedef void (*_new_class_array)(const char *name, int32_t size);
typedef SharpObject (*_get_object_at)(SharpObject obj, int32_t index);
typedef void (*_push_number)(double value);
typedef void (*_push_object)(SharpObject value);
typedef void (*_call)(int32_t address);
typedef bool (*_is_exception_thrown)();
typedef const char* (*_get_class_name)(SharpObject obj);
typedef void (*_prepare_exception)(SharpObject exception);
typedef void (*_clear_exception)();
typedef SharpObject (*_get_exception_object)();
typedef int32_t (*_allocate_stack_space)(int32_t size);
typedef int32_t (*_allocate_local_variable)() ;
typedef void (*_dup_stack)();

namespace snb_api {
    namespace internal {
        extern _ref_increment ref_increment;
        extern _ref_decrement ref_decrement;
        extern _get_local_number_at get_local_number_at;
        extern _get_local_object_at get_local_object_at;
        extern _get_field get_field;
        extern _get_raw_number_data get_raw_number_data;
        extern _pop_number pop_number;                          // throws exception
        extern _pop_object pop_object;                          // throws exception
        extern _get_size get_size;
        extern _assign_object assign_object;
        extern _get_static_class get_static_class;
        extern _new_array new_array;                             // throws exception
        extern _new_class new_class;                             // throws exception
        extern _new_object_array new_object_array;               // throws exception
        extern _new_class_array new_class_array;                 // throws exception
        extern _get_object_at get_object_at;                     // throws exception
        extern _push_number push_number;                         // throws exception
        extern _push_object push_object;                         // throws exception
        extern _push_object push_object;                         // throws exception
        extern _call call;                                       // throws exception
        extern _is_exception_thrown is_exception_thrown;
        extern _get_class_name get_class_name;                   // throws exception
        extern _prepare_exception prepare_exception;
        extern _clear_exception clear_exception;
        extern _get_exception_object get_exception_object;
        extern _allocate_stack_space allocate_stack_space;        // throws exception
        extern _allocate_local_variable allocate_local_variable;  // throws exception
        extern _dup_stack dup_stack;                              // throws exception
    }

    class VMException : public std::runtime_error {
    public:
        VMException(SharpObject handle, const char *msg)
                :
                exceptionClass(handle),
                std::runtime_error(msg)
        {
        }

        SharpObject exceptionClass;
    };

    struct LocalVariable {
        LocalVariable():
                obj(nullptr),
                num(nullptr),
                address(0)
        {
        }

        LocalVariable(
                int32_t address
        ):
                obj(internal::get_local_object_at(address)),
                num(internal::get_local_number_at(address)),
                address(address)
        {}

        LocalVariable(
                const LocalVariable &local
        ):
                obj(local.obj),
                num(local.num),
                address(local.address)
        {}

        ~LocalVariable()
        {}

        SharpObject obj;
        double* num;
        int32_t address;

        void refresh() {
            obj = internal::get_local_object_at(address);
            num = internal::get_local_number_at(address);
        }
    };

    struct Locals {
        std::list<LocalVariable> locals;

        void refreshAll() {
            for(auto &local : locals) {
                local.refresh();
            }
        }
    };

    void throwException(SharpObject exceptionClass) {
        throw VMException(exceptionClass, "");
    }

    void check_for_err() {
        if(internal::is_exception_thrown()) {
            throwException(nullptr);
        }
    }

    LocalVariable create_local_variable() {
        using namespace internal;
        auto addr = allocate_local_variable(); // create space for the var
        if(addr == INVALID_NUMBER) return { };
        return {addr};
    }

    SharpObject create_string_class(
            const char* message,
            void (*constructor)(SharpObject instance, SharpObject message)
    ) {
        using namespace internal;

        auto klass = create_local_variable();
        auto arry = create_local_variable();
        new_class("std#string");
        assign_object(klass.obj, pop_object());

        new_array(strlen(message), TYPE_INT8);
        assign_object(arry.obj, pop_object());
        auto str = get_raw_number_data(arry.obj);
        for(int32_t i = 0; i < strlen(message); i++) {
            str[i] = message[i];
        }

        constructor(klass.obj, arry.obj);
        pop_object(); // pop arry local field
        return pop_object(); // return "klass" or created string class
    }
}

#endif //SHARP_SNB_API_H
