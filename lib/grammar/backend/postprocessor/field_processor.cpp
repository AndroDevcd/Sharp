//
// Created by BNunnally on 9/10/2021.
//

#include "field_processor.h"
#include "../../frontend/parser/Parser.h"
#include "../types/sharp_class.h"
#include "../types/sharp_field.h"
#include "../../taskdelegator/task_delegator.h"
#include "../compiler/expressions/expression.h"
#include "../operation/operation.h"
#include "../types/sharp_function.h"
#include "../compiler/field_compiler.h"
#include "../../compiler_info.h"
#include "../astparser/ast_parser.h"
#include "../preprocessor/class_preprocessor.h"

void process_field(sharp_class *with_class, Ast *ast) {
    string name;
    if(parser::isStorageType(ast->getToken(0))) {
        name = ast->getToken(1).getValue();
    } else name = ast->getToken(0).getValue();

    sharp_field *field = resolve_field(name, with_class);

    process_field(field);
}

void process_field(sharp_field *field) {
    GUARD(globalLock)

    create_context(field);
    Ast *ast = field->ast;
    if(field->type.type == type_untyped) {
        field->type.type = type_undefined;

        if(field->ast->hasSubAst(ast_inject_request)) {
            if(field->ast->hasSubAst(ast_expression)) {
                create_new_error(GENERIC, ast->line, ast->col,
                            "injected field `" + field->name +
                                  "` cannot be assigned a value");
            }

            Ast *irAst = field->ast->getSubAst(ast_inject_request);
            if(irAst->getTokenCount() > 0) {
                field->request = new injection_request(irAst->getToken(0).getValue());
            } else {
                string anyComponent = any_component_name; // we must find the component to inject the field value
                field->request = new injection_request(anyComponent);
            }
        }

        if(field->ast->hasToken(COLON)) {
            sharp_type type = resolve(field->ast->getSubAst(ast_utype));
            validate_field_type(true, field, type, NULL, field->ast);
        } else if(ast->hasSubAst(ast_expression)) {
            field->type.type = type_untyped;
            if(current_file->stage >= pre_compilation_state) {
                expression e;
                compile_expression(e, ast->getSubAst(ast_expression));
                validate_field_type(false, field, e.type, &e.scheme, field->ast);
            }
        }

        if(field->type.type != type_undefined) {
            if(ast->hasSubAst(ast_setter)) {
                process_setter(field, ast->getSubAst(ast_setter));
            }

            if(ast->hasSubAst(ast_getter)) {
                process_getter(field, ast->getSubAst(ast_getter));
            }
        }

        if(ast->hasSubAst(ast_variable_decl)) {
            long startAst = 0;
            for(long i = 0; i < ast->getSubAstCount(); i++) {
                if(ast->getSubAst(i)->getType() == ast_variable_decl)
                {
                    startAst = i;
                    break;
                }
            }

            for(long i = startAst; i < ast->getSubAstCount(); i++) {
                Ast *trunk = ast->getSubAst(i);

                string name = trunk->getToken(0).getValue();
                sharp_field *xtraField = resolve_field(name, field->owner);

                if(xtraField) {
                    xtraField->fieldType = field->fieldType;
                    xtraField->type = field->type;
                }
            }
        }
    }

    delete_context();
}

void process_setter(sharp_field *field, Ast *ast) {
    uInt flags;
    if(ast->hasSubAst(ast_access_type)) {
        flags = parse_access_flags(
                flag_public
                | flag_private | flag_protected
                | flag_local | flag_static,
                "setter function", field->owner,
                ast->getSubAst(ast_access_type)
        );

        if(!check_flag(flags, flag_public) && !check_flag(flags, flag_private)
           && !check_flag(flags, flag_protected))
            flags |= flag_public;

        if(!check_flag(flags, flag_public))
            flags |= flag_public;
    } else flags = field->flags;

    if(check_flag(field->flags, flag_const)) {
        create_new_error(GENERIC, ast->line, ast->col,
                               "cannot apply setter to constant field `" + field->name + "`");
    }

    if(field->fieldType == tls_field) {
        create_new_error(GENERIC, ast->line, ast->col,
                               "cannot apply setter to thread_local field `" + field->name + "`");
    }

    List<sharp_field*> fields;
    impl_location location(currThread->currTask->file, ast);
    fields.add(new sharp_field(
            ast->getToken(0).getValue(),
            field->owner,
            location,
            field->type,
            flags,
            normal_field,
            ast));

    sharp_type returnType(type_nil);
    string name = "set_" + field->name;

    GUARD(globalLock)
    sharp_function *fun = NULL;
    if(!create_function(
            field->owner,
            flags,
            normal_function,
            name,
            false,
            fields,
            returnType,
            ast,
            fun)) {
        deleteList(fields);
    } else {
        field->setter = fun;
        fun->locals.addAll(fun->parameters);
    }
}

void process_getter(sharp_field *field, Ast *ast) {
    uInt flags;

    bool threadSafe = false;
    if(ast->hasSubAst(ast_access_type)) {
        flags = parse_access_flags(
                flag_public
                | flag_private | flag_protected
                | flag_local | flag_static,
                "getter function", field->owner,
                ast->getSubAst(ast_access_type)
        );

        if(!check_flag(flags, flag_public) && !check_flag(flags, flag_private)
           && !check_flag(flags, flag_protected))
            flags |= flag_public;

        if(!check_flag(flags, flag_public))
            flags |= flag_public;
    } else flags = field->flags;

    if(flags != field->flags) {
        create_new_error(GENERIC, ast->line, ast->col,
                         "getter for field `" + field->name + "`, cannot have differing access types from the field itself.");
    }

    if(field->fieldType == tls_field) {
        create_new_error(GENERIC, ast->line, ast->col,
                                                           "cannot apply getter to thread_local field `" + field->name + "`");
    }

    List<sharp_field*> fields;

    sharp_type returnType(field->type);
    string name = "get_" + field->name;

    GUARD(globalLock)
    create_function(
            field->owner,
            flags,
            normal_function,
            name,
            false,
            fields,
            returnType,
            ast
    );

    field->getter = field->owner->functions.last();
}

void create_static_init_flag_field(sharp_class *owner, Ast *ast) {
    string name = static_init_flag_name;
    field_type ft = normal_field;
    uInt flags = flag_private | flag_static  | flag_excuse;

    if(resolve_field(name, owner, false) == NULL) {
        check_decl_conflicts(ast, owner, "field", name);
        create_field(
                currThread->currTask->file,
                owner, name, flags, sharp_type(type_var),
                ft, ast
        );
    }
}

void validate_field_type(
        bool hardType,
        sharp_field *field,
        sharp_type &type,
        operation_schema *scheme,
        Ast *ast) {
    if(type.type == type_class
        || (type.type >= type_int8 && type.type <= type_object)) {
        if(type.type == type_class)
            create_dependency(type._class);

        if(!hardType && scheme->steps.empty()) {
            create_new_error(GENERIC, ast, " cannot assign hard type as value for field `" + field->fullName + "`");
            return;
        }
        /*else if (!hardType) { // todo: i dont think this is needed but keeping just in case
            string module = "std";
            string className = "";

            if(!type.isArray) {
                switch (type.type) {
                    case type_int8:
                        className = "char";
                        break;
                    case type_int16:
                        className = "short";
                        break;
                    case type_int32:
                        className = "int";
                        break;
                    case type_int64:
                        className = "long";
                        break;
                    case type_uint8:
                        className = "uchar";
                        break;
                    case type_uint16:
                        className = "ushort";
                        break;
                    case type_uint32:
                        className = "uint";
                        break;
                    case type_uint64:
                        className = "ulong";
                        break;
                    case type_var:
                        className = "double";
                        break;
                    case type_function_ptr:
                    case type_object:
                        // do notjing
                        break;
                    default: {
                        create_new_error(GENERIC, ast,
                                         " failed to assign wrapper class for field `" + field->fullName +
                                         "`, where type `" + type_to_str(type) + "` was found instead.");
                        break;
                    }
                }
            }

            if(className != "") {
                auto wrapperClass = sharp_type(resolve_class(get_module(module), className, false, false));
                if (wrapperClass._class != NULL) {
                    create_dependency(wrapperClass._class);
                    field->type.copy(wrapperClass);
                    return;
                } else {
                    create_new_error(GENERIC, ast,
                                     " failed to assign wrapper class `" + className + "` type for field `" + field->fullName +
                                     "`");
                }
            }
        }*/
    } else if(type.type == type_integer
        || type.type == type_decimal) {
        string module = "std";
        string className = "";
        if(type.type == type_integer) {
            className = "int";
        } else {
            className = "double";
        }

        auto wrapperClass = sharp_type(resolve_class(get_module(module), className, false, false));
        if (wrapperClass._class != NULL) {
            create_dependency(wrapperClass._class);
            field->type.copy(wrapperClass);
            return;
        } else {
            create_new_error(GENERIC, ast,
                             " failed to assign wrapper class `" + className + "` type for field `" + field->fullName +
                             "`");
        }
        field->type.type = type_undefined;
        return;
    } else if(type.type == type_char
        || type.type == type_bool) {
        string module = "std";
        string className = "";
        if(type.type == type_char) {
            className = "char";
        } else {
            className = "bool";
        }

        auto wrapperClass = sharp_type(resolve_class(get_module(module), className, false, false));
        if (wrapperClass._class != NULL) {
            create_dependency(wrapperClass._class);
            field->type.copy(wrapperClass);
            return;
        } else {
            create_new_error(GENERIC, ast,
                             " failed to assign wrapper class `" + className + "` type for field `" + field->fullName +
                             "`");
        }
        field->type.type = type_undefined;
        return;
    } else if(type.type == type_string) {
        string module = "std";
        string className = "string";
        auto wrapperClass = sharp_type(resolve_class(get_module(module), className, false, false));
        if (wrapperClass._class != NULL) {
            create_dependency(wrapperClass._class);
            field->type.copy(wrapperClass);
            return;
        } else {
            create_new_error(GENERIC, ast,
                             " failed to assign wrapper class `" + className + "` type for field `" + field->fullName +
                             "`");
        }
        field->type.type = type_undefined;
        return;
    } else if(type.type == type_null) {
        field->type.type = type_object;
        return;
    } else if(type.type == type_field) {
        create_dependency(type.field);

        if(hardType) {
            create_new_error(GENERIC, ast, " illegal use of field `" + type.field->fullName + "` as a type");
            return;
        } else {
            if(type.field == field) {
                create_new_error(GENERIC, ast, " cannot assign field `" + field->fullName + "` to its-self");
                return;
            }
        }

        bool nullable = type.nullable || type.field->type.nullable;
        field->type = type.field->type;
        field->type.nullable = nullable;
        return;
    } else if(type.type == type_lambda_function) {
        create_dependency(type.fun);

        if(!is_fully_qualified_function(type.fun)) {
            create_new_error(
                    GENERIC, ast->line, ast->col, "expression being assigned to field `" + field->fullName + "` is not a fully qualified lambda expression. Try assigning types to all of the fields in your lambda.");
        } else {
            type.type = type_function_ptr;
        }
    } else if(type.type == type_function_ptr) {
        create_dependency(type.fun);

        if(hardType && type.fun->type != blueprint_function) {
            create_new_error(
                    GENERIC, ast->line, ast->col, " field `" + field->fullName + "` cannot have a method representing the type.");
        } else if(!hardType && type.fun->type == blueprint_function) {
            create_new_error(
                    GENERIC, ast->line, ast->col, "expression being assigned to field `" + field->fullName + "` must resolve to a class, object, lambda, or number value.");
        }
    } else if(type.type == type_get_component_request) {
        create_new_error(
                GENERIC, ast->line, ast->col,
                "cannot resolve type from `get()` expression.");
    } else if(type.type != type_undefined) {
        create_new_error(GENERIC, ast->line, ast->col,
                 " field `" + field->fullName + "` cannot be assigned type `" + type_to_str(type) +
                 "` due to invalid type assignment format");
    }

    field->type.copy(type);
}
