//
// Created by bknun on 6/11/2022.
//
#include "sharp_label.h"
#include "../context/context.h"
#include "../dependency/dependancy.h"
#include "../operation/operation.h"
#include "../../taskdelegator/task_delegator.h"
#include "sharp_function.h"

sharp_label* create_label(
        string name,
        context *context,
        Ast *createLocation,
        operation_schema *scheme) {
    if(scheme && context->functionCxt) {
        sharp_label *label;
        if((label = resolve_label(name, context)) == NULL) {
            Int id = create_allocate_label_operation(scheme);
            label = new sharp_label(name, id, impl_location(currThread->currTask->file, createLocation));
            context->labels.add(label);
            context->functionCxt->labels.add(label);
            return label;
        } else {
            if(currThread->currTask->file->errors->createNewError(
                    PREVIOUSLY_DEFINED, createLocation, "label `" + name +
                                                        "` is already defined"))
                print_impl_location(label->name, "label", label->location);
            return label;
        }
    }

    return NULL;
}