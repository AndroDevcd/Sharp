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


                    for (int i = 0; i < size; i++) {
                        if (procs[i] == nullptr) return MISSING_API_PROC;
                    }

                    return 0;
                } else return INVALID_API_VERS;
            } else return INVALID_PROC_SIZE;
        }
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