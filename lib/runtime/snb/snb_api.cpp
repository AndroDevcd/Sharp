//
// Created by bknun on 7/14/2023.
//

#include "snb_api.h"

namespace snb_api {
    namespace internal {
        _ref_increment ref_increment;
        _ref_decrement ref_decrement;
        _get_local_number_at get_local_number_at;
        _get_local_object_at get_local_object_at;
        _get_field get_field;
        _get_raw_number_data get_raw_number_data;
        _pop_number pop_number;
        _pop_object pop_object;
        _get_size get_size;
        _assign_object assign_object;
        _get_static_class get_static_class;
        _new_array new_array;
        _new_class new_class;
        _new_object_array new_object_array;
        _new_class_array new_class_array;
        _get_object_at get_object_at;
        _push_number push_number;
        _push_object push_object;
        _call call;
        _is_exception_thrown is_exception_thrown;
        _get_class_name get_class_name;
        _prepare_exception prepare_exception;
        _clear_exception clear_exception;
        _get_exception_object get_exception_object;
        _allocate_stack_space allocate_stack_space;
        _allocate_local_variable allocate_local_variable;
        _dup_stack dup_stack;
        _return_object return_object;
        _return_number return_number;
        _return_call return_call;
        _validate_app_id validate_app_id;

        int lib_init(
                void *procs[],
                int size,
                int vers
        ) {
            if(size == LIB_PROCS_SIZE) {
                if(vers == SNB_API_VERS) {
                    int proc_id = 0;
                    ref_increment = (_ref_increment) procs[proc_id++];
                    ref_decrement = (_ref_decrement) procs[proc_id++];
                    get_local_number_at = (_get_local_number_at) procs[proc_id++];
                    get_local_object_at = (_get_local_object_at) procs[proc_id++];
                    get_field = (_get_field) procs[proc_id++];
                    get_raw_number_data = (_get_raw_number_data) procs[proc_id++];
                    pop_number = (_pop_number) procs[proc_id++];
                    pop_object = (_pop_object) procs[proc_id++];
                    get_size = (_get_size) procs[proc_id++];
                    assign_object = (_assign_object) procs[proc_id++];
                    get_static_class = (_get_static_class) procs[proc_id++];
                    new_array = (_new_array) procs[proc_id++];
                    new_class = (_new_class) procs[proc_id++];
                    new_object_array = (_new_object_array) procs[proc_id++];
                    new_class_array = (_new_class_array) procs[proc_id++];
                    get_object_at = (_get_object_at) procs[proc_id++];
                    push_number = (_push_number) procs[proc_id++];
                    push_object = (_push_object) procs[proc_id++];
                    call = (_call) procs[proc_id++];
                    is_exception_thrown = (_is_exception_thrown) procs[proc_id++];
                    get_class_name = (_get_class_name) procs[proc_id++];
                    prepare_exception = (_prepare_exception) procs[proc_id++];
                    clear_exception = (_clear_exception) procs[proc_id++];
                    get_exception_object = (_get_exception_object) procs[proc_id++];
                    allocate_stack_space = (_allocate_stack_space) procs[proc_id++];
                    allocate_local_variable = (_allocate_local_variable) procs[proc_id++];
                    dup_stack = (_dup_stack) procs[proc_id++];
                    return_object = (_return_object) procs[proc_id++];
                    return_number = (_return_number) procs[proc_id++];
                    return_call = (_return_call) procs[proc_id++];
                    validate_app_id = (_validate_app_id) procs[proc_id++];


                    for (int i = 0; i < size; i++) {
                        if (procs[i] == nullptr) return MISSING_API_PROC;
                    }

                    return 0;
                } else return INVALID_API_VERS;
            } else return INVALID_PROC_SIZE;
        }
    }


    void throwException(SharpObject exceptionClass) {
        throw VMException(exceptionClass, "");
    }

    void check_for_err() {
        if(internal::is_exception_thrown()) {
            throwException(nullptr);
        }
    }

    SharpObject create_new_primitive_wrapper(
            const char *classname,
            var value,
            void (*constructor)(SharpObject $instance, var value)
    ) {
        using namespace internal;
        auto exceptionVar = create_local_variable(); // create space for new exception
        if(exceptionVar.obj == nullptr) return nullptr;

        new_class(classname);
        use_var(exceptionVar,
            assign_object(exceptionVar.obj, pop_object());
        )

        use_var(exceptionVar,
           constructor(exceptionVar.obj, value);
        )

        return pop_object();
    }

    SharpObject create_new_exception(
            const char *classname,
            const char *errorMsg,
            SharpObject (*get_stack_trace)(SharpObject $instance),
            void (*constructor)(SharpObject $instance)
    ) {
        using namespace internal;
        auto exceptionVar = create_local_variable(); // create space for new exception
        if(exceptionVar.obj == nullptr) return nullptr;

        new_class(classname);
        assign_object(exceptionVar.obj, pop_object());

        use_var(exceptionVar,
                constructor(exceptionVar.obj);
        )

        use_var(exceptionVar,
                auto message = get_field(exceptionVar.obj, "message");
                        new_array(strlen(errorMsg), TYPE_INT8);
                        assign_object(message, pop_object());
                        auto str = get_raw_number_data(message);
                        for(int32_t i = 0; i < strlen(errorMsg); i++) {
                            str[i] = errorMsg[i];
                        }
        )

        use_var(exceptionVar,
                get_stack_trace(exceptionVar.obj);
        )
        return pop_object();
    }

    void string_from(std::string &str, SharpObject strObj) {
        auto raw = internal::get_raw_number_data(strObj);
        auto size = internal::get_size(strObj);

        for(long i = 0; i < size; i++) {
            str += (char)raw[i];
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
        use_var(klass,
                assign_object(klass.obj, pop_object());
        )

        new_array(strlen(message), TYPE_INT8);
        use_var(arry,
                assign_object(arry.obj, pop_object());
                        auto str = get_raw_number_data(arry.obj);
                        for(int32_t i = 0; i < strlen(message); i++) {
                            str[i] = message[i];
                        }
        )

        use_vars(klass, arry,
                 constructor(klass.obj, arry.obj);
        )
        pop_object(); // pop arry local field
        return pop_object(); // return "klass" or created string class
    }
}

#ifdef __cplusplus
extern "C" {
#endif

EXPORTED int snb_initialize(void* lib_procs[], int size, int vers) {
    return snb_api::internal::lib_init(lib_procs, size, vers);
}

#ifdef __cplusplus
}
#endif