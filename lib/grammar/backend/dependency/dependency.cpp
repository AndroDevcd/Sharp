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

        if((matchName && sc->name.find(name) != string::npos)
            || sc->name == name) {
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
    GUARD(sc->mut)
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


sharp_field* resolve_field(string name, sharp_module *module) {
    sharp_class *sc = NULL;
    sharp_field *sf = NULL;
    GUARD(module->moduleLock)
    List<sharp_class*> *searchList = &module->classes;

    for(Int i = 0; i < searchList->size(); i++) {
        sc = searchList->get(i);

        if(check_flag(sc->flags, flag_global)
            && (sf = resolve_field(name, sc)) != NULL) {
            return sf;
        }
    }

    return NULL;
}

sharp_field* resolve_field(string name, import_group *group) {
    List<sharp_module*> &imports = group->modules;
    sharp_field *sf;

    for(Int i = 0; i < imports.size(); i++) {
        if((sf = resolve_field(name, imports.get(i))) != NULL) {
            return sf;
        }
    }

    return NULL;
}

sharp_field* resolve_field(string name, sharp_file *file) {
    List<sharp_module*> &imports = file->imports;
    sharp_field *sf;

    for(Int i = 0; i < imports.size(); i++) {
        if((sf = resolve_field(name, imports.get(i))) != NULL) {
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

sharp_field* resolve_field(string name, sharp_class *searchClass) {
    GUARD(searchClass->mut)
    for(Int i = 0; i < searchClass->fields.size(); i++) {
        if(searchClass->fields.get(i)->name == name) {
            return searchClass->fields.get(i);
        }
    }

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
    GUARD(searchClass->mut)

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
    sharp_function mock_function("mock", searchClass, impl_location(),
            flag_none, NULL, parameters, sharp_type(), undefined_function);

    locate_functions_with_name(name, searchClass, functionType, checkBaseClass,
            locatedFunctions);

    if(!locatedFunctions.empty()) {
        for(Int i = 0; i < locatedFunctions.size(); i++) {
            sharp_function *func = locatedFunctions.get(i);
            mock_function.returnType = func->returnType;

            if(is_explicit_type_match(sharp_type(func), sharp_type(&mock_function))) {
                if(resolvedFunction == NULL) resolvedFunction = func;
                else ambiguous = true;
            }
        }

        if(implicitCheck) {

            for(Int i = 0; i < locatedFunctions.size(); i++) {
                sharp_function *func = locatedFunctions.get(i);
                mock_function.returnType = func->returnType;

                if(is_implicit_type_match(
                        sharp_type(func),
                        sharp_type(&mock_function),
                        excludedMatches)) {
                    if(resolvedFunction == NULL) resolvedFunction = func;
                    else ambiguous = true;
                }
            }
        }
    }

    if(ambiguous && resolveLocation != NULL) {
        currThread->currTask->file->errors->createNewError(GENERIC, resolveLocation->line, resolveLocation->col, "call to method `" + name + "` is ambiguous");
    }

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


bool resolve_local_field(
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
        stored_context_item &contextItem = ctx.storedItems.get(i);

        if((field = resolve_local_field(item.name, &contextItem)) != NULL) {

            if(ctx.functionCxt == contextItem.functionCxt) { // same func no closure needed
                resultType.field = field;

                create_local_field_access_operation(scheme, field);

                if(field->type.type == type_class)
                    create_dependency(ctx.functionCxt, field->type._class);
            } else {
                if(can_capture_closure(field)) {
                    sharp_class *closure_class = create_closure_class(
                            currThread->currTask->file, currModule, contextItem.functionCxt,
                            item.ast);
                    create_closure_field(closure_class, item.name,
                            field->type, item.ast);
                    sharp_field *staticClosureRef = create_closure_field(
                            resolve_class(currModule, global_class_name, false, false),
                            "closure_ref_" + ctx.functionCxt->fullName,
                            sharp_type(closure_class),
                            item.ast
                    );

                    field->closures.add(staticClosureRef);
                    resultType.field = staticClosureRef;

                    create_static_field_access_operation(scheme, staticClosureRef);

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


bool resolve_primary_class_field(
        unresolved_item &item,
        sharp_class *primaryClass,
        bool isSelfInstance,
        sharp_type &resultType,
        operation_scheme *scheme,
        context &ctx) {

    sharp_field *field;
    if((field = resolve_field(item.name, primaryClass)) != NULL) {
        resultType.type = type_field;
        resultType.field = field;
        resolve_field(field);

        if(field->getter != NULL) {
            if(isSelfInstance) create_instance_field_getter_operation(scheme, field);
            else create_primary_instance_field_getter_operation(scheme, field);
            create_dependency(ctx.functionCxt, field->getter);
        } else {
            if(isSelfInstance) create_instance_field_access_operation(scheme, field);
            else create_primary_instance_field_access_operation(scheme, field);
        }

        if(ctx.type == block_context)
            create_dependency(ctx.functionCxt, field);
        return true;
    } else return false;
}

bool resolve_primary_class_alias(
        unresolved_item &item,
        sharp_class *primaryClass,
        sharp_type &resultType,
        operation_scheme *scheme,
        context &ctx) {

    sharp_alias *alias;
    if((alias = resolve_alias(item.name, primaryClass)) != NULL) {
        resolve_alias(alias);
        resultType.copy(alias->type);

        if(alias->operation && scheme) {
            scheme->copy(*alias->operation);
        }

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
        sharp_type &resultType,
        operation_scheme *scheme,
        context &ctx) {

    sharp_field *field;
    if((field = resolve_enum(item.name, primaryClass)) != NULL) {
        resultType.type = type_field;
        resultType.field = field;
        resolve_field(field);

        create_static_field_access_operation(scheme, field);

        if(ctx.type == block_context)
            create_dependency(ctx.functionCxt, field);
        return true;
    } else return false;
}

bool resolve_primary_class_function_address(
        unresolved_item &item,
        sharp_class *primaryClass,
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

        resultType.type = type_function;
        resultType.fun = functions.first();
        resolve_function(functions.first());

        create_get_static_function_address_operation(scheme, functions.first());

        if(ctx.type == block_context)
            create_dependency(ctx.functionCxt, functions.first());
        return true;
    } else return false;
}

bool resolve_primary_class_inner_class(
        unresolved_item &item,
        sharp_class *primaryClass,
        sharp_type &resultType,
        operation_scheme *scheme,
        context &ctx) {

    sharp_class *sc;
    if((sc = resolve_class(primaryClass, item.name, false, false)) != NULL) {
        resultType.type = type_class;
        resultType._class = sc;

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
        resolve_field(field);

        if(field->getter != NULL) {
            create_primary_instance_field_getter_operation(scheme, field);
            create_dependency(ctx.functionCxt, field->getter);
        } else {
            create_primary_instance_field_access_operation(scheme, field);
        }

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
        resolve_alias(alias);
        resultType.copy(alias->type);

        if(alias->operation && scheme) {
            scheme->copy(*alias->operation);
        }

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
        resolve_field(field);

        create_static_field_access_operation(scheme, field);

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
                               " getting address from ambiguous function reference `" + item.name + "` in class " + primaryClass->fullName + "`");
        }

        if(!check_flag(functions.first()->flags, flag_static)) {
            currThread->currTask->file->errors->createNewError(GENERIC,
                                                               item.ast, " cannot get address of non-static function `" + functions.first()->fullName + "` ");
        }

        resultType.type = type_function;
        resultType.fun = functions.first();
        resolve_function(functions.first());

        create_get_static_function_address_operation(scheme, functions.first());

        if(ctx.type == block_context)
            create_dependency(ctx.functionCxt, functions.first());
        return true;
    } else return false;
}

bool resolve_global_class(
        unresolved_item &item,
        sharp_class *primaryClass,
        sharp_type &resultType,
        context &ctx) {

    sharp_class *sc;
    if(resultType.type == type_untyped) {
        sc = resolve_class(currThread->currTask->file, item.name, false, false);
    } else {
        if(resultType.type == type_import_group) {
            sc = resolve_class(resultType.group, item.name, false, false);
        } else {
            sc = resolve_class(resultType.module, item.name, false, false);
        }
    }

    if(sc != NULL) {
        resultType.type = type_class;
        resultType._class = sc;

        if(ctx.type == block_context)
            create_dependency(ctx.functionCxt, sc);
        create_dependency(primaryClass, sc);
        return true;
    } else return false;
}

// resolve logic flow
// local fields -> class instance / static fields -> class aliases -> class enums -> function reference via name
// local class (inner class inside primary class) -> Primary class ->  global field -> global alias -> global enums
// -> global function reference -> global_class
void resolve_normal_item(
        unresolved_item &item,
        sharp_type &resultType,
        operation_scheme *scheme,
        uInt filter,
        Ast *resolveLocation) {
    context &context = currThread->currTask->file->context;

    if(resultType.type == type_untyped) {
        // first item
        if(context.type == block_context
            && hasFilter(filter, resolve_filter_local_field)
            && resolve_local_field(item, resultType, scheme, context)) {

            return;
        }

        sharp_class *primaryClass = get_primary_class(&context);
        if(primaryClass) {
            if(hasFilter(filter, resolve_filter_class_field)
               && resolve_primary_class_field(item, primaryClass, true, resultType, scheme, context)) {

                return;
            }

            if(hasFilter(filter, resolve_filter_class_alias)
               && resolve_primary_class_alias(item, primaryClass, resultType, scheme, context)) {

                return;
            }

            if(hasFilter(filter, resolve_filter_class_enum)
               && resolve_primary_class_enum(item, primaryClass, resultType, scheme, context)) {

                return;
            }

            if(hasFilter(filter, resolve_filter_function_address) // todo: come back for function resolution
               && resolve_primary_class_function_address(item, primaryClass, resultType, scheme, context)) {

                return;
            }

            if(hasFilter(filter, resolve_filter_inner_class)
               && resolve_primary_class_inner_class(item, primaryClass, resultType, scheme, context)) {

                return;
            }

            if(hasFilter(filter, resolve_filter_class)
               && resolve_primary_class(item, primaryClass, resultType, scheme, context)) {

                return;
            }
        }

        if(hasFilter(filter, resolve_filter_field)
           && resolve_global_class_field(item, resultType, scheme, context)) {

            return;
        }

        if(hasFilter(filter, resolve_filter_alias)
           && resolve_global_class_alias(item, resultType, scheme, context)) {

            return;
        }

        if(hasFilter(filter, resolve_filter_enum)
           && resolve_global_class_enum(item, resultType, scheme, context)) {

            return;
        }

        if(hasFilter(filter, resolve_filter_function_address)
           && resolve_global_class_function_address(item, resultType, scheme, context)) {

            return;
        }

        if(hasFilter(filter, resolve_filter_class)
           && resolve_global_class(item, primaryClass, resultType, context)) {

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

            return;
        }

        if(hasFilter(filter, resolve_filter_alias)
           && resolve_global_class_alias(item, resultType, scheme, context)) {

            return;
        }

        if(hasFilter(filter, resolve_filter_enum)
           && resolve_global_class_enum(item, resultType, scheme, context)) {

            return;
        }

        if(hasFilter(filter, resolve_filter_function_address)
           && resolve_global_class_function_address(item, resultType, scheme, context)) {

            return;
        }

        if(primaryClass) {
            if (hasFilter(filter, resolve_filter_class)
                && resolve_global_class(item, primaryClass, resultType, context)) {

                return;
            }
        }
    } else {
        if(resultType.type == type_class) {
            sharp_class *primaryClass = resultType._class;

            if(hasFilter(filter, resolve_filter_class_field)
               && resolve_primary_class_field(item, primaryClass, false, resultType, scheme, context)) {

                return;
            }

            if(hasFilter(filter, resolve_filter_class_alias)
               && resolve_primary_class_alias(item, primaryClass, resultType, scheme, context)) {

                return;
            }

            if(hasFilter(filter, resolve_filter_function_address) // todo: come back for function resolution
               && resolve_primary_class_function_address(item, primaryClass, resultType, scheme, context)) {

                return;
            }

            if(hasFilter(filter, resolve_filter_inner_class)
               && resolve_primary_class_inner_class(item, primaryClass, resultType, scheme, context)) {

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
        } else {
            currThread->currTask->file->errors->createNewError(COULD_NOT_RESOLVE,
                               resolveLocation, " `" + item.name + "` after non class type `" + type_to_str(resultType) + "`?");
        }
    }
}

void resolve_item(
        unresolved_item &item,
        sharp_type &resultType,
        operation_scheme *scheme,
        uInt filter,
        Ast *resolveLocation) {
    switch(item.type) {
        case normal_reference:
            resolve_normal_item(item, resultType, scheme, filter, resolveLocation);
            break;
        case module_reference:
            break;
        case operator_reference:
            break;
        case generic_reference:
            break;
        case function_ptr_reference:
            break;
        case function_reference:
            break;
    }
}

void resolve(
        sharp_type &unresolvedType,
        sharp_type &resultType,
        uInt filter,
        Ast *resolveLocation,
        operation_scheme *scheme,
        List<sharp_type> *parameters) { // todo: process parameters
    if(!(unresolvedType.type == type_untyped
        && unresolvedType.unresolvedType != NULL)) {
        resultType.copy(unresolvedType);
    } else {
        unresolved_type *type = unresolvedType.unresolvedType;
        resultType.type = type_untyped;

        for(Int i = 0; i < type->items.size(); i++) {
            resolve_item(type->items.get(i), resultType,
                    scheme, filter, resolveLocation);


            if(resultType.type == type_undefined)
                break;
        }
    }
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

