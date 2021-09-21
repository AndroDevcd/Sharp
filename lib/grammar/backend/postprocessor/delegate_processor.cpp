//
// Created by BNunnally on 9/19/2021.
//

#include "delegate_processor.h"
#include "../types/sharp_class.h"
#include "function_processor.h"
#include "../../taskdelegator/task_delegator.h"

void process_delegates(sharp_class *with_class) {
    List<sharp_function*> functions, requiredFunctions;

    if(with_class->baseClass != NULL
        || !with_class->interfaces.empty()) {
        locate_functions_with_type(
                with_class, normal_function,
                false, functions);

        if(with_class->baseClass != NULL) {
            locate_functions_with_type(
                    with_class->baseClass, delegate_function,
                    false, requiredFunctions);
        }

        for(Int i = 0; i < with_class->interfaces.size(); i++) {
            locate_functions_with_type(
                    with_class->interfaces.get(i), delegate_function,
                    true, requiredFunctions);
        }

        for(Int i = 0; i < functions.size(); i++) {
            process_function_return_type(functions.get(i));
        }

        for(Int i = 0; i < requiredFunctions.size(); i++) {
            process_function_return_type(requiredFunctions.get(i));
        }

        bool foundFunc;
        for(Int i = 0; i < requiredFunctions.size(); i++) {
            sharp_function *delegate = requiredFunctions.get(i);
            foundFunc = false;

            for(Int j = 0; j < functions.size(); j++) {
                sharp_function *fun = functions.get(i);

                if(is_explicit_type_match(sharp_type(delegate), sharp_type(fun))
                    && is_explicit_type_match(delegate->returnType, fun->returnType)) {
                    foundFunc = true;
                    break;
                }
            }

            if(!foundFunc) {
                stringstream err;
                err << "delegate function `" << function_to_str(delegate) << "` must be defined in class 1"
                    << with_class->fullName << "1:";
                currThread->currTask->file->errors->createNewError(GENERIC, with_class->ast, err.str());
                print_impl_location(delegate->name, "delegate", delegate->implLocation);
            }
        }

        functions.free();
        requiredFunctions.free();
    }
}