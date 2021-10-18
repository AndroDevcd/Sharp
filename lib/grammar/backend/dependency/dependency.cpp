//
// Created by BNunnally on 8/31/2021.
//

#include "dependancy.h"
#include "../../compiler_info.h"
#include "../types/sharp_module.h"
#include "../types/sharp_class.h"
#include "../../sharp_file.h"
#include "../types/sharp_function.h"
#include "../types/sharp_field.h"
#include "../../taskdelegator/task_delegator.h"
#include "../types/sharp_alias.h"
#include "../types/import_group.h"
#include "../operation/operation.h"
#include "../postprocessor/field_processor.h"
#include "../postprocessor/alias_processor.h"
#include "../postprocessor/function_processor.h"
#include "../../settings/settings.h"
#include "../preprocessor/class_preprocessor.h"
#include "../postprocessor/class_processor.h"
#include "../astparser/ast_parser.h"
#include "../postprocessor/mutation_processor.h"
#include "../postprocessor/delegate_processor.h"

sharp_class* resolve_class(
        sharp_module* module,
        string name,
        bool isGeneric,
        bool matchName) {
    sharp_class *sc = NULL;
    GUARD(module->moduleLock)
    List<sharp_class*> *searchList = &module->classes;

    if(isGeneric) searchList = &module->genericClasses;
    for(Int i = 0; i < searchList->size(); i++) {
        sc = searchList->get(i);

        if(((matchName && sc->name.find(name) != string::npos)
            || sc->name == name) && sc->module == module) {
            return sc;
        }
    }

    return NULL;
}


sharp_class* resolve_class(
        sharp_class* sc,
        string name,
        bool isGeneric,
        bool matchName) {
    sharp_class *child = NULL;
    GUARD(globalLock)
    List<sharp_class*> *searchList = &sc->children;

    if(isGeneric) searchList = &sc->generics;
    for(Int i = 0; i < searchList->size(); i++) {
        sc = searchList->get(i);

        if((matchName && sc->name.find(name) != string::npos)
           || sc->name == name) {
            return sc;
        }
    }

    return NULL;
}

sharp_class* resolve_class(
                import_group* group,
                string name,
                bool isGeneric,
                bool matchName) {
    List<sharp_module*> &imports = group->modules;
    sharp_class *sc;

    for(Int i = 0; i < imports.size(); i++) {
        if((sc = resolve_class(
                imports.get(i),
                name,
                isGeneric,
                matchName)) != NULL) {
            return sc;
        }
    }

    return NULL;
}

import_group* resolve_import_group(sharp_file *file, string name) {
    List<import_group*> &groups = file->importGroups;

    for(Int i = 0; i < groups.size(); i++) {
        if(groups.get(i)->name == name) {
            return groups.get(i);
        }
    }

    return NULL;
}
sharp_class* resolve_class(
        sharp_file* file,
        string name,
        bool isGeneric,
        bool matchName) {
    List<sharp_module*> &imports = file->imports;
    sharp_class *sc;

    for(Int i = 0; i < imports.size(); i++) {
        if((sc = resolve_class(
                imports.get(i),
                name,
                isGeneric,
                matchName)) != NULL) {
            return sc;
        }
    }

    return NULL;
}


sharp_field* resolve_field(string name, sharp_module *module, bool checkBase) {
    sharp_class *sc = NULL;
    sharp_field *sf = NULL;
    GUARD(module->moduleLock)
    List<sharp_class*> *searchList = &module->classes;

    for(Int i = 0; i < searchList->size(); i++) {
        sc = searchList->get(i);

        if(check_flag(sc->flags, flag_global)
            && (sf = resolve_field(name, sc, checkBase)) != NULL) {
            return sf;
        }
    }

    return NULL;
}

sharp_field* resolve_field(string name, import_group *group, bool checkBase) {
    List<sharp_module*> &imports = group->modules;
    sharp_field *sf;

    for(Int i = 0; i < imports.size(); i++) {
        if((sf = resolve_field(name, imports.get(i), checkBase)) != NULL) {
            return sf;
        }
    }

    return NULL;
}

sharp_field* resolve_field(string name, sharp_file *file, bool checkBase) {
    List<sharp_module*> &imports = file->imports;
    sharp_field *sf;

    for(Int i = 0; i < imports.size(); i++) {
        if((sf = resolve_field(name, imports.get(i), checkBase)) != NULL) {
            return sf;
        }
    }

    return NULL;
}

sharp_field* resolve_local_field(string name, stored_context_item *context) {
    for(Int i = 0; i < context->localFields.size(); i++) {
        if(context->localFields.get(i)->name == name)
            return context->localFields.get(i);
    }

    return NULL;
}

sharp_field* resolve_field(string name, sharp_class *searchClass, bool checkBase) {
    GUARD(globalLock)
    for(Int i = 0; i < searchClass->fields.size(); i++) {
        if(searchClass->fields.get(i)->name == name) {
            return searchClass->fields.get(i);
        }
    }

    if(checkBase && searchClass->baseClass)
        return resolve_field(name, searchClass->baseClass, true);
    return NULL;
}


sharp_field* resolve_enum(string name, sharp_module *module) {
    sharp_class *sc = NULL;
    sharp_field *sf = NULL;
    GUARD(module->moduleLock)
    List<sharp_class*> *searchList = &module->classes;

    for(Int i = 0; i < searchList->size(); i++) {
        sc = searchList->get(i);

        if((sc->type == class_enum || check_flag(sc->flags, flag_global))
           && (sf = resolve_enum(name, sc)) != NULL) {
            return sf;
        }
    }

    return NULL;
}

sharp_field* resolve_enum(string name, sharp_file *file) {
    List<sharp_module*> &imports = file->imports;
    sharp_field *sf;

    for(Int i = 0; i < imports.size(); i++) {
        if((sf = resolve_enum(name, imports.get(i))) != NULL) {
            return sf;
        }
    }

    return NULL;
}

sharp_field* resolve_enum(string name, import_group *group) {
    List<sharp_module*> &imports = group->modules;
    sharp_field *sf;

    for(Int i = 0; i < imports.size(); i++) {
        if((sf = resolve_enum(name, imports.get(i))) != NULL) {
            return sf;
        }
    }

    return NULL;
}

sharp_field* resolve_enum(string name, sharp_class *searchClass) {
    GUARD(globalLock)

    if(searchClass->type == class_enum) {
        for (Int i = 0; i < searchClass->fields.size(); i++) {
            if (searchClass->fields.get(i)->name == name) {
                return searchClass->fields.get(i);
            }
        }
    } else {
        sharp_field *sf;
        for(Int i = 0 ; i < searchClass->children.size(); i++) {
            if(searchClass->children.get(i)->type == class_enum
                && (sf = resolve_enum(name, searchClass->children.get(i))) != NULL) {
                return sf;
            }
        }
    }

    return NULL;
}


sharp_alias* resolve_alias(string name, sharp_module *module) {
    sharp_class *sc = NULL;
    sharp_alias *sa = NULL;
    GUARD(module->moduleLock)
    List<sharp_class*> *searchList = &module->classes;

    for(Int i = 0; i < searchList->size(); i++) {
        sc = searchList->get(i);

        if(check_flag(sc->flags, flag_global)
            && (sa = resolve_alias(name, sc)) != NULL) {
            return sa;
        }
    }

    return NULL;
}

sharp_alias* resolve_alias(string name, import_group *group) {
    List<sharp_module*> &imports = group->modules;
    sharp_alias *sa;

    for(Int i = 0; i < imports.size(); i++) {
        if((sa = resolve_alias(name, imports.get(i))) != NULL) {
            return sa;
        }
    }

    return NULL;
}

sharp_alias* resolve_alias(string name, sharp_file *file) {
    List<sharp_module*> &imports = file->imports;
    sharp_alias *sa;

    for(Int i = 0; i < imports.size(); i++) {
        if((sa = resolve_alias(name, imports.get(i))) != NULL) {
            return sa;
        }
    }

    return NULL;
}

sharp_alias* resolve_alias(string name, sharp_class *searchClass) {
    for(Int i = 0; i < searchClass->aliases.size(); i++) {
        if(searchClass->aliases.get(i)->name == name) {
            return searchClass->aliases.get(i);
        }
    }

    return NULL;
}

sharp_function* resolve_function(
        string name,
        import_group *group,
        List<sharp_field*> &parameters,
        Int functionType,
        uInt excludeMatches,
        Ast *resolveLocation,
        bool checkBaseClass,
        bool implicitCheck) {
    List<sharp_module*> &imports = group->modules;
    sharp_function *sf;

    for(Int i = 0; i < imports.size(); i++) {
        if((sf = resolve_function(
                name, imports.get(i), parameters,
                functionType, excludeMatches,
                resolveLocation, checkBaseClass,
                implicitCheck)) != NULL) {
            return sf;
        }
    }

    return NULL;
}

sharp_function* resolve_function(
        string name,
        sharp_file *file,
        List<sharp_field*> &parameters,
        Int functionType,
        uInt excludeMatches,
        Ast *resolveLocation,
        bool checkBaseClass,
        bool implicitCheck) {
    List<sharp_module*> &imports = file->imports;
    sharp_function *sf;

    for(Int i = 0; i < imports.size(); i++) {
        if((sf = resolve_function(
                name, imports.get(i), parameters,
                functionType, excludeMatches,
                resolveLocation, checkBaseClass,
                implicitCheck)) != NULL) {
            return sf;
        }
    }

    return NULL;
}

sharp_function* resolve_function(
        string name,
        sharp_module *module,
        List<sharp_field*> &parameters,
        Int functionType,
        uInt excludeMatches,
        Ast *resolveLocation,
        bool checkBaseClass,
        bool implicitCheck) {
    sharp_class *sc = NULL;
    sharp_function *sf = NULL;
    GUARD(module->moduleLock)
    List<sharp_class*> *searchList = &module->classes;

    for(Int i = 0; i < searchList->size(); i++) {
        sc = searchList->get(i);

        if(check_flag(sc->flags, flag_global)
        && (sf = resolve_function(
                name, sc, parameters,
                functionType, excludeMatches,
                resolveLocation, checkBaseClass,
                implicitCheck)) != NULL) {
            return sf;
        }
    }

    return NULL;
}

// this is used for recursion protection
thread_local uInt recursionLevel = 0;
thread_local List<sharp_function*> resolvedFunctions;
sharp_function* resolve_function(
        string name,
        sharp_class *searchClass,
        List<sharp_field*> &parameters,
        Int functionType,
        uInt excludedMatches,
        Ast *resolveLocation,
        bool checkBaseClass,
        bool implicitCheck) {
    List<sharp_function*> locatedFunctions;
    sharp_function *resolvedFunction = NULL;
    bool ambiguous = false;
    recursionLevel++;
    locate_functions_with_name(name, searchClass, functionType, checkBaseClass,
            locatedFunctions);

    if(!locatedFunctions.empty()) {
        for(Int i = 0; i < locatedFunctions.size(); i++) {
            sharp_function *func = locatedFunctions.get(i);

            if(function_parameters_match(func->parameters, parameters, true)) {
                resolvedFunction = func;
                break;
            }
        }

        if(implicitCheck && resolvedFunction == NULL) {

            for(Int i = 0; i < locatedFunctions.size(); i++) {
                sharp_function *func = locatedFunctions.get(i);

                if(!resolvedFunctions.find(func)) {
                    resolvedFunctions.add(func);

                    if (function_parameters_match(
                            func->parameters,
                            parameters,
                            false,
                            excludedMatches)) {
                        if (resolvedFunction == NULL) resolvedFunction = func;
                        else ambiguous = true;
                    }
                }
            }
        }
    }

    if(ambiguous && resolveLocation != NULL) {
        currThread->currTask->file->errors->createNewError(GENERIC, resolveLocation->line, resolveLocation->col, "call to method `" + name + "` is ambiguous");
    }

    recursionLevel--;
    if(recursionLevel == 0)
        resolvedFunctions.free();
    return resolvedFunction;
}

sharp_class* resolve_class(string name, bool isGeneric, bool matchName) {
    GUARD(globalLock)

    sharp_class *sc = NULL;
    for(Int i = 0; i < modules.size(); i++) {
        if((sc = resolve_class(modules.get(i), name, isGeneric, matchName)) != NULL) {
            return sc;
        }
    }

    return NULL;
}

bool resolve_function_for_address(
        string name,
        sharp_module *module,
        Int functionType,
        bool checkBase,
        List<sharp_function*> &results) {
    sharp_class *sc = NULL;
    GUARD(module->moduleLock)
    List<sharp_class*> *searchList = &module->classes;

    for(Int i = 0; i < searchList->size(); i++) {
        sc = searchList->get(i);

        if(resolve_function_for_address(
                name,
                sc,
                functionType,
                checkBase,
                results)) {
            return true;
        }
    }

    return false;
}

bool resolve_function_for_address(
        string name,
        sharp_file *file,
        Int functionType,
        bool checkBase,
        List<sharp_function*> &results) {
    List<sharp_module*> &imports = file->imports;

    for(Int i = 0; i < imports.size(); i++) {
        if(resolve_function_for_address(
                name,
                imports.get(i),
                functionType,
                checkBase,
                results)) {
            return true;
        }
    }

    return false;
}

bool resolve_function_for_address(
        string name,
        import_group *group,
        Int functionType,
        bool checkBase,
        List<sharp_function*> &results) {
    List<sharp_module*> &imports = group->modules;

    for(Int i = 0; i < imports.size(); i++) {
        if(resolve_function_for_address(
                name,
                imports.get(i),
                functionType,
                checkBase,
                results)) {
            return true;
        }
    }

    return false;
}

bool resolve_function_for_address(
        string name,
        sharp_class *searchClass,
        Int functionType,
        bool checkBase,
        List<sharp_function*> &results) {
    return locate_functions_with_name(name, searchClass, functionType, checkBase, results);
}

bool hasFilter(uInt filters, resolve_filter filter) {
    return ((filters & filter) == filter);
}


bool resolve_local_field( // todo: support tls access for all fields
        unresolved_item &item,
        sharp_type &resultType,
        operation_scheme *scheme,
        context &ctx) {
    sharp_field *field;

    if((field = resolve_local_field(item.name, &ctx)) != NULL) {
        resultType.type = type_field;
        resultType.field = field;

        create_local_field_access_operation(scheme, field);

        if(field->type.type == type_class)
            create_dependency(ctx.functionCxt, field->type._class);
        return true;
    }

    for(Int i = ctx.storedItems.size() - 1; i >= 0; i--) {
        stored_context_item *contextItem = ctx.storedItems.get(i);

        if((field = resolve_local_field(item.name, contextItem)) != NULL) {

            if(ctx.functionCxt == contextItem->functionCxt) { // same func no closure needed
                resultType.field = field;

                create_local_field_access_operation(scheme, field);

                if(field->type.type == type_class)
                    create_dependency(ctx.functionCxt, field->type._class);
            } else {
                if(can_capture_closure(field)) {
                    sharp_class *closure_class = create_closure_class(
                            currThread->currTask->file, currModule, contextItem->functionCxt,
                            item.ast);
                    sharp_field *closure = create_closure_field(closure_class, item.name, // todo: add scopeId to field struct and apply this to name for nested scoped locals w/ same name
                            field->type, item.ast);
                    sharp_field *staticClosureRef = create_closure_field(
                            resolve_class(currModule, global_class_name, false, false),
                            "closure_ref_" + contextItem->functionCxt->fullName,
                            sharp_type(closure_class),
                            item.ast
                    );

                    contextItem->functionCxt->closure = staticClosureRef;
                    field->closure = closure;
                    resultType.field = closure;

                    create_static_field_access_operation(scheme, staticClosureRef);
                    create_instance_field_access_operation(scheme, closure);

                    if(field->type.type == type_class)
                        create_dependency(ctx.functionCxt, field->type._class);
                    create_dependency(ctx.functionCxt, closure_class);
                    create_dependency(ctx.functionCxt, staticClosureRef);
                } else {
                    resultType.field = field;
                    currThread->currTask->file->errors->createNewError(GENERIC, item.ast,
                            "cannot capture closure of local field `" + field->name + "`. Only types of (arrays, classes, & objects) are allowed.");
                }
            }

            resultType.type = type_field;
            return true;
        }
    }

    return false;
}

void check_access(
        string &type,
        string &name,
        uInt flags,
        context &ctx,
        bool inPrimaryClass,
        sharp_class *owner,
        Ast *ast) {

    if(owner != NULL) {
        sharp_class *primary = get_primary_class(&ctx);

        if (check_flag(flags, flag_local)
            && currThread->currTask->file != owner->implLocation.file) {
            currThread->currTask->file->errors->createNewError(GENERIC, ast,
                                                               "cannot access local " + type + " `" + name +
                                                               "`. outside of home file: `"
                                                               + owner->implLocation.file->name + "`.");
            return;
        }

        if (check_flag(flags, flag_private) && !inPrimaryClass) {
            currThread->currTask->file->errors->createNewError(GENERIC, ast,
                                                               "cannot access private " + type + " `" + name +
                                                               "`. outside of class: `"
                                                               + owner->fullName + "`.");
            return;
        }

        if (check_flag(flags, flag_protected) && !inPrimaryClass
            && !((primary->module == owner->module) || is_implicit_type_match(owner, primary))) {
            currThread->currTask->file->errors->createNewError(GENERIC, ast,
                                                               "cannot access protected " + type + " `" + name +
                                                               "`. outside of class: `"
                                                               + owner->fullName + "`.");
            return;
        }
    }
}

bool resolve_primary_class_field(
        unresolved_item &item,
        sharp_class *primaryClass,
        bool isSelfInstance,
        bool isStatic,
        sharp_type &resultType,
        operation_scheme *scheme,
        context &ctx) {

    sharp_field *field;
    if((field = resolve_field(item.name, primaryClass, true)) != NULL) {
        resultType.type = type_field;
        resultType.field = field;
        process_field(field);

        if(ctx.isStatic && !check_flag(field->flags, flag_static)
            && isSelfInstance) {
            sharp_function *fun = get_primary_function(&ctx);

            if(fun != NULL && !check_flag(fun->flags, flag_static)) {
                currThread->currTask->file->errors->createNewError(GENERIC, item.ast,
                     "cannot capture closure of instance field `" + field->name
                     + "` from static context. Try accessing `self` to explicitly capture class closure.");
            } else {
                currThread->currTask->file->errors->createNewError(GENERIC, item.ast,
                         "cannot access instance field `" + field->name + "` from static context.");
            }
        } else {

            if(isStatic && !check_flag(field->flags, flag_static)) {
                currThread->currTask->file->errors->createNewError(GENERIC, item.ast,
                         "cannot access instance field `" + field->name + "` from static context.");
            }

            if (field->getter != NULL) {
                if (!isSelfInstance){
                    if(isStatic)
                        create_static_field_getter_operation(scheme, field);
                    else
                        create_instance_field_getter_operation(scheme, field);
                }
                else {
                        create_primary_instance_field_getter_operation(scheme, field);
                }
                create_dependency(ctx.functionCxt, field->getter);
            } else {
                if (!isSelfInstance) {
                    if(isStatic)
                        create_static_field_access_operation(scheme, field);
                    else
                        create_instance_field_access_operation(scheme, field);
                }
                else create_primary_instance_field_access_operation(scheme, field);
            }
        }

        string fieldType = "field";
        check_access(fieldType, field->fullName,
                field->flags, ctx, isSelfInstance,
                field->owner, item.ast);
        if(ctx.type == block_context)
            create_dependency(ctx.functionCxt, field);
        return true;
    } else return false;
}

bool resolve_primary_class_alias(
        unresolved_item &item,
        sharp_class *primaryClass,
        bool isSelfInstance,
        sharp_type &resultType,
        operation_scheme *scheme,
        context &ctx) {

    sharp_alias *alias;
    if((alias = resolve_alias(item.name, primaryClass)) != NULL) {
        process_alias(alias);
        resultType.copy(alias->type);

        if(alias->operation && scheme) {
            scheme->copy(*alias->operation);
        }

        string fieldType = "alias";
        check_access(fieldType, alias->fullName,
                     alias->flags, ctx, isSelfInstance,
                     alias->owner, item.ast);

        if(ctx.type == block_context && alias->type.type == type_field)
            create_dependency(ctx.functionCxt, alias->type.field);
        else if(ctx.type == block_context && alias->type.type == type_function)
            create_dependency(ctx.functionCxt, alias->type.fun);
        return true;
    } else return false;
}

bool resolve_primary_class_enum(
        unresolved_item &item,
        sharp_class *primaryClass,
        bool isSelfInstance,
        sharp_type &resultType,
        operation_scheme *scheme,
        context &ctx) {

    sharp_field *field;
    if((field = resolve_enum(item.name, primaryClass)) != NULL) {
        resultType.type = type_field;
        resultType.field = field;

        create_static_field_access_operation(scheme, field);


        string fieldType = "enum";
        check_access(fieldType, field->fullName,
                     field->owner->flags, ctx, isSelfInstance,
                     field->owner, item.ast);

        if(ctx.type == block_context)
            create_dependency(ctx.functionCxt, field);
        return true;
    } else return false;
}

bool resolve_primary_class_function_address(
        unresolved_item &item,
        sharp_class *primaryClass,
        bool isSelfInstance,
        sharp_type &resultType,
        operation_scheme *scheme,
        context &ctx) {

    List<sharp_function*> functions;
    if(resolve_function_for_address(
            item.name,
            primaryClass,
            undefined_function,
            true,
            functions)) {
        if(functions.size() > 1) {
            create_new_warning(GENERIC, __w_ambig, item.ast->line,item.ast->col,
                    " getting address from ambiguous function reference `" + item.name + "` in class " + primaryClass->fullName + "`");
        }

        if(!check_flag(functions.first()->flags, flag_static)) {
            currThread->currTask->file->errors->createNewError(GENERIC,
                         item.ast, " cannot get address of non-static function `" + functions.first()->fullName + "` ");
        }

        resultType.type = type_function_ptr;
        resultType.fun = functions.first();
        process_function_return_type(functions.first());

        create_get_static_function_address_operation(scheme, functions.first());


        string fieldType = "function";
        check_access(fieldType, functions.first()->fullName,
                     functions.first()->flags, ctx, isSelfInstance,
                     functions.first()->owner, item.ast);
        if(ctx.type == block_context)
            create_dependency(ctx.functionCxt, functions.first());
        return true;
    } else return false;
}

bool resolve_primary_class_inner_class(
        unresolved_item &item,
        sharp_class *primaryClass,
        bool isSelfInstance,
        sharp_type &resultType,
        operation_scheme *scheme,
        bool isGeneric,
        context &ctx) {

    sharp_class *sc;
    if((sc = resolve_class(primaryClass, item.name, isGeneric, false)) != NULL) {
        resultType.type = type_class;
        resultType._class = sc;


        string fieldType = "class";
        check_access(fieldType, sc->fullName,
                     sc->flags, ctx, isSelfInstance,
                     sc->owner, item.ast);
        if(ctx.type == block_context)
            create_dependency(ctx.functionCxt, sc);
        create_dependency(primaryClass, sc);
        return true;
    } else return false;
}

bool resolve_primary_class(
        unresolved_item &item,
        sharp_class *primaryClass,
        sharp_type &resultType,
        operation_scheme *scheme,
        context &ctx) {

    if(primaryClass->name == item.name) {
        resultType.type = type_class;
        resultType._class = primaryClass;

        if(ctx.type == block_context)
            create_dependency(ctx.functionCxt, primaryClass);
        create_dependency(primaryClass, primaryClass);
        return true;
    } else return false;
}

bool resolve_generic_type_param(
        unresolved_item &item,
        sharp_class *primaryClass,
        sharp_type &resultType,
        operation_scheme *scheme,
        context &ctx) {

    for(Int i = 0; i < primaryClass->genericTypes.size(); i++) {
        if(primaryClass->genericTypes.get(i).name == item.name) {
            resultType.copy(primaryClass->genericTypes.get(i).type);

            if(resultType.type == type_class) {
                if(ctx.type == block_context)
                    create_dependency(ctx.functionCxt, resultType._class);
            }
            return true;
        }
    }

    return false;
}

bool resolve_global_class_field(
        unresolved_item &item,
        sharp_type &resultType,
        operation_scheme *scheme,
        context &ctx) {

    sharp_field *field;
    if(resultType.type == type_untyped) {
        field = resolve_field(item.name, currThread->currTask->file);
    } else {
        if(resultType.type == type_import_group) {
            field = resolve_field(item.name, resultType.group);
        } else {
            field = resolve_field(item.name, resultType.module);
        }
    }

    if(field != NULL) {
        resultType.type = type_field;
        resultType.field = field;
        if(check_flag(field->flags, flag_local)
            && field->implLocation.file != currThread->currTask->file) {
            currThread->currTask->file->errors->createNewError(GENERIC, item.ast->line,item.ast->col,
                   " cannot access local field `" + field->name + "` from outside file `" + field->implLocation.file->name + "`.");
        }

        process_field(field);

        if(field->getter != NULL) {
            create_static_field_getter_operation(scheme, field);
            create_dependency(ctx.functionCxt, field->getter);
        } else {
            create_static_field_access_operation(scheme, field);
        }


        string fieldType = "field";
        check_access(fieldType, field->fullName,
                     field->flags, ctx, false,
                     field->owner, item.ast);
        if(ctx.type == block_context)
            create_dependency(ctx.functionCxt, field);
        return true;
    } else return false;
}

bool resolve_global_class_alias(
        unresolved_item &item,
        sharp_type &resultType,
        operation_scheme *scheme,
        context &ctx) {

    sharp_alias *alias;
    if(resultType.type == type_untyped) {
        alias = resolve_alias(item.name, currThread->currTask->file);
    } else {
        if(resultType.type == type_import_group) {
            alias = resolve_alias(item.name, resultType.group);
        } else {
            alias = resolve_alias(item.name, resultType.module);
        }
    }

    if(alias != NULL) {
        if(check_flag(alias->flags, flag_local)
           && alias->location.file != currThread->currTask->file) {
            currThread->currTask->file->errors->createNewError(GENERIC, item.ast->line,item.ast->col,
                " cannot access local alias `" + alias->name + "` from outside file `" + alias->location.file->name + "`.");
        }

        process_alias(alias);
        resultType.copy(alias->type);

        if(alias->operation && scheme) {
            scheme->copy(*alias->operation);
        }


        string fieldType = "alias";
        check_access(fieldType, alias->fullName,
                     alias->flags, ctx, false,
                     alias->owner, item.ast);
        if(ctx.type == block_context && alias->type.type == type_field)
            create_dependency(ctx.functionCxt, alias->type.field);
        else if(ctx.type == block_context && alias->type.type == type_function)
            create_dependency(ctx.functionCxt, alias->type.fun);
        return true;
    } else return false;
}

bool resolve_global_class_enum(
        unresolved_item &item,
        sharp_type &resultType,
        operation_scheme *scheme,
        context &ctx) {

    sharp_field *field;
    if(resultType.type == type_untyped) {
        field = resolve_enum(item.name, currThread->currTask->file);
    } else {
        if(resultType.type == type_import_group) {
            field = resolve_enum(item.name, resultType.group);
        } else {
            field = resolve_enum(item.name, resultType.module);
        }
    }

    if(field != NULL) {
        resultType.type = type_field;
        resultType.field = field;
        if(check_flag(field->owner->flags, flag_local)
           && field->implLocation.file != currThread->currTask->file) {
            currThread->currTask->file->errors->createNewError(GENERIC, item.ast->line,item.ast->col,
                                                               " cannot access local enum `" + field->name + "` from outside file `" + field->implLocation.file->name + "`.");
        }

        create_static_field_access_operation(scheme, field);


        string fieldType = "enum";
        check_access(fieldType, field->fullName,
                     field->flags, ctx, false,
                     field->owner, item.ast);
        if(ctx.type == block_context)
            create_dependency(ctx.functionCxt, field);
        return true;
    } else return false;
}


bool resolve_global_class_function_address(
        unresolved_item &item,
        sharp_type &resultType,
        operation_scheme *scheme,
        context &ctx) {

    List<sharp_function*> functions;
    bool success = false;

    if(resultType.type == type_untyped) {
        success = resolve_function_for_address(
                item.name,
                currThread->currTask->file,
                undefined_function,
                true,
                functions);
    } else {
        if(resultType.type == type_import_group) {
            success = resolve_function_for_address(
                    item.name,
                    resultType.group,
                    undefined_function,
                    true,
                    functions);
        } else {
            success = resolve_function_for_address(
                    item.name,
                    resultType.module,
                    undefined_function,
                    true,
                    functions);
        }
    }

    if(success) {
        if(functions.size() > 1) {
            create_new_warning(GENERIC, __w_ambig, item.ast->line,item.ast->col,
                               " getting address from ambiguous function reference `" + item.name + "` in class " + functions.first()->owner->fullName + "`");
        }

        if(check_flag(functions.first()->flags, flag_local)
           && functions.first()->implLocation.file != currThread->currTask->file) {
            currThread->currTask->file->errors->createNewError(GENERIC, item.ast->line,item.ast->col,
                  " cannot access local function `" + functions.first()->name + "` from outside file `" + functions.first()->implLocation.file->name + "`.");
        }

        if(!check_flag(functions.first()->flags, flag_static)) {
            currThread->currTask->file->errors->createNewError(GENERIC,
                                                               item.ast, " cannot get address of non-static function `" + functions.first()->fullName + "` ");
        }

        resultType.type = type_function_ptr;
        resultType.fun = functions.first();
        process_function_return_type(functions.first());

        create_get_static_function_address_operation(scheme, functions.first());


        string fieldType = "function";
        check_access(fieldType, functions.first()->fullName,
                     functions.first()->flags, ctx, false,
                     functions.first()->owner, item.ast);
        if(ctx.type == block_context)
            create_dependency(ctx.functionCxt, functions.first());
        return true;
    } else return false;
}

bool resolve_global_class(
        unresolved_item &item,
        sharp_class *primaryClass,
        sharp_type &resultType,
        bool isGeneric,
        context &ctx) {

    sharp_class *sc;
    if(resultType.type == type_untyped) {
        sc = resolve_class(currThread->currTask->file, item.name, isGeneric, false);
    } else {
        if(resultType.type == type_import_group) {
            sc = resolve_class(resultType.group, item.name, isGeneric, false);
        } else {
            sc = resolve_class(resultType.module, item.name, isGeneric, false);
        }
    }

    if(sc != NULL) {
        resultType.type = type_class;
        resultType._class = sc;

        string fieldType = "class";
        check_access(fieldType, sc->fullName,
                     sc->flags, ctx, false,
                     sc->owner, item.ast);
        if(ctx.type == block_context)
            create_dependency(ctx.functionCxt, sc);
        create_dependency(primaryClass, sc);
        return true;
    } else return false;
}

// resolve logic flow
// local fields -> class instance / static fields -> class aliases -> class enums -> function reference via name
// local class (inner class inside primary class) -> Primary class -> genericType -> global field -> global alias -> global enums
// -> global function reference -> global_class
void resolve_normal_item(
        unresolved_item &item,
        sharp_type &resultType,
        bool &hardType,
        operation_scheme *scheme,
        uInt filter,
        Ast *resolveLocation) {
    context &context = currThread->currTask->file->context;


    if(resultType.type == type_untyped) {
        // first item
        if(context.type == block_context
            && hasFilter(filter, resolve_filter_local_field)
            && resolve_local_field(item, resultType, scheme, context)) {

            hardType = false;
            resultType.nullable = resultType.field->type.nullable;
            return;
        }

        sharp_class *primaryClass = get_primary_class(&context);
        if(primaryClass) {
            if(hasFilter(filter, resolve_filter_class_field)
               && resolve_primary_class_field(item, primaryClass, true, false, resultType, scheme, context)) {

                hardType = false;
                resultType.nullable = resultType.field->type.nullable;
                return;
            }

            if(hasFilter(filter, resolve_filter_class_alias)
               && resolve_primary_class_alias(item, primaryClass, true, resultType, scheme, context)) {

                hardType = false;
                return;
            }

            if(hasFilter(filter, resolve_filter_class_enum)
               && resolve_primary_class_enum(item, primaryClass, true, resultType, scheme, context)) {

                hardType = false;
                return;
            }

            if(hasFilter(filter, resolve_filter_function_address)
               && resolve_primary_class_function_address(item, primaryClass, true, resultType, scheme, context)) {

                hardType = false;
                return;
            }

            if(hasFilter(filter, resolve_filter_inner_class)
               && resolve_primary_class_inner_class(item, primaryClass, true, resultType, scheme, false, context)) {

                hardType = false;
                return;
            }

            if(hasFilter(filter, resolve_filter_generic_inner_class)
               && resolve_primary_class_inner_class(item, primaryClass, true, resultType, scheme, true, context)) {

                hardType = true;
                return;
            }

            if(hasFilter(filter, resolve_filter_class)
               && resolve_primary_class(item, primaryClass, resultType, scheme, context)) {

                hardType = true;
                return;
            }

            if(hasFilter(filter, resolve_filter_generic_type_param)
               && !primaryClass->genericTypes.empty()
               && resolve_generic_type_param(item, primaryClass, resultType, scheme, context)) {

                hardType = true;
                return;
            }
        }

        if(hasFilter(filter, resolve_filter_field)
           && resolve_global_class_field(item, resultType, scheme, context)) {

            hardType = false;
            resultType.nullable = resultType.field->type.nullable;
            return;
        }

        if(hasFilter(filter, resolve_filter_alias)
           && resolve_global_class_alias(item, resultType, scheme, context)) {

            hardType = true;
            return;
        }

        if(hasFilter(filter, resolve_filter_enum)
           && resolve_global_class_enum(item, resultType, scheme, context)) {

            hardType = false;
            return;
        }


        if (hasFilter(filter, resolve_filter_class)
            && resolve_global_class(item, primaryClass, resultType, false, context)) {

            hardType = true;
            return;
        }


        if (hasFilter(filter, resolve_filter_generic_class)
            && resolve_global_class(item, primaryClass, resultType, true, context)) {

            hardType = true;
            return;
        }

        if(hasFilter(filter, resolve_filter_function_address)
           && resolve_global_class_function_address(item, resultType, scheme, context)) {

            hardType = false;
            return;
        }

        primaryClass = resolve_class(currThread->currTask->file, item.name, false, true);

        resultType.type = type_undefined;

        if(primaryClass == NULL) {
            currThread->currTask->file->errors->createNewError(COULD_NOT_RESOLVE,
                                                               resolveLocation, " `" + item.name + "` ");
        } else {
            currThread->currTask->file->errors->createNewError(COULD_NOT_RESOLVE,
                       resolveLocation, " `" + item.name + "` did you possibly mean: `" + primaryClass->fullName + "`?");
        }
    } else if(resultType.type == type_module || resultType.type == type_import_group) {
        sharp_class *primaryClass = get_primary_class(&context);

        if(hasFilter(filter, resolve_filter_field)
           && resolve_global_class_field(item, resultType, scheme, context)) {

            hardType = false;
            resultType.nullable = resultType.field->type.nullable;
            return;
        }

        if(hasFilter(filter, resolve_filter_alias)
           && resolve_global_class_alias(item, resultType, scheme, context)) {

            hardType = true;
            return;
        }

        if(hasFilter(filter, resolve_filter_enum)
           && resolve_global_class_enum(item, resultType, scheme, context)) {

            hardType = false;
            return;
        }

        if(primaryClass) {
            if (hasFilter(filter, resolve_filter_class)
                && resolve_global_class(item, primaryClass, resultType, false, context)) {

                hardType = true;
                return;
            }

            if (hasFilter(filter, resolve_filter_generic_class)
                && resolve_global_class(item, primaryClass, resultType, true, context)) {

                hardType = true;
                return;
            }
        }

        if(hasFilter(filter, resolve_filter_function_address)
           && resolve_global_class_function_address(item, resultType, scheme, context)) {

            hardType = false;
            return;
        }
    } else {
        bool nullable = resultType.nullable;

        if(resultType.type == type_class) {
            sharp_class *primaryClass = resultType._class;

            if(hasFilter(filter, resolve_filter_class_field)
               && resolve_primary_class_field(item, primaryClass, false, hardType, resultType, scheme, context)) {

                if(!hardType) {
                    if(nullable) {
                        if(item.accessType == access_normal) {
                            currThread->currTask->file->errors->createNewError(GENERIC, item.ast->line,item.ast->col,
                                      "accessing field `" + resultType.field->name + "` from nullable without `?.` or `!!.`.");
                        } else if(item.accessType == access_forced) resultType.nullable = false;
                    }
                } else {
                    if (item.accessType != access_normal) {
                        create_new_warning(GENERIC, __w_access, item.ast->line, item.ast->col,
                                           " unnessicary access type `" + access_type_to_str(item.accessType)
                                           + "` on statically accessed field.");
                    }
                }

                hardType = false;
                return;
            }

            if(hasFilter(filter, resolve_filter_class_alias)
               && hardType
               && resolve_primary_class_alias(item, primaryClass, false, resultType, scheme, context)) {

                hardType = true;
                if(item.accessType != access_normal) {
                    create_new_warning(GENERIC, __w_access, item.ast->line,item.ast->col,
                         " unnessicary access type `" + access_type_to_str(item.accessType)
                         + "` on statically accessed alias.");
                }
                return;
            }

            if(hasFilter(filter, resolve_filter_function_address)
               && resolve_primary_class_function_address(item, primaryClass, false, resultType, scheme, context)) {

                hardType = false;
                if(item.accessType != access_normal) {
                    create_new_warning(GENERIC, __w_access, item.ast->line,item.ast->col,
                                       " unnessicary access type `" + access_type_to_str(item.accessType)
                                       + "` on statically accessed function.");
                }
                return;
            }

            if(hasFilter(filter, resolve_filter_inner_class)
               && hardType
               && resolve_primary_class_inner_class(item, primaryClass, false, resultType, scheme, false, context)) {

                hardType = true;
                if(item.accessType != access_normal) {
                    create_new_warning(GENERIC, __w_access, item.ast->line,item.ast->col,
                                       " unnessicary access type `" + access_type_to_str(item.accessType)
                                       + "` on statically accessed inner class.");
                }
                return;
            }

            if(hasFilter(filter, resolve_filter_generic_inner_class)
               && hardType
               && resolve_primary_class_inner_class(item, primaryClass, false, resultType, scheme, true, context)) {

                hardType = true;
                if(item.accessType != access_normal) {
                    create_new_warning(GENERIC, __w_access, item.ast->line,item.ast->col,
                                       " unnessicary access type `" + access_type_to_str(item.accessType)
                                       + "` on statically accessed inner class.");
                }
                return;
            }

            primaryClass = resolve_class(primaryClass, item.name, false, true);

            resultType.type = type_undefined;
            if(primaryClass == NULL) {
                currThread->currTask->file->errors->createNewError(COULD_NOT_RESOLVE,
                                                                   resolveLocation, " `" + item.name + "` ");
            } else {
                currThread->currTask->file->errors->createNewError(COULD_NOT_RESOLVE,
                                                                   resolveLocation, " `" + item.name + "` did you possibly mean: `" + primaryClass->fullName + "`?");
            }
        } else if(resultType.type == type_field) {
            sharp_field *field = resultType.field;
            nullable = field->type.nullable;

            if(field->type.type == type_class) {
                sharp_class *primaryClass = field->type._class;

                if(hasFilter(filter, resolve_filter_class_field)
                   && resolve_primary_class_field(item, primaryClass, false, false, resultType, scheme, context)) {

                    hardType = false;
                    if(nullable) {
                        if(item.accessType == access_normal) {
                            currThread->currTask->file->errors->createNewError(GENERIC, item.ast->line,item.ast->col,
                                   "accessing nullable field `" + field->name + "` without `?.` or `!!.`.");
                        } else if(item.accessType == access_forced) resultType.nullable = false;
                    }
                    return;
                }

                if(hasFilter(filter, resolve_filter_class_alias)
                   && resolve_primary_class_alias(item, primaryClass, false, resultType, scheme, context)) {

                    hardType = true;
                    create_new_warning(GENERIC, __w_access, item.ast->line,item.ast->col,
                                       " unusual access of alias through field `" + field->name
                                       + "`. Aliases have static access by default so you could also type `"
                                       + field->owner->fullName + "." + item.name + "`");
                    return;
                }

                if(hasFilter(filter, resolve_filter_function_address)
                   && resolve_primary_class_function_address(item, primaryClass, false, resultType, scheme, context)) {

                    hardType = false;
                    create_new_warning(GENERIC, __w_access, item.ast->line,item.ast->col,
                                       " unusual access of static function through field `" + field->name
                                       + "`. Getting addresses of static functions can also be accessed this way `"
                                       + field->owner->fullName + "." + item.name + "`");
                    return;
                }

                if(hasFilter(filter, resolve_filter_inner_class)
                   && resolve_primary_class_inner_class(item, primaryClass, false, resultType, scheme, false, context)) {

                    hardType = true;
                    currThread->currTask->file->errors->createNewError(GENERIC, item.ast->line,item.ast->col,
                                       " cannot access inner class through field `" + field->name + "`.");
                    return;
                }

                if(hasFilter(filter, resolve_filter_generic_inner_class)
                   && resolve_primary_class_inner_class(item, primaryClass, false, resultType, scheme, true, context)) {

                    hardType = true;
                    currThread->currTask->file->errors->createNewError(GENERIC, item.ast->line,item.ast->col,
                                       " cannot access inner class through field `" + field->name + "`.");
                    return;
                }

                resultType.type = type_undefined;
                currThread->currTask->file->errors->createNewError(COULD_NOT_RESOLVE,
                                resolveLocation, " `" + item.name + "` in field `" + field->fullName + "`");
            } else {

                currThread->currTask->file->errors->createNewError(GENERIC, item.ast->line,item.ast->col,
                                       "field `" + field->name + "` of type `" + type_to_str(field->type) + "` must be of type class.");
            }
        }
        else {
            currThread->currTask->file->errors->createNewError(COULD_NOT_RESOLVE,
                               resolveLocation, " `" + item.name + "` after non class type `" + type_to_str(resultType) + "`?");
        }
    }
}

void resolve_module_item(
        unresolved_item &item,
        sharp_type &resultType,
        Ast *resolveLocation) {

    if(item.name.find('.') == string::npos) {
        sharp_file *file = currThread->currTask->file;
        import_group *group = resolve_import_group(file, item.name);
        if(group != NULL) {
            resultType.type = type_import_group;
            resultType.group = group;
            return;
        }
    }

    sharp_module *module = get_module(item.name);
    if(module) {
        resultType.type = type_module;
        resultType.module = module;
    }
}

void resolve_operator_item(
        unresolved_item &item,
        sharp_type &resultType,
        bool &hardType,
        operation_scheme *scheme,
        uInt filter,
        Ast *resolveLocation) {
    context &context = currThread->currTask->file->context;

    if(resultType.type == type_untyped) {
        // first item
        sharp_class *primaryClass = get_primary_class(&context);
        if(primaryClass) {
            if(hasFilter(filter, resolve_filter_function_address)
               && resolve_primary_class_function_address(item, primaryClass, true, resultType, scheme, context)) {

                hardType = false;
                return;
            }
        }

        if(hasFilter(filter, resolve_filter_function_address)
           && resolve_global_class_function_address(item, resultType, scheme, context)) {

            hardType = false;
            return;
        }

        resultType.type = type_undefined;

        currThread->currTask->file->errors->createNewError(COULD_NOT_RESOLVE,
                                                               resolveLocation, " `" + item.name + "` ");
    } else if(resultType.type == type_module || resultType.type == type_import_group) {
        if(hasFilter(filter, resolve_filter_function_address)
           && resolve_global_class_function_address(item, resultType, scheme, context)) {

            hardType = false;
            return;
        }
    } else {
        if(resultType.type == type_class) {
            sharp_class *primaryClass = resultType._class;

            if(hasFilter(filter, resolve_filter_function_address)
               && resolve_primary_class_function_address(item, primaryClass, false, resultType, scheme, context)) {

                hardType = false;
                return;
            }

            resultType.type = type_undefined;
            currThread->currTask->file->errors->createNewError(COULD_NOT_RESOLVE,
                                    resolveLocation, " `" + item.name + "` ");
        } else {
            currThread->currTask->file->errors->createNewError(COULD_NOT_RESOLVE,
                                                               resolveLocation, " `" + item.name + "` after non class type `" + type_to_str(resultType) + "`?");
        }
    }
}

void resolve_function_ptr_item(
        unresolved_item &item,
        sharp_type &resultType,
        bool &hardType,
        operation_scheme *scheme,
        uInt filter,
        Ast *resolveLocation) {
    List<sharp_field*> params;
    sharp_type returnType;
    uInt flags = flag_public;
    impl_location location(currThread->currTask->file, resolveLocation->line, resolveLocation->col);
    context &context = currThread->currTask->file->context;
    sharp_class *primaryClass = get_primary_class(&context);

    for(Int i = 0; i < item.typeSpecifiers.size(); i++) {
        sharp_type resolvedType;
        resolve(*item.typeSpecifiers.get(i), resolvedType, hardType,
                resolve_hard_type, item.ast, NULL);

        string name = "";
        params.add(new sharp_field(
                name, primaryClass, location,
                resolvedType, flags, normal_field,
                resolveLocation));
    }

    if(item.returnType) {
        resolve(*item.returnType, returnType, hardType,
                resolve_hard_type, item.ast, NULL);
    } else returnType.type = type_nil;

    hardType = true;
    string ptr = "fptr";
    sharp_function *fptr = new sharp_function(
            ptr, primaryClass, location,
            flags, resolveLocation, params,
            returnType, blueprint_function);

    resultType.type = type_function_ptr;
    resultType.fun = fptr;
}


bool resolve_primary_class_function(
        unresolved_item &item,
        sharp_class *primaryClass,
        bool isPrimaryClass,
        bool isStaticCall,
        sharp_type &resultType,
        operation_scheme *scheme,
        context &ctx) {
    // we assume that the scheme is already setup before coming here with code for params

    List<sharp_field*> params;
    for(Int i = 0; i < item.typeSpecifiers.size(); i++) {

        string name = "param";
        impl_location location;
        params.add(new sharp_field(
                name,
                NULL,
                location,
                *item.typeSpecifiers.get(i),
                flag_none,
                normal_field,
                item.ast
                ));
    }

    sharp_function* fun;
    if((fun = resolve_function(
            item.name,
            primaryClass,
            params,
            undefined_function,
            match_operator_overload | match_initializer,
            item.ast,
            true,
            true)) != NULL) {
        process_function_return_type(fun);

        if((isStaticCall || (isPrimaryClass && ctx.isStatic)) && !check_flag(fun->flags, flag_static)) {
            if(isPrimaryClass && ctx.isStatic) {
                currThread->currTask->file->errors->createNewError(GENERIC, item.ast,
                          "cannot capture closure from instance function `" + fun->fullName +
                          "` from static context, `self->` is required before function call.");
            } else {
                currThread->currTask->file->errors->createNewError(GENERIC, item.ast,
                           "cannot access instance function `" + fun->fullName +
                          "` from static context.");
            }
        }

        resultType.copy(fun->returnType);

        compile_function_call(
                scheme, params, item.operations,
                fun, isStaticCall, isPrimaryClass
        );

        if(ctx.type == block_context)
            create_dependency(ctx.functionCxt, fun);
        return true;
    } else return false;
}

bool resolve_global_class_function(
        unresolved_item &item,
        sharp_type &resultType,
        operation_scheme *scheme,
        context &ctx) {
    // we assume that the scheme is already setup before coming here with code for params

    List<sharp_field*> params;
    for(Int i = 0; i < item.typeSpecifiers.size(); i++) {
        string name = "param";
        impl_location location;
        params.add(new sharp_field(
                name,
                NULL,
                location,
                *item.typeSpecifiers.get(i),
                flag_none,
                normal_field,
                item.ast
                ));
    }

    sharp_function* fun;

    if(resultType.type == type_import_group) {
        fun = resolve_function(
                item.name,
                resultType.group,
                params,
                undefined_function,
                match_operator_overload | match_initializer,
                item.ast,
                true,
                true);
    } else if(resultType.type == type_module) {
        fun = resolve_function(
                item.name,
                resultType.module,
                params,
                undefined_function,
                match_operator_overload | match_initializer,
                item.ast,
                true,
                true);
    } else {
        fun = resolve_function(
                item.name,
                currThread->currTask->file,
                params,
                undefined_function,
                match_operator_overload | match_initializer,
                item.ast,
                true,
                true);
    }

    if(fun != NULL) {
        resultType.type = type_function;
        resultType.fun = fun;
        process_function_return_type(fun);
        if(fun->returnType.type == type_untyped || fun->returnType.type == type_undefined) {
            resultType.type = type_undefined;
            return true;
        }

        resultType.nullable = fun->returnType.nullable;
        resultType.isArray = fun->returnType.isArray;

        if(scheme) {

            compile_function_call(
                    scheme, params, item.operations,
                    fun, true, false
            );
        }

        if(ctx.type == block_context)
            create_dependency(ctx.functionCxt, fun);
        params.free();
        return true;
    } else {
        params.free();
        return false;
    }
}

void resolve_function_reference_item(
        unresolved_item &item,
        sharp_type &resultType,
        bool &hardType,
        operation_scheme *scheme,
        uInt filter,
        Ast *resolveLocation) {
    context &context = currThread->currTask->file->context;

    List<sharp_field*> params;
    for(Int i = 0; i < item.typeSpecifiers.size(); i++) {

        string name = "param";
        impl_location location;
        params.add(new sharp_field(
                name,
                NULL,
                location,
                *item.typeSpecifiers.get(i),
                flag_none,
                normal_field,
                item.ast
        ));
    }

    sharp_type noType;
    sharp_function mock_function(item.name, get_primary_class(&context), impl_location(),
                                 flag_none, NULL, params, noType, undefined_function, true);

    string unresolvedFunction;
    noType.type = type_function;
    noType.fun = &mock_function;
    unresolvedFunction = type_to_str(noType);
    params.free();

    if(item.name == "append" && resolveLocation->line == 73 && resolveLocation->col == 30) {
        int i = 0;
    }

    if(resultType.type == type_untyped) {
        // first item
        sharp_class *primaryClass = get_primary_class(&context);
        if(primaryClass && !check_flag(primaryClass->flags, flag_global)
            && resolve_primary_class_function(
                    item, primaryClass, true,
                    false, resultType, scheme,
                    context)) {

            hardType = false;
            return;
        }

        if(resolve_global_class_function(item, resultType, scheme, context)) {
            return;
        }

        resultType.type = type_undefined;
        currThread->currTask->file->errors->createNewError(COULD_NOT_RESOLVE,
                               resolveLocation, " `" + unresolvedFunction + "` ");
    } else if(resultType.type == type_module || resultType.type == type_import_group) {
        if(resolve_global_class_function(item, resultType, scheme, context)) {
            return;
        }
    } else {
        if(resultType.type == type_class) {
            sharp_class *primaryClass = resultType._class;

            if(resolve_primary_class_function(
                    item, primaryClass, false, hardType,
                    resultType, scheme, context)) {

                hardType = false;
                return;
            }

            resultType.type = type_undefined;
            currThread->currTask->file->errors->createNewError(COULD_NOT_RESOLVE,
                                                               resolveLocation, " `" + unresolvedFunction + "` ");
        } else if(resultType.type == type_field) {
            if(resultType.field->type.type == type_class) {
                sharp_class *primaryClass = resultType.field->type._class;

                if (resolve_primary_class_function(
                        item, primaryClass, false, false,
                        resultType, scheme, context)) {

                    hardType = false;
                    return;
                }

                currThread->currTask->file->errors->createNewError(COULD_NOT_RESOLVE,
                               resolveLocation, " `" + unresolvedFunction + "` ");
            } else {
                resultType.type = type_undefined;
                currThread->currTask->file->errors->createNewError(GENERIC,
                             resolveLocation, " illegal use of non-class field `" + resultType.field->name + "` as class type.");
            }
        } else {
            currThread->currTask->file->errors->createNewError(COULD_NOT_RESOLVE,
                                                               resolveLocation, " `" + unresolvedFunction + "` after non class type `" + type_to_str(resultType) + "`.");
        }
    }
}

string get_typed_generic_class_name(unresolved_item &item, List<sharp_type> &resultTypes) {
    string fullname = item.name + "<";

    for(Int i = 0; i < item.typeSpecifiers.size(); i++) {
        resolve(
                *item.typeSpecifiers.get(i),
                resultTypes.__new(),
                false,
                resolve_hard_type,
                item.ast,
                NULL);

        fullname += type_to_str(resultTypes.last());

        if((i + 1) < item.typeSpecifiers.size()) {
            fullname += ", ";
        }
    }

    fullname += ">";
    return fullname;
}

bool resolve_primary_class_generic(
        unresolved_item &item,
        sharp_class *primaryClass,
        string &typedClassName,
        sharp_type &resultType,
        operation_scheme *scheme,
        context &ctx) {

    if(primaryClass->name == typedClassName) {
        resultType.type = type_class;
        resultType._class = primaryClass;

        if(ctx.type == block_context)
            create_dependency(ctx.functionCxt, primaryClass);
        create_dependency(primaryClass, primaryClass);
        return true;
    } else return false;
}

sharp_class *create_generic_class(List<sharp_type> &genericTypes, sharp_class *genericBlueprint) {
    bool created = false;

    sharp_class *generic = create_generic_class(genericBlueprint, genericTypes, created);

    if(created && generic) {
        GUARD2(globalLock)
        genericBlueprint->genericClones.add(generic);
        pre_process_class(NULL, generic, generic->ast);
        process_class(NULL, generic, generic->ast);
        process_generic_extension_functions(generic, genericBlueprint);
        process_generic_mutations(generic, genericBlueprint);
        process_delegates(generic);
        return generic;
    } else if(generic)
        return generic;

    return genericBlueprint;
}

bool resolve_primary_class_inner_class_generic(
        unresolved_item &item,
        sharp_class *primaryClass,
        bool isSelfInstance,
        string &typedClassName,
        sharp_type &resultType,
        List<sharp_type> &genericTypes,
        context &ctx) {

    sharp_class *sc;
    if((sc = resolve_class(primaryClass, typedClassName, false, false)) == NULL) {
        if((sc = resolve_class(primaryClass, item.name, true, false)) != NULL) {
            sc = create_generic_class(genericTypes, sc);
        }
    }

    if(sc) {
        resultType.type = type_class;
        resultType._class = sc;

        if(sc->blueprintClass) {
            int i = 0;
        }
        string fieldType = "class";
        check_access(fieldType, sc->fullName,
                     sc->flags, ctx, isSelfInstance,
                     sc->owner, item.ast);
        if(ctx.type == block_context)
            create_dependency(ctx.functionCxt, sc);
        create_dependency(primaryClass, sc);
        return true;
    }

    return false;
}

bool resolve_global_class_generic(
        unresolved_item &item,
        sharp_class *primaryClass,
        string &typedClassName,
        sharp_type &resultType,
        List<sharp_type> &genericTypes,
        context &ctx) {

    sharp_class *sc;
    if(resultType.type == type_untyped) {
        if((sc = resolve_class(currThread->currTask->file, typedClassName, false, false)) == NULL) {
            if((sc = resolve_class(currThread->currTask->file, item.name, true, false)) != NULL) {
                sc = create_generic_class(genericTypes, sc);
            }
        }
    } else {
        if(resultType.type == type_import_group) {
            if((sc = resolve_class(resultType.group, item.name, false, false)) == NULL) {
                if((sc = resolve_class(resultType.group, item.name, true, false)) != NULL) {
                    sc = create_generic_class(genericTypes, sc);
                }
            }
        } else {
            if((sc = resolve_class(resultType.module, item.name, false, false)) == NULL) {
                if((sc = resolve_class(resultType.module, item.name, true, false)) != NULL) {
                    sc = create_generic_class(genericTypes, sc);
                }
            }
        }
    }

    if(sc != NULL) {
        resultType.type = type_class;
        resultType._class = sc;


        string fieldType = "class";
        check_access(fieldType, sc->fullName,
                     sc->flags, ctx, false,
                     sc->owner, item.ast);
        if(ctx.type == block_context)
            create_dependency(ctx.functionCxt, sc);
        create_dependency(primaryClass, sc);
        return true;
    } else return false;
}

void resolve_generic_item(
        unresolved_item &item,
        sharp_type &resultType,
        bool &hardType,
        operation_scheme *scheme,
        uInt filter,
        Ast *resolveLocation) {
    List<sharp_type> resolvedTypes;
    context &context = currThread->currTask->file->context;
    string typedClassName = get_typed_generic_class_name(item, resolvedTypes);

    if(resultType.type == type_untyped) {
        // first item
        sharp_class *primaryClass = get_primary_class(&context);
        if(primaryClass) {
            if(hasFilter(filter, resolve_filter_class)
               && resolve_primary_class_generic(
                       item, primaryClass,
                       typedClassName, resultType,
                       scheme, context)) {

                hardType = true;
                return;
            }

            if(hasFilter(filter, resolve_filter_inner_class)
               && resolve_primary_class_inner_class_generic(
                       item, primaryClass, true, typedClassName,
                       resultType, resolvedTypes,
                       context)) {

                hardType = true;
                return;
            }

        }

        if(hasFilter(filter, resolve_filter_class)
           && resolve_global_class_generic(
                   item, primaryClass, typedClassName,
                   resultType, resolvedTypes,
                   context)) {

            hardType = true;
            return;
        }

        primaryClass = resolve_class(currThread->currTask->file, item.name, true, true);

        resultType.type = type_undefined;

        if(primaryClass == NULL) {
            currThread->currTask->file->errors->createNewError(COULD_NOT_RESOLVE,
                                                               resolveLocation, " `" + item.name + "` ");
        } else {
            currThread->currTask->file->errors->createNewError(COULD_NOT_RESOLVE,
                                                               resolveLocation, " `" + item.name + "` did you possibly mean: `" + primaryClass->fullName + "`?");
        }
    } else if(resultType.type == type_module || resultType.type == type_import_group) {
        sharp_class *primaryClass = get_primary_class(&context);

        if(hasFilter(filter, resolve_filter_class)
           && resolve_global_class_generic(
                item, primaryClass, typedClassName,
                resultType, resolvedTypes,
                context)) {

            hardType = true;
            return;
        }
    } else {
        if(resultType.type == type_class) {
            sharp_class *primaryClass = resultType._class;

            if(hasFilter(filter, resolve_filter_inner_class)
               && resolve_primary_class_inner_class_generic(
                    item, primaryClass, false, typedClassName,
                    resultType, resolvedTypes,
                    context)) {

                hardType = true;
                return;
            }

            primaryClass = resolve_class(primaryClass, item.name, true, true);

            resultType.type = type_undefined;
            if(primaryClass == NULL) {
                currThread->currTask->file->errors->createNewError(COULD_NOT_RESOLVE,
                                                                   resolveLocation, " `" + item.name + "` ");
            } else {
                currThread->currTask->file->errors->createNewError(COULD_NOT_RESOLVE,
                                                                   resolveLocation, " `" + item.name + "` did you possibly mean: `" + primaryClass->fullName + "`?");
            }
        } else if(resultType.type == type_field) {
            sharp_field *field = resultType.field;
            if(field->type.type == type_class) {
                sharp_class *primaryClass = field->type._class;


                if(hasFilter(filter, resolve_filter_inner_class)
                   && resolve_primary_class_inner_class_generic(
                        item, primaryClass, false, typedClassName,
                        resultType, resolvedTypes,
                        context)) {
                    currThread->currTask->file->errors->createNewError(GENERIC, item.ast->line,item.ast->col,
                                                                       " cannot access inner class through field `" + field->name + "`.");
                    hardType = true;
                    return;
                }

                resultType.type = type_undefined;
                currThread->currTask->file->errors->createNewError(COULD_NOT_RESOLVE,
                                                                   resolveLocation, " `" + item.name + "` in field `" + field->fullName + "`");
            } else {
                currThread->currTask->file->errors->createNewError(GENERIC, item.ast->line,item.ast->col,
                                                                   "field `" + field->name + "` of type `" + type_to_str(field->type) + "` must be of type class.");
            }
        }
        else {
            currThread->currTask->file->errors->createNewError(COULD_NOT_RESOLVE,
                                                               resolveLocation, " `" + item.name + "` after non class type `" + type_to_str(resultType) + "`?");
        }
    }
}

void resolve_item(
        unresolved_item &item,
        sharp_type &resultType,
        bool &hardType,
        operation_scheme *scheme,
        uInt filter,
        Ast *resolveLocation) {
    switch(item.type) {
        case normal_reference:
            resolve_normal_item(item, resultType, hardType, scheme, filter, resolveLocation);
            break;
        case module_reference:
            resolve_module_item(item, resultType, resolveLocation);
            break;
        case operator_reference:
            resolve_operator_item(item, resultType, hardType, scheme, filter, resolveLocation);
            break;
        case generic_reference:
            resolve_generic_item(item, resultType, hardType, scheme, filter, resolveLocation);
            break;
        case function_ptr_reference:
            resolve_function_ptr_item(item, resultType, hardType, scheme, filter, resolveLocation);
            break;
        case function_reference:
            resolve_function_reference_item(item, resultType, hardType, scheme, filter, resolveLocation);
            break;
    }
}

void resolve(
        sharp_type &unresolvedType,
        sharp_type &resultType,
        bool ignoreInitialType,
        uInt filter,
        Ast *resolveLocation,
        operation_scheme *scheme) {
    if(!ignoreInitialType && unresolvedType.type != type_untyped) {
        resultType.copy(unresolvedType);
    } else {
        unresolved_type &type = unresolvedType.unresolvedType;
        bool hardType = false;

        for(Int i = 0; i < type.items.size(); i++) {
            resolve_item(*type.items.get(i), resultType, hardType,
                    scheme, filter, resolveLocation);

            if(resultType.type == type_undefined)
                break;
        }

        if(resultType.type == type_function) {
            resultType = resultType.fun->returnType;
        }

        resultType.isArray = unresolvedType.isArray;
        resultType.nullable = unresolvedType.nullable;

        if(resultType.nullable && !resultType.isArray
            && (resultType.type >= type_int8 && resultType.type <= type_var)) {
            currThread->currTask->file->errors->createNewError(
                    GENERIC, resolveLocation, "expression of type `"
                                                 + type_to_str(resultType) + "` cannot be nullable.");
        }
    }
}

sharp_type expression_type_to_normal_type(expression *e) {
    if(e->type.type == type_integer
        || e->type.type == type_decimal) {
        return sharp_type(type_var);
    } else if(e->type.type == type_bool || e->type.type == type_char) {
        return sharp_type(type_int8);
    } else if(e->type.type == type_string) {
        return sharp_type(type_var, false, true);
    } else {
        return e->type;
    }
}

sharp_type resolve(
        Ast *resolveLocation,
        uInt filter,
        operation_scheme *scheme,
        sharp_class *with_class) {
    sharp_type unresolvedType, resolvedType;

    if(with_class != NULL) {
        resolvedType.type = type_class;
        resolvedType._class = with_class;
    }

    if(resolveLocation->getType() == ast_refrence_pointer) {
        parse_reference_pointer(unresolvedType, resolveLocation);
    } else if(resolveLocation->getType() == ast_utype) {
        parse_utype(unresolvedType, resolveLocation);
    } else if(resolveLocation->getType() == ast_dotnotation_call_expr) {
        parse_utype(unresolvedType, resolveLocation->getSubAst(ast_dot_fn_e)->getSubAst(ast_utype));

        List<expression*> expressions;
        Ast *list = resolveLocation->getSubAst(ast_dot_fn_e)->getSubAst(ast_expression_list);
        for(Int i = 0; i < list->getSubAstCount(); i++) {
            expressions.add(new expression());
            compile_expression(*expressions.last(), list->getSubAst(i));

            if(expressions.last()->scheme.schemeType == scheme_none
                && expressions.last()->type != type_get_component_request) {
                currThread->currTask->file->errors->createNewError(
                        GENERIC, list->getSubAst(i), "expression of type `"
                                      + type_to_str(expressions.last()->type) + "` must evaluate to a value");
            }
        }

        unresolved_item *&item
            = unresolvedType.unresolvedType.items.last();
        if(item->type == normal_reference
            || item->type == operator_reference) {

            item->type = function_reference;
            for(Int i = 0; i < expressions.size(); i++) {
                sharp_type et = expression_type_to_normal_type(expressions.get(i));
                item->typeSpecifiers.add(new sharp_type(et));
                item->operations.add(new operation_scheme());
                item->operations.last()->copy(expressions.get(i)->scheme);
            }
        } else if(item->type == generic_reference) {
            currThread->currTask->file->errors->createNewError(
                    GENERIC, resolveLocation, "generic functions are not supported.");

        } else {
            currThread->currTask->file->errors->createNewError(
                    GENERIC, resolveLocation, "expected function call.");
        }

        deleteList(expressions);
    } else {
        currThread->currTask->file->errors->createNewError(
                INTERNAL_ERROR, resolveLocation->line, resolveLocation->col, "expected ast of utype or type_identifier");
        return sharp_type();
    }

    resolve(
            unresolvedType,
            resolvedType,
            with_class != NULL,
            filter,
            resolveLocation,
            scheme);

    return resolvedType;
}

void create_dependency(sharp_class* depender, sharp_class* dependee) {
    if(depender != dependee)
        depender->dependencies.addif(dependency(dependee));
    create_dependency(depender->implLocation.file, dependee->implLocation.file);
}

void create_dependency(sharp_file* depender, sharp_file* dependee) {
    if(depender != dependee)
        depender->dependencies.addif(dependency(dependee));
}

void create_dependency(sharp_function* depender, sharp_function* dependee) {
    if(depender != dependee)
        depender->dependencies.addif(dependency(dependee));
    create_dependency(depender->owner, dependee->owner);
}

void create_dependency(sharp_function* depender, sharp_class* dependee) {
    depender->dependencies.addif(dependency(dependee));
    create_dependency(depender->owner, dependee);
}

void create_dependency(sharp_function* depender, sharp_field* dependee) {
    depender->dependencies.addif(dependency(dependee));
    create_dependency(depender->owner, dependee->owner);
}

void create_dependency(sharp_field* depender, sharp_function* dependee) {
    depender->dependencies.addif(dependency(dependee));
    create_dependency(depender->owner, dependee->owner);
}

void create_dependency(sharp_field* depender, sharp_class* dependee) {
    depender->dependencies.addif(dependency(dependee));
    create_dependency(depender->owner, dependee);
}

void create_dependency(sharp_field* depender, sharp_field* dependee) {
    if(depender != dependee)
        depender->dependencies.addif(dependency(dependee));
    create_dependency(depender->owner, dependee->owner);
}

