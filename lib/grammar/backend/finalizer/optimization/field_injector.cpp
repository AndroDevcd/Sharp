//
// Created by bknun on 8/24/2022.
//

#include "field_injector.h"
#include "../../types/types.h"
#include "dependency_runner.h"
#include "optimizer.h"
#include "../generation/generator.h"


void inject_field_initialization(sharp_class *with_class, Ast *ast, sharp_field *field) {
    if(field->fieldType == normal_field && field->scheme) {
        sharp_function *function;
        List<sharp_field*> params;
        sharp_type void_type(type_nil);

        if(check_flag(field->flags, flag_static)) {
            function = resolve_function(
                    static_init_name(with_class->name), with_class,
                    params, initializer_function, exclude_all,
                    ast, false, false
            );

            if(function == NULL) {
                generation_error("static init function for class: " + with_class->fullName + " was not found");
            }
        } else {

            function = resolve_function(
                    instance_init_name(with_class->name), with_class,
                    params,initializer_function, exclude_all,
                    ast, false, false
            );

            if(function == NULL) {
                generation_error("instance init function for class: " + with_class->fullName + " was not found");
            }
        }

        mark(function);
        function->scheme->steps.add(new operation_step(operation_step_scheme, field->scheme));
    } else if(field->scheme) {
        sharp_function *function;
        List<sharp_field*> params;
        sharp_type void_type(type_nil);
        string platform_kernel = "platform.kernel";
        sharp_class *platformClass = resolve_class(get_module(platform_kernel), "platform", false, false);

        function = resolve_function(
                tls_init_function_name, platformClass,
                params,normal_function, exclude_all,
                ast, false, false
        );

        if(function->scheme == NULL) {
            function->scheme = new operation_schema(scheme_master);
        }

        mark(function);
        function->scheme->steps.add(new operation_step(operation_step_scheme, field->scheme));
    }
}

void injectRelevantClassFields(sharp_class *with_class) {
    if(!with_class->injected) {
        with_class->injected = true;

        for (Int i = 0; i < with_class->children.size(); i++) {
            injectRelevantClassFields(with_class->children.get(i));
        }

        for (Int i = 0; i < with_class->fields.size(); i++) {
            sharp_field *field = with_class->fields.get(i);

            if (field->used) {
                inject_field_initialization(with_class, field->ast, field);
            }
        }
    }
}

void injectAllRelevantFields() {
    for(Int i = 0; i < compressedCompilationFiles.size(); i++) {
        sharp_file *file = compressedCompilationFiles.get(i);

        for(Int j = 0; j < file->classes.size(); j++) {
            injectRelevantClassFields(file->classes.get(j));
        }
    }
}