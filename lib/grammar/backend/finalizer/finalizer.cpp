//
// Created by bknun on 7/4/2022.
//

#include "finalizer.h"
#include "../../taskdelegator/task_delegator.h"
#include "../types/types.h"
#include "../compiler/compiler.h"
#include "optimization/optimizer.h"
#include "generation/generator.h"

void setup_core_functions() {
    string starterClass = "platform";
    string starterMethod = "srt_init";
    string platform_kernel = "platform.kernel";

    sharp_class *StarterClass = resolve_class(get_module(platform_kernel), starterClass, false, false);
    if(StarterClass != NULL && user_main_method != NULL) {
        List<sharp_field*> params;
        string name = "args";
        sharp_type type(type_object, false, true);
        params.add(new sharp_field(name, StarterClass, StarterClass->implLocation, type, flag_public, normal_field, StarterClass->ast));

        sharp_function *init_function =
                resolve_function(starterMethod, StarterClass, params, normal_function, exclude_all, StarterClass->ast, false, false);

        if(init_function == NULL) {
            sharpFiles.get(0)->errors->createNewError(GENERIC, 0, 0, "could not locate main method '" + starterMethod + "(object[])' in starter class");
        } else {
           sharp_function *StaticInit, *TlsSetup;
            if(!check_flag(init_function->flags, flag_static)) {
                init_function->implLocation.file->errors->createNewError(GENERIC, init_function->ast, "main method '" + starterMethod + "(object[])' must be static");
            }

            if(!check_flag(init_function->flags, flag_private)) {
                init_function->implLocation.file->errors->createNewError(GENERIC, init_function->ast, "main method '" + starterMethod + "(object[])' must be private");
            }

            if(init_function->returnType != type_var) {
                init_function->implLocation.file->errors->createNewError(GENERIC, init_function->ast, "main method '" + starterMethod + "(object[])' must  return a var");
            }

            deleteList(params);

            StaticInit = resolve_function(static_init_function_name, StarterClass, params, normal_function, exclude_all, StarterClass->ast, false, false);
            if(StaticInit == NULL) {
                init_function->implLocation.file->errors->createNewError(GENERIC, init_function->ast, "could not locate runtime environment setup method '" static_init_function_name "()' in starter class");
                return;
            }

            TlsSetup = resolve_function(tls_init_function_name, StarterClass, params, normal_function, exclude_all, StarterClass->ast, false, false);
            if(TlsSetup == NULL) {
                init_function->implLocation.file->errors->createNewError(GENERIC, init_function->ast, "could not locate thread local storage init method '" + starterMethod + "()' in starter class");
                return;
            }

            create_line_record_operation(StaticInit->scheme, StaticInit->ast->line);
            create_line_record_operation(TlsSetup->scheme, TlsSetup->ast->line);
            if(!check_flag(StaticInit->flags, flag_static)) {
                init_function->implLocation.file->errors->createNewError(GENERIC, init_function->ast, "runtime environment setup method '" static_init_function_name "()' must be static");
            }
            if(!check_flag(TlsSetup->flags, flag_static)) {
                init_function->implLocation.file->errors->createNewError(GENERIC, init_function->ast, "thread local storage init method '" tls_init_function_name "()' must be static");
            }

            if(StaticInit->returnType != type_nil) {
                init_function->implLocation.file->errors->createNewError(GENERIC, init_function->ast, "runtime environment setup method '" static_init_function_name "()' must return `nil`");
            }
            if(TlsSetup->returnType != type_nil) {
                init_function->implLocation.file->errors->createNewError(GENERIC, init_function->ast, "thread local storage init method '" tls_init_function_name "()' must return `nil`");
            }

            if(!check_flag(StaticInit->flags, flag_private)) {
                init_function->implLocation.file->errors->createNewError(GENERIC, init_function->ast, "runtime environment setup method '" static_init_function_name "()' must be private");
            }

            if(!check_flag(StaticInit->flags, flag_private)) {
                init_function->implLocation.file->errors->createNewError(GENERIC, init_function->ast, "thread local storage init method '"  tls_init_function_name  "()' must be private");
            }

            sharp_field *userMain;
            switch(main_method_sig) {
                case 0: { // fn main(string[]) : var;
                    userMain = resolve_field("main", StarterClass, false);
                    break;
                }
                case 1: { // fn main2(string[]);
                    userMain = resolve_field("main2", StarterClass, false);
                    break;
                }
                case 2: { // fn main3();
                    userMain = resolve_field("main3", StarterClass, false);
                    break;
                }
                case 3: { // fn main4() var;
                    userMain = resolve_field("main4", StarterClass, false);
                    break;
                }
            }

            if(userMain != NULL) {
                if(!check_flag(userMain->flags, flag_static)) {
                    init_function->implLocation.file->errors->createNewError(GENERIC, userMain->ast, "main method function pointer field '" +
                            userMain->name + ": " + type_to_str(userMain->type) + "' must be static");
                }

                if(!check_flag(userMain->flags, flag_private)) {
                    init_function->implLocation.file->errors->createNewError(GENERIC, userMain->ast, "main method function pointer field '" + userMain->name + ": " + type_to_str(userMain->type) + "' must be ptivate");
                }

                if(userMain->type != type_function_ptr) {
                    init_function->implLocation.file->errors->createNewError(GENERIC, userMain->ast, "main method function pointer field '" + userMain->name + ": " + type_to_str(userMain->type) + "' must be a function pointer");
                }



                APPLY_TEMP_SCHEME_WITH_TYPE(0, scheme_assign_value, (*StaticInit->scheme),
                    operation_schema *resultVariableScheme = new operation_schema();
                    operation_schema *functionAddressScheme = new operation_schema();

                    ALLOCATE_REGISTER_1X(0, functionAddressScheme,
                        create_get_static_function_address_operation(functionAddressScheme, user_main_method, false);
                    )
                    create_get_value_operation(&scheme_0, functionAddressScheme, false, false);
                    create_push_to_stack_operation(&scheme_0);

                    create_static_field_access_operation(resultVariableScheme, userMain);
                    create_get_value_operation(&scheme_0, resultVariableScheme, false, false);
                    create_pop_value_from_stack_operation(&scheme_0);
                    create_unused_data_operation(&scheme_0);

                    delete resultVariableScheme;
                    delete functionAddressScheme;
                )
            } else
                init_function->implLocation.file->errors->createNewError(INTERNAL_ERROR,  init_function->ast, "user main method function pointer was not found");

            create_return_operation(StaticInit->scheme);
            create_return_operation(TlsSetup->scheme);

            genesis_method = init_function; // reset main method to __srt_init()
        }
    } else if(StarterClass == NULL) {
        sharpFiles.get(0)->errors->createNewError(GENERIC, 0, 0, "Could not find starter class '" + starterClass + "' for application entry point.");
    } else if(user_main_method == NULL) {
        sharpFiles.get(0)->errors->createNewError(GENERIC, 0, 0, "could not locate main method 'main(string[])'");
    }
}

void finalize_compilation() {
    if(all_files_compiled_successfully())
    {
        optimize();
        generate();
    }
}