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

        if(field->name == "feep") {
            int r = 0;
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
    uInt flags = flag_none;

    if(check_flag(field->flags, flag_local))
        flags |= flag_local;

    if(check_flag(field->flags, flag_public))
        flags |= flag_public;
    else if(check_flag(field->flags, flag_private))
        flags |= flag_private;
    else if(check_flag(field->flags, flag_protected))
        flags |= flag_protected;

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
            ast->getToken(1).getValue(),
            field->owner,
            location,
            field->type,
            flags,
            normal_field,
            ast));

    sharp_type returnType(type_nil);
    string name = "set_" + field->name;

    GUARD(globalLock)
    if(!create_function(
            field->owner,
            flags,
            normal_function,
            name,
            false,
            fields,
            returnType,
            ast
            )) {
        deleteList(fields);
    }

    field->setter = field->owner->functions.last();
}

void process_getter(sharp_field *field, Ast *ast) {
    uInt flags = flag_none;

    if(check_flag(field->flags, flag_local))
        flags |= flag_local;

    if(check_flag(field->flags, flag_public))
        flags |= flag_public;
    else if(check_flag(field->flags, flag_private))
        flags |= flag_private;
    else if(check_flag(field->flags, flag_protected))
        flags |= flag_protected;


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
    } else if(type.type == type_integer
        || type.type == type_decimal) {
        field->type.type = type.type == type_decimal ? type_var : type_int64;
        return;
    } else if(type.type == type_char
        || type.type == type_bool) {
        field->type.type = type_int8;
        return;
    } else if(type.type == type_string) {
        field->type.type = type_int8;
        field->type.isArray = true;
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

        field->type = type.field->type;
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
