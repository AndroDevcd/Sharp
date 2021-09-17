//
// Created by BNunnally on 9/9/2021.
//

#include "base_class_processor.h"
#include "../../sharp_file.h"
#include "../../taskdelegator/task_delegator.h"
#include "../astparser/ast_parser.h"
#include "../types/sharp_module.h"
#include "../types/sharp_class.h"
#include "../../../util/File.h"

void process_base_class(
        sharp_class *with_class,
        Ast *ast) {
    Ast* block = ast->getSubAst(ast_block), *trunk;

    create_context(with_class->owner, true);
    sharp_class *base = resolve_base_class(with_class, ast->getSubAst(ast_base_class));

    if(base != NULL) {
        if(base->type == class_interface
            && with_class->type != class_interface) {
            stringstream err;
            err << "class '" << with_class->fullName << "' can only be inherited by another interface";
            currThread->currTask->file->errors->createNewError(GENERIC, ast->line, ast->col, err.str());
            err.str("");
            goto end;
        } else if(base->type == class_enum
            && with_class->type != class_enum) {
            stringstream err;
            err << "enum '" << with_class->fullName << "' can only be inherited by another enum";
            currThread->currTask->file->errors->createNewError(
                    GENERIC, ast->line, ast->col, err.str());
            err.str("");
            goto end;
        } else if(File::endswith(string("#") + global_class_name, base->fullName)) {
            stringstream err;
            err << "class '" << with_class->fullName << "' cannot inherit god level class `"
                << base->fullName << "` as a base class";
            currThread->currTask->file->errors->createNewError(
                    GENERIC, ast->line, ast->col, err.str());
            err.str("");
            goto end;
        }

        if(with_class->baseClass != NULL) {
            stringstream err;
            err << "class '" << with_class->fullName << "' already has a base class of `"
                << with_class->baseClass->fullName << "` and cannot be overridden";
            currThread->currTask->file->errors->createNewError(
                    GENERIC, ast->line, ast->col, err.str());
        } else with_class->baseClass = base;
    } else {
        if(with_class->type == class_enum) {
            inherit_enum_class(with_class, ast);
        } else {
            inherit_object_class(with_class, ast);
        }
    }

    end:
    delete_context();
}

void inherit_object_class(sharp_class *with_class, Ast *ast) {
    if(with_class->fullName != "std#_object_" && with_class->baseClass == NULL) {
        string stdModule = "std";
        sharp_class *sc = resolve_class(get_module(stdModule), "_object_", false, false);

        if(sc != NULL && (sc->type == class_enum)) {
            stringstream err;
            err << "support class for objects must be of type class";
            currThread->currTask->file->errors->createNewError(
                    GENERIC, ast, err.str());
        } else {
            if(sc != NULL) {
                if(!File::endswith(string("#") + global_class_name, with_class->fullName)) {
                    with_class->baseClass = sc;
                }
            } else {
                currThread->currTask->file->errors->createNewError(
                        GENERIC, ast, "support class for objects not found");
            }
        }
    }
}

void inherit_enum_class(sharp_class *with_class, Ast *ast) {
    if(with_class->fullName != "std#_enum_" && with_class->baseClass == NULL) {
        string stdModule = "std";
        sharp_class *sc = resolve_class(get_module(stdModule), "_object_", false, false);

        if(sc != NULL && (sc->type == class_enum)) {
            stringstream err;
            err << "support class for enums must be of type class";
            currThread->currTask->file->errors->createNewError(
                    GENERIC, ast, err.str());
        } else {
            if(sc != NULL) {
                with_class->baseClass = sc;
            } else {
                currThread->currTask->file->errors->createNewError(
                        GENERIC, ast, "support class for enums not found");
            }
        }
    }
}

sharp_class* resolve_base_class(sharp_class *with_class, Ast *ast) {
    if(ast != NULL) {
        sharp_type baseClass(resolve(ast->getSubAst(ast_refrence_pointer)));

        if(baseClass.type == type_class) {
            if(is_implicit_type_match(baseClass._class, with_class)) {
                currThread->currTask->file->errors->createNewError(GENERIC, ast,
                                       "cyclic dependency of class `" + with_class->fullName + "` in parent class `" +
                                       baseClass._class->fullName + "`");
                return NULL;
            }

            return baseClass._class;
        } else {
            stringstream err;
            err << " base class must be of type class but was found to be of type: " << type_to_str(baseClass);
            currThread->currTask->file->errors->createNewError(
                    GENERIC, ast, err.str());
        }

        return NULL;
    } else {
        return NULL;
    }
}
