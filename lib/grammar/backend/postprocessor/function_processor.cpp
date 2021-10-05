//
// Created by BNunnally on 9/11/2021.
//

#include <iomanip>
#include "function_processor.h"
#include "../astparser/ast_parser.h"
#include "../../taskdelegator/task_delegator.h"
#include "../types/sharp_class.h"
#include "../types/sharp_field.h"
#include "field_processor.h"

void process_function(
        sharp_class *with_class,
        function_type type,
        Ast *ast) {
    string name;

    if(type == initializer_function) {
        name = "init<" + with_class->name + ">";
    } else if(type == constructor_function) {
        name = ast->getToken(0).getValue();

        long pos;
        string class_name = with_class->name;
        if((pos = with_class->name.find('<')) != string::npos) {
            class_name = with_class->name.substr(0, pos);
        }

        if(name != class_name) {
            currThread->currTask->file->errors->createNewError(GENERIC, ast,
                       "constructor `" + name + "` must match holding class name `" + class_name + "`");
        }
    } else if(ast->hasSubAst(ast_refrence_pointer))
        process_extension_class(with_class, type == delegate_function, name, ast->getSubAst(ast_refrence_pointer));
    else
        name = ast->getToken(0).getValue();


    if(name.find("operator") != string::npos)
        type = operator_function;

    if(with_class != NULL) {
        if(with_class->blueprintClass) {
            with_class->extensionFunctions.add(unresolved_extension_function(name, ast, type));
            process_generic_clone_functions(with_class, name, type, ast);
        } else {
            process_function(with_class, name, type, ast);
        }
    }
}

void process_extension_class(
        sharp_class *&with_class,
        bool isDelegate,
        string &name,
        Ast *ast) {
    if(ast->getTokenCount() > 0) {
        name = ast->getToken(0).getValue();
    } else {
        sharp_type unresolvedType, resultType;
        parse_reference_pointer(unresolvedType, ast);

        unresolved_item &firstItem =
                *unresolvedType.unresolvedType.items.first();
        unresolved_item *lastItem =
                unresolvedType.unresolvedType.items.last();
        if(firstItem.type == operator_reference) {
            name = firstItem.name;
        } else {
            name = lastItem->name;
            if(isDelegate) {
                currThread->currTask->file->errors->createNewError(GENERIC, ast,
                      "extension function `" + name + "` cannot be a delegate.");
            }

            unresolvedType.unresolvedType.items.pop_back();

            resolve(
                    unresolvedType,
                    resultType,
                    false,
                    resolve_hard_type | resolve_generic_type,
                    ast);

            if(resultType.type == type_class) {
                with_class = resultType._class;
            } else {
                with_class = NULL;
                currThread->currTask->file->errors->createNewError(GENERIC, ast,
                        "extension function `" + name + "` must be extended by a class");
            }
        }
    }
}

void process_generic_clone_functions(
        sharp_class *with_class,
        string &name,
        function_type type,
        Ast *ast) {
    GUARD(with_class->mut)

    for(Int i = 0; i < with_class->genericClones.size(); i++) {
        process_function(with_class->genericClones.get(i), name, type, ast);
    }
}

void process_generic_extension_functions(
        sharp_class *with_class,
        sharp_class *blueprint) {
    GUARD(blueprint->mut)

    for(Int i = 0; i < blueprint->extensionFunctions.size(); i++) {
        unresolved_extension_function uef = blueprint->extensionFunctions.get(i);
        process_function(
                with_class,
                uef.name,
                uef.type,
                uef.ast
        );
    }
}

void process_function(
        sharp_class *with_class,
        string &name,
        function_type type,
        Ast *ast) {

    GUARD2(with_class->mut)
    uInt flags = flag_none;
    List<sharp_field*> params;
    if(ast->hasSubAst(ast_access_type)) {
        flags = parse_access_flags(
                flag_public
                | flag_private | flag_protected | flag_local
                | flag_static | flag_native,
                "function", with_class,
                ast->getSubAst(ast_access_type)
        );

        if(check_flag(flags, flag_native)) {
            type = native_function;
        }

        if(!check_flag(flags, flag_public) && !check_flag(flags, flag_private)
           && !check_flag(flags, flag_protected))
            flags |= flag_public;

        if(!check_flag(flags, flag_public))
            flags |= flag_public;
    } else flags = flag_public;

    parse_utype_arg_list(params, ast->getSubAst(ast_utype_arg_list));
    for(Int i = 0; i < params.size(); i++) {
        sharp_field *param = params.get(i);
        sharp_type resolvedType;

        param->owner = with_class;
        resolve(param->type, resolvedType, false,
                resolve_hard_type, param->ast);
        param->type.free();
        param->type = sharp_type();

        validate_field_type(true, param, resolvedType, NULL, param->ast);
    }

    sharp_type returnType;

    if(create_function(
            with_class, flags, type,
            name, false, params,
            returnType, ast)) {
        sharp_function *fun = with_class->functions.last();

        if(fun->name == "to_string" && fun->owner->name == "_object_") {

            create_context(fun);
            string fname = "%test";
            create_function(with_class, flag_static, type, fname, false,
                    params, returnType, ast);

            create_context(with_class->functions.last());
            expression e;
            compile_expression(e, fun->ast->getSubAst(ast_expression));

            validate_function_type(false, fun, e.type, &e.scheme, fun->ast);
            delete_context();
            delete_context();
        }

        if(ast->hasSubAst(ast_method_return_type)) {
            Ast *returnTypeAst = ast->getSubAst(ast_method_return_type);

            if(returnTypeAst->hasToken("nil")) {
                returnType.type = type_nil;
            } else {
                returnType = resolve(returnTypeAst->getSubAst(ast_utype));
                validate_function_type(true, fun, returnType, NULL, fun->ast);
            }

        } else if(ast->hasToken(":=")) {
            returnType.type = type_untyped;
        } else {
            returnType.type = type_nil;
        }

        fun->returnType.copy(returnType);
    }
}

void process_function_return_type(sharp_function *fun) {
    if(fun->returnType.type == type_untyped
        && fun->ast->hasSubAst(ast_expression)
        && currThread->currTask->file->stage >= pre_compilation_finished_state) {
        fun->returnType.type = type_undefined;

        create_context(fun);
        expression e;
        compile_expression(e, fun->ast->getSubAst(ast_expression));
        validate_function_type(false, fun, e.type, &e.scheme, fun->ast);
        delete_context();
    }
}


void validate_function_type(
        bool hardType,
        sharp_function *fun,
        sharp_type &type,
        operation_scheme *scheme,
        Ast *ast) {
    if(type.type == type_class
       || type.type == type_nil
       || (type.type >= type_int8 && type.type <= type_object)) {
        if(type.type == type_class)
            create_dependency(fun, type._class);

        if(!hardType && scheme->steps.empty()) {
            currThread->currTask->file->errors->createNewError(GENERIC, ast, " cannot assign hard type as value for function `" + fun->fullName + "`");
            return;
        }
    } else if(type.type == type_integer
              || type.type == type_decimal) {
        fun->returnType.type = type_var;
        return;
    } else if(type.type == type_char
              || type.type == type_bool) {
        fun->returnType.type = type_int8;
        return;
    } else if(type.type == type_null) {
        fun->returnType.type = type_object;
        return;
    } else if(type.type == type_string) { // todo: default it to string type
        fun->returnType.type = type_int8;
        fun->returnType.isArray = true;
        return;
    } else if(type.type == type_field) {
        create_dependency(fun, type.field);

        if(hardType) {
            currThread->currTask->file->errors->createNewError(GENERIC, ast, " illegal use of field `" + type.field->fullName + "` as a type");
            return;
        }

        fun->returnType = type.field->type;
        return;
    } else if(type.type == type_lambda_function) {
        create_dependency(fun, type.fun);

        if(!is_fully_qualified_function(type.fun)) {
            currThread->currTask->file->errors->createNewError(
                    GENERIC, ast->line, ast->col, "expression being assigned to function `" + fun->fullName + "` is not a fully qualified lambda expression. Try assigning types to all of the fields in your lambda.");
        } else {
            type.type = type_function_ptr;
        }
    } else if(type.type == type_function_ptr) {
        create_dependency(fun, type.fun);

        if(hardType && type.fun->type != blueprint_function) {
            currThread->currTask->file->errors->createNewError(
                    GENERIC, ast->line, ast->col, " function `" + fun->fullName + "` cannot have a method representing the type.");
        } else if(!hardType && type.fun->type == blueprint_function) {
            currThread->currTask->file->errors->createNewError(
                    GENERIC, ast->line, ast->col, "expression being assigned to function `" + fun->fullName + "` must resolve to a class, object, lambda, or number value.");
        }
    } else if(type.type == type_get_component_request) {
        currThread->currTask->file->errors->createNewError(
                GENERIC, ast->line, ast->col,
                "cannot assign `get()` to function `" + fun->fullName + "` outside a type definition block.");
    } else if(type.type != type_undefined) {
        currThread->currTask->file->errors->createNewError(GENERIC, ast->line, ast->col,
                 " function `" + fun->fullName + "` cannot be assigned type `" + type_to_str(type) +
                 "` due to invalid type assignment format");
    }

    fun->returnType = type;
}
