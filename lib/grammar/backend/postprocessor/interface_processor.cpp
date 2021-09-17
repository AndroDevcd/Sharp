//
// Created by BNunnally on 9/16/2021.
//

#include "interface_processor.h"
#include "../types/sharp_type.h"
#include "../dependency/dependancy.h"
#include "../../taskdelegator/task_delegator.h"
#include "../types/sharp_class.h"

void process_interfaces(sharp_class *with_class, Ast *ast) {
    if(ast->hasSubAst(ast_reference_pointer_list)) {
        Ast *refPtrList = ast->getSubAst(ast_reference_pointer_list);
        for(Int i = 0; i < refPtrList->getSubAstCount(); i++) {
            Ast *refPtr = refPtrList->getSubAst(i);

            sharp_type _interface(resolve(refPtr));

            if(_interface.type == type_class) {
                if(_interface._class->type != class_interface) {
                    stringstream err;
                    err << "class `" + _interface._class->name + "` is not an interface";
                    currThread->currTask->file->errors->createNewError(
                            GENERIC, ast, err.str());
                } else if(!with_class->interfaces.addif(_interface._class)) {
                    stringstream err;
                    err << "duplicate interface '" << _interface._class->name << "'";
                    currThread->currTask->file->errors->createNewError(
                            GENERIC, ast, err.str());
                }
            } else {
                stringstream err;
                err << " interface class must be of type class but was found to be of type: " << type_to_str(_interface);
                currThread->currTask->file->errors->createNewError(
                        GENERIC, ast, err.str());
            }
        }
    }
}